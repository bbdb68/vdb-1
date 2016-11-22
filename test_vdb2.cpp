#include "vdb2.h"

int main()
{
    struct Thing
    {
        float error;
        int count;
    };

    #define N 32
    Thing things[N*N];
    vdb_for(x, 0, N)
    vdb_for(y, 0, N)
    {
        things[y*N+x].count = y*N+x;
        things[y*N+x].error = x*y;
    }

    VDBB("my label");
    {
        vdbView(mat_perspective(SO_PI/4.0f, vdb_input.width, vdb_input.height, 0.01f, 10.0f),
                vdbCamera3D(vdb_input), m_id4());
        glBegin(GL_TRIANGLES);
        {
            int nx = (int)(vdb_input.width/64.0f);
            int ny = (int)(vdb_input.height/64.0f);
            vdb_for(yi, 0, ny+1)
            vdb_for(xi, 0, nx+1)
            {
                float xt = (float)xi/nx;
                float yt = (float)yi/ny;
                float xn = -1.0f + 2.0f*xt;
                float yn = -1.0f + 2.0f*yt;
                float dx = 2.0f/nx;
                float dy = 2.0f/ny;
                glColor4f(0.2f+0.8f*xt, 0.3f+0.7f*yt, 0.5f+0.5f*sinf(0.3f*vdb_input.t), 1.0f);
                glVertex2f(xn, yn);
                glVertex2f(xn+dx, yn);
                glVertex2f(xn+dx, yn+dy);
                glVertex2f(xn+dx, yn+dy);
                glVertex2f(xn, yn+dy);
                glVertex2f(xn, yn);

                vdbNote(xn+1.0f/nx, yn+1.0f/ny, "%.2f", xn);
                if (vdbMap(xn+1.0f/nx, yn+1.0f/ny))
                {
                    SetTooltip("%.2f %.2f", xn+1.0f/nx, yn+1.0f/ny);
                }
            }
        }
        glEnd();

        {
            int i;
            float x_src, y_src;
            vdbUnmap(&i, &x_src, &y_src);
            // SetTooltip("%d: %.2f %.2f", i, x_src, y_src);

            float x_win, y_win;
            vdbModelToWindow(x_src, y_src, 0.0f, 1.0f, &x_win, &y_win);
            vdbOrtho(0.0f, vdb_input.width, vdb_input.height, 0.0f);
            glBegin(GL_TRIANGLES);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            vdbFillCircle(x_win+1.0f, y_win+1.0f, 5.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            vdbFillCircle(x_win, y_win, 5.0f);
            glEnd();
        }
    }
    VDBE();

    for (int i = 0; i < 10; i++)
    {
        VDBB("Testfor");
        {
            glClearColor(i/10.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        VDBE();
    }

    VDBB("Ho");
    {
        vdbOrtho(-1.0f, +1.0f, -1.0f, +1.0f);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        {
            mat3 R = m_mat3(mat_rotate_z(0.4f)*mat_rotate_y(0.4f)*mat_rotate_x(0.4f));
            int n = 16;
            vdb_for(zi, 0, n+1)
            vdb_for(yi, 0, n+1)
            vdb_for(xi, 0, n+1)
            {
                float xt = (float)xi/n;
                float yt = (float)yi/n;
                float zt = (float)zi/n;
                int it = xi*n*n+yi*n+zi;
                float offx = sinf(3.0f*vdb_input.t+0.0f+it)*(-1.0f+2.0f*noise_hash1f(it))/(4.0f*n);
                float offy = sinf(3.0f*vdb_input.t+1.5f+it)*(-1.0f+2.0f*noise_hash1f(it))/(4.0f*n);
                float offz = sinf(3.0f*vdb_input.t+3.1f+it)*(-1.0f+2.0f*noise_hash1f(it))/(4.0f*n);
                vec3 pm = m_normalize(m_vec3(-1.0f+2.0f*xt, -1.0f+2.0f*yt, -1.0f+2.0f*zt));
                pm += m_vec3(offx, offy, offz);
                vec3 pc = R*pm + m_vec3(0.0f, 0.0f, -5.0f);
                float f = (vdb_input.height/2.0f) / tanf(SO_PI/8.0f);
                vec2 uv = m_project_pinhole(f, f, vdb_input.width/2.0f, vdb_input.height/2.0f, pc);
                float xn = -1.0f + 2.0f*uv.x/vdb_input.width;
                float yn = -1.0f + 2.0f*uv.y/vdb_input.height;
                glColor4f(0.9f+0.1f*xt, 0.8f+0.2f*yt, 0.85f+0.15f*zt, 1.0f);
                glVertex2f(xn, yn);

                vdbMap(xn, yn);
            }
        }
        glEnd();

        {
            int i;
            float x_src, y_src;
            vdbUnmap(&i, &x_src, &y_src);
            SetTooltip("%d: %.2f %.2f", i, x_src, y_src);

            float x_win, y_win;
            vdbModelToWindow(x_src, y_src, 0.0f, 1.0f, &x_win, &y_win);
            vdbOrtho(0.0f, vdb_input.width, vdb_input.height, 0.0f);
            glBegin(GL_TRIANGLES);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            vdbFillCircle(x_win+1.0f, y_win+1.0f, 5.0f);
            glColor4f(1.0f, 1.0f, 0.3f, 1.0f);
            vdbFillCircle(x_win, y_win, 5.0f);
            glEnd();
        }
    }
    VDBE();
}
