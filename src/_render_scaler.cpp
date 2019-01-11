// The temporal super sampler renders a high-resolution framebuffer
// with dimensions (W<<N,H<<N) where N is the upsampling factor.
// This framebuffer is linearly downsampled and blitted to the window.
// The rendering is done over several frames where each frame is assigned
// to a regular sampling of the high-resolution buffer. Here for example
// is a 3x2 framebuffer supersampled to 6x4.
//                          -----------------
//                         | 1 2 | 1 2 | 1 2 |
//                         | 3 4 | 3 4 | 3 4 |
//                         |-----+-----+-----|
//                         | 1 2 | 1 2 | 1 2 |
//                         | 3 4 | 3 4 | 3 4 |
//                          -----------------
// Each number refers to the frame at which that pixel gets rendered.
// For example, in frame 1, only the upper-left pixels are rendered
// to a low-resolution framebuffer (here 3x2), and then distributed
// to the high-res framebuffer:
//                                   _________________
//                 ___________      | 1 # | 1 # | 1 # |
//                | 1 | 1 | 1 |     | # # | # # | # # |
//                | --+---+---| --> |-----+-----+-----|
//                | 1 | 1 | 1 |     | 1 # | 1 # | 1 # |
//                 -----------      | # # | # # | # # |
//                                   -----------------
// In this example, it would take 4 frames before the output is stable,
// assuming a static scene.
namespace render_scaler
{
    static render_texture_t output;
    static render_texture_t lowres;
    static int subpixel;
    static int upsample;
    static bool has_begun;
    void GetSubpixelOffsetI(int w, int h, int n, int *idx, int *idy)
    {
        // todo: nicer sampling pattern for any upsampling factor
        int i = subpixel;
        if (n == 2)
        {
            static const int remap[] = {10,5,16,14,12,2,1,11,3,7,9,15,4,8,13,6};
            i = remap[subpixel]-1;
        }
        else if (n == 3)
        {
            static const int remap[] = {42, 3, 8, 34, 18, 38, 25, 61, 16, 48, 56, 7, 55, 32, 59, 41, 4, 36, 30, 1, 24, 50, 0, 12, 39, 45, 31, 10, 58, 14, 23, 43, 46, 44, 9, 40, 28, 6, 17, 15, 60, 27, 37, 52, 20, 53, 19, 62, 47, 33, 63, 13, 5, 49, 54, 26, 35, 22, 21, 57, 29, 2, 51, 11};
            i = remap[subpixel];
        }
        *idx = i % (1<<n);
        *idy = i / (1<<n);
    }
    // dx,dy = pixel offset to apply to projection matrix element (0,2) and (1,2)
    // . . dx 0
    // . . dy 0
    // . .  . .
    // . .  . 1
    // Note: the result is divided by z afterwards.
    void GetSubpixelOffset(int w, int h, int n, float *dx, float *dy)
    {
        int idx,idy;
        GetSubpixelOffsetI(w,h,n,&idx,&idy);
        float pixel_width = 2.0f/(w<<n); // width in NDC [-1,+1] space
        float pixel_height = 2.0f/(h<<n); // width in NDC [-1,+1] space
        int num_samples_x = 1 << n;
        *dx = (-0.5f*num_samples_x + 0.5f + idx)*pixel_width;
        *dy = (-0.5f*num_samples_x + 0.5f + idy)*pixel_height;
    }
    void Begin(int w, int h, int n)
    {
        using namespace render_texture;

        has_begun = true;
        if (lowres.width != w || lowres.height != h)
        {
            FreeRenderTexture(&lowres);
            lowres = MakeRenderTexture(w, h, GL_NEAREST, GL_NEAREST, true);
            subpixel = 0;
        }
        if (output.width != w<<n || output.height != h<<n)
        {
            FreeRenderTexture(&output);
            output = MakeRenderTexture(w<<n, h<<n, GL_NEAREST, GL_NEAREST, false);
            subpixel = 0;
        }
        upsample = n;
        EnableRenderTexture(&lowres);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    void End()
    {
        using namespace render_texture;

        static GLuint program = 0;
        static GLint attrib_position = 0;
        static GLint uniform_sampler0 = 0;
        static GLint uniform_dx = 0;
        static GLint uniform_dy = 0;
        static GLint uniform_tile_dim = 0;
        if (!program)
        {
            #define SHADER(S) "#version 150\n" #S
            const char *vs = SHADER(
            in vec2 position;
            out vec2 texel;
            void main()
            {
                texel = vec2(0.5) + 0.5*position;
                gl_Position = vec4(position, 0.0, 1.0);
            }
            );

            const char *fs = SHADER(
            in vec2 texel;
            uniform sampler2D sampler0;
            uniform float dx;
            uniform float dy;
            uniform float tile_dim;
            out vec4 out_color;
            void main()
            {
                out_color = texture(sampler0, texel);
                float ix = trunc(mod(gl_FragCoord.x,tile_dim));
                float iy = trunc(mod(gl_FragCoord.y,tile_dim));
                if (ix == dx && iy == dy)
                    out_color = texture(sampler0,texel);
                else
                    discard;
            }
            );
            #undef SHADER

            program = LoadShaderFromMemory(vs,fs);
            attrib_position = glGetAttribLocation(program, "position");
            uniform_sampler0 = glGetUniformLocation(program, "sampler0");
            uniform_dx = glGetUniformLocation(program, "dx");
            uniform_dy = glGetUniformLocation(program, "dy");
            uniform_tile_dim = glGetUniformLocation(program, "tile_dim");
            assert(attrib_position >= 0 && "Unused or nonexistent attribute");
            assert(uniform_sampler0 >= 0 && "Unused or nonexistent uniform");
            assert(uniform_dx >= 0 && "Unused or nonexistent uniform");
            assert(uniform_dy >= 0 && "Unused or nonexistent uniform");
            assert(uniform_tile_dim >= 0 && "Unused or nonexistent uniform");
            assert(program && "Failed to compile TemporalBlend shader");
        }

        has_begun = false;
        DisableRenderTexture(&lowres);

        imm_gl_state_t last_state = GetImmediateGLState();
        float last_projection[4*4];
        vdbGetProjection(last_projection);
        vdbProjection(NULL);
        vdbPushMatrix();
        vdbLoadMatrix(NULL);
        vdbBlendNone();
        vdbCullFace(false);
        vdbDepthTest(false);
        vdbDepthWrite(false);

        // interleave just-rendered frame into full resolution framebuffer
        {
            int idx,idy;
            GetSubpixelOffsetI(lowres.width,lowres.height,upsample,&idx,&idy);

            static GLuint vao = 0;
            static GLuint vbo = 0;
            if (!vao)
            {
                static float position[] = { -1,-1, 1,-1, 1,1, 1,1, -1,1, -1,-1 };
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
            }
            assert(vao);
            assert(vbo);

            EnableRenderTexture(&output);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glUseProgram(program);
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(uniform_sampler0, 0);
            glBindTexture(GL_TEXTURE_2D, lowres.color[0]);
            glUniform1f(uniform_dx, (float)idx);
            glUniform1f(uniform_dy, (float)idy);
            glUniform1f(uniform_tile_dim, (float)(1<<upsample));
            glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(attrib_position);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(attrib_position);
            glUseProgram(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            DisableRenderTexture(&output);
        }

        // draw full resolution framebuffer onto window framebuffer
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, output.color[0]);
            vdbTriangles();
            vdbColor(1,1,1,1);
            vdbTexel(0,0); vdbVertex(-1,-1);
            vdbTexel(1,0); vdbVertex(+1,-1);
            vdbTexel(1,1); vdbVertex(+1,+1);
            vdbTexel(1,1); vdbVertex(+1,+1);
            vdbTexel(0,1); vdbVertex(-1,+1);
            vdbTexel(0,0); vdbVertex(-1,-1);
            vdbEnd();
        }

        SetImmediateGLState(last_state);
        vdbPopMatrix();
        vdbProjection(last_projection);

        // just a linear sampling order. want something nicer in the future
        int num_subpixels = (1<<upsample)*(1<<upsample);
        subpixel = (subpixel+1)%num_subpixels;
    }
}
