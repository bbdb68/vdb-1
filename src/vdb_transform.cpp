#include "_matrix.cpp"

namespace transform
{
    static vdbMat4 projection = vdbMatIdentity();
    static vdbMat4 view_model = vdbMatIdentity();
    static vdbMat4 pvm = vdbMatIdentity();
    int viewport_left;
    int viewport_bottom;
    int viewport_width;
    int viewport_height;

    void Reset();
}

#if VDB_USE_FIXED_FUNCTION_PIPELINE==1
// This path uses the fixed-function pipeline of legacy OpenGL.
// It is available only in compatibility profiles of OpenGL, which
// itself is not available on certain drivers (Mesa, for one).

void transform::Reset()
{
    projection = vdbMatIdentity();
    view_model = vdbMatIdentity();
    pvm = vdbMatIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    CheckGLError();
}

void vdbProjection(float *m)
{
    glMatrixMode(GL_PROJECTION);
    if (m)
    {
        transform::projection = *(vdbMat4*)m;
        #ifdef VDB_MATRIX_ROW_MAJOR
        glLoadMatrixf(m);
        #else
        glLoadTransposeMatrixf(m);
        #endif
    }
    else
    {
        transform::projection = vdbMatIdentity();
        glLoadIdentity();
    }
    transform::pvm = vdbMul4x4(transform::projection, transform::view_model);
}

void vdbGetProjection(float *m)
{
    assert(m);
    glGetFloatv(GL_PROJECTION_MATRIX, m);
}

void vdbGetPVM(float *m)
{
    assert(m);
    *(vdbMat4*)m = transform::pvm;
}

void vdbGetMatrix(float *m)
{
    assert(m && "pointer passed to vdbGetMatrix was NULL");
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
}

void vdbMultMatrix(float *m)
{
    assert(m && "pointer passed to vdbMultMatrix was NULL");
    if (m)
    {
        glMatrixMode(GL_MODELVIEW);
        #ifdef VDB_MATRIX_ROW_MAJOR
        glMultMatrixf(m);
        glGetFloatv(GL_MODELVIEW_MATRIX, (float*)transform::view_model.data);
        #else
        glMultTransposeMatrixf(m);
        glGetFloatv(GL_TRANSPOSE_MODELVIEW_MATRIX, (float*)transform::view_model.data);
        #endif
        transform::pvm = vdbMul4x4(transform::projection, transform::view_model);
    }
}

void vdbLoadMatrix(float *m)
{
    glMatrixMode(GL_MODELVIEW);
    if (m)
    {
        #ifdef VDB_MATRIX_ROW_MAJOR
        glLoadMatrixf(m);
        glGetFloatv(GL_MODELVIEW_MATRIX, (float*)transform::view_model.data);
        #else
        glLoadTransposeMatrixf(m);
        glGetFloatv(GL_TRANSPOSE_MODELVIEW_MATRIX, (float*)transform::view_model.data);
        #endif
    }
    else
    {
        transform::view_model = vdbMatIdentity();
        glLoadIdentity();
    }
    transform::pvm = vdbMul4x4(transform::projection, transform::view_model);
}

static int vdb_push_pop_matrix_index = 0;

void vdbPushMatrix()
{
    vdb_push_pop_matrix_index++;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
}

void vdbPopMatrix()
{
    vdb_push_pop_matrix_index--;
    assert(vdb_push_pop_matrix_index >= 0 && "Mismatched vdb{Push/Pop}Matrix calls");
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    #ifdef VDB_MATRIX_ROW_MAJOR
    glGetFloatv(GL_MODELVIEW_MATRIX, (float*)transform::view_model.data);
    #else
    glGetFloatv(GL_TRANSPOSE_MODELVIEW_MATRIX, (float*)transform::view_model.data);
    #endif
    transform::pvm = vdbMul4x4(transform::projection, transform::view_model);
}

#else
#include "_matrix_stack.cpp"

namespace transform
{
    static matrix_stack_t matrix_stack = {0};
}

void transform::Reset()
{
    projection = vdbMatIdentity();
    view_model = vdbMatIdentity();
    pvm = vdbMatIdentity();
    matrix_stack.Reset();
}

void vdbProjection(float *m)
{
    using namespace transform;
    if (m)
        projection = *(vdbMat4*)m;
    else
        projection = vdbMatIdentity();
    pvm = vdbMul4x4(projection, view_model);
}

void vdbGetProjection(float *m)
{
    assert(m);
    *(vdbMat4*)m = transform::projection;
}

void vdbGetPVM(float *m)
{
    assert(m);
    *(vdbMat4*)m = transform::pvm;
}

void vdbGetMatrix(float *m)
{
    assert(m);
    *(vdbMat4*)m = transform::view_model;
}

void vdbMultMatrix(float *m)
{
    using namespace transform;
    if (m)
    {
        matrix_stack.Multiply(*(vdbMat4*)m);
        view_model = matrix_stack.Top();
        pvm = vdbMul4x4(projection, view_model);
    }
}

void vdbLoadMatrix(float *m)
{
    using namespace transform;
    if (m)
        matrix_stack.Load(*(vdbMat4*)m);
    else
        matrix_stack.LoadIdentity();
    view_model = matrix_stack.Top();
    pvm = vdbMul4x4(projection, view_model);
}

void vdbPushMatrix()
{
    using namespace transform;
    matrix_stack.Push();
    view_model = matrix_stack.Top();
}

void vdbPopMatrix()
{
    using namespace transform;
    matrix_stack.Pop();
    view_model = matrix_stack.Top();
    pvm = vdbMul4x4(projection, view_model);
}

#endif

void vdbTranslate(float x, float y, float z) { vdbMultMatrix(vdbMatTranslate(x,y,z).data); }
void vdbRotateXYZ(float x, float y, float z) { vdbMultMatrix(vdbMatRotateXYZ(x,y,z).data); }
void vdbRotateZYX(float z, float y, float x) { vdbMultMatrix(vdbMatRotateZYX(z,y,x).data); }

int vdbGetWindowWidth() { return window::window_width; }
int vdbGetWindowHeight() { return window::window_height; }

int vdbGetFramebufferWidth()
{
    using namespace render_texture;
    if (current_render_texture)
        return current_render_texture->width;
    else return window::framebuffer_width;
}

int vdbGetFramebufferHeight()
{
    using namespace render_texture;
    if (current_render_texture)
        return current_render_texture->height;
    else return window::framebuffer_height;
}

float vdbGetAspectRatio()
{
    return (float)vdbGetFramebufferWidth()/vdbGetFramebufferHeight();
}

void vdbViewporti(int left, int bottom, int width, int height)
{
    glViewport(left, bottom, (GLsizei)width, (GLsizei)height);
    transform::viewport_left = left;
    transform::viewport_bottom = bottom;
    transform::viewport_width = width;
    transform::viewport_height = height;
}

void vdbViewport(float left, float bottom, float width, float height)
{
    int fb_width = vdbGetFramebufferWidth();
    int fb_height = vdbGetFramebufferHeight();
    vdbViewporti((int)(left*fb_width), (int)(bottom*fb_height),
                 (int)(width*fb_width), (int)(height*fb_height));
}

void vdbOrtho(float x_left, float x_right, float y_bottom, float y_top)
{
    vdbMat4 p = vdbMatIdentity();
    p(0,0) = 2.0f/(x_right-x_left);
    p(0,3) = (x_left+x_right)/(x_left-x_right);
    p(1,1) = 2.0f/(y_top-y_bottom);
    p(1,3) = (y_bottom+y_top)/(y_bottom-y_top);
    vdbProjection(p.data);
}

void vdbOrtho(float x_left, float x_right, float y_bottom, float y_top, float z_near, float z_far)
{
    assert(false && "not implemented yet");
    vdbMat4 p = vdbMatIdentity();
    vdbProjection(p.data);
}

void vdbPerspective(float yfov, float z_near, float z_far, float x_offset, float y_offset)
{
    float t = 1.0f/tanf(yfov/2.0f);
    vdbMat4 p = {0};
    p(0,0) = t/(vdbGetAspectRatio());
    p(0,2) = x_offset;
    p(1,1) = t;
    p(1,2) = y_offset;
    p(2,2) = (z_near+z_far)/(z_near-z_far);
    p(3,2) = -1.0f;
    p(2,3) = 2.0f*z_near*z_far/(z_near-z_far);
    vdbProjection(p.data);
}

vdbVec2 vdbWindowToNDC(float xw, float yw)
{
    using namespace window;
    using namespace transform;
    float xf = framebuffer_width*(xw/window_width);
    float yf = framebuffer_height*(1.0f - yw/window_height);
    float xn = -1.0f+2.0f*(xf-viewport_left)/viewport_width;
    float yn = -1.0f+2.0f*(yf-viewport_bottom)/viewport_height;
    vdbVec2 result(xn,yn);
    return result;
}

vdbVec2 vdbNDCToWindow(float xn, float yn)
{
    using namespace window;
    using namespace transform;
    float xf = viewport_left + (0.5f+0.5f*xn)*viewport_width;
    float yf = viewport_bottom + (0.5f+0.5f*yn)*viewport_height;
    float xw = window_width*(xf/framebuffer_width);
    float yw = window_height*(1.0f - yf/framebuffer_height);
    vdbVec2 result(xw,yw);
    return result;
}

vdbVec3 vdbNDCToModel(float x_ndc, float y_ndc, float depth)
{
    using namespace transform;

    // assuming projection is of the form
    // ax       bx
    //    ay    by
    //       az bz
    //       cw aw
    // (e.g. orthographic or perspective transform)

    // also assumes modelview matrix is SE3
    // (e.g. rotation and translation only)

    float ax = projection(0,0);
    float ay = projection(1,1);
    // float az = projection(2,2);
    float bx = projection(0,3);
    float by = projection(1,3);
    // float bz = projection(2,3);
    float cw = projection(3,2);
    float aw = projection(3,3);

    float w_clip = cw*depth + aw;
    float x_clip = x_ndc*w_clip;
    float y_clip = y_ndc*w_clip;
    // float z_clip = az*depth + bz;
    vdbVec4 view(0,0,0,0);
    view.x = (x_clip-bx)/ax;
    view.y = (y_clip-by)/ay;
    view.z = depth;
    view.w = 1.0f;
    vdbVec4 model = vdbMulSE3Inverse(view_model, view);
    vdbVec3 result(model.x,model.y,model.z);
    return result;
}

vdbVec2 vdbModelToNDC(float x, float y, float z, float w)
{
    vdbVec4 model(x,y,z,w);
    vdbVec4 clip = vdbMul4x1(transform::pvm, model);
    vdbVec2 ndc(clip.x/clip.w, clip.y/clip.w);
    return ndc;
}
