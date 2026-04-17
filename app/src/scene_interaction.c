#include "scene_internal.h"

int is_entity_collidable(const Entity* e)
{
    if (e == NULL) return 0;
    if (strcmp(e->type, "painting") == 0) return 0;
    if (strcmp(e->type, "lamp") == 0) return 0;
    return 1;
}

void resolve_camera_collisions(const Scene* scene, Camera* camera)
{
    if (scene == NULL || camera == NULL) return;

    /* Approximate the camera as a vertical capsule. */
    const float cam_r = 0.25f;
    const float cam_h = 1.70f;

    const float cam_z_max = (float)camera->position.z;
    const float cam_z_min = cam_z_max - cam_h;

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];
        if (!is_entity_collidable(e)) continue;

        const float base_z = e->pz + e->ground_offset_z;
        const float e_z_min = base_z + e->bounds_min_z_local * e->sz;
        const float e_z_max = base_z + e->bounds_max_z_local * e->sz;

        if (e_z_max < cam_z_min || e_z_min > cam_z_max) {
            continue;
        }

        /* Horizontal collision: circle vs. oriented rectangle. */

        const float local_cx = (e->bounds_min_x_local + e->bounds_max_x_local) * 0.5f;
        const float local_cy = (e->bounds_min_y_local + e->bounds_max_y_local) * 0.5f;
        const float half_x = (e->bounds_max_x_local - e->bounds_min_x_local) * 0.5f * e->sx;
        const float half_y = (e->bounds_max_y_local - e->bounds_min_y_local) * 0.5f * e->sy;

        const float box_cx = e->px + local_cx * e->sx;
        const float box_cy = e->py + local_cy * e->sy;

        const float px = (float)camera->position.x - box_cx;
        const float py = (float)camera->position.y - box_cy;

        const float a = (float)(e->rz * (M_PI / 180.0));
        const float ca = cosf(a);
        const float sa = sinf(a);

        const float lx =  ca * px + sa * py;
        const float ly = -sa * px + ca * py;

        float qx = lx;
        float qy = ly;
        if (qx < -half_x) qx = -half_x;
        if (qx >  half_x) qx =  half_x;
        if (qy < -half_y) qy = -half_y;
        if (qy >  half_y) qy =  half_y;

        float dx = lx - qx;
        float dy = ly - qy;
        float d2 = dx*dx + dy*dy;

        if (d2 < cam_r * cam_r) {
            float push_lx = 0.0f;
            float push_ly = 0.0f;

            if (d2 < 1e-8f) {
                const float pen_x = half_x - fabsf(lx);
                const float pen_y = half_y - fabsf(ly);
                if (pen_x < pen_y) {
                    const float target = (lx >= 0.0f) ? (half_x + cam_r) : -(half_x + cam_r);
                    push_lx = target - lx;
                } else {
                    const float target = (ly >= 0.0f) ? (half_y + cam_r) : -(half_y + cam_r);
                    push_ly = target - ly;
                }
            } else {
                const float d = sqrtf(d2);
                const float nx = dx / d;
                const float ny = dy / d;
                const float push = (cam_r - d) + 0.0001f;
                push_lx = nx * push;
                push_ly = ny * push;
            }

            const float wx = ca * push_lx - sa * push_ly;
            const float wy = sa * push_lx + ca * push_ly;
            camera->position.x += wx;
            camera->position.y += wy;
        }
    }

    clamp_camera_to_room(camera);
}

int invert_matrix_4x4(const double m[16], double inv_out[16])
{
    double inv[16];

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    double det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabs(det) < 1e-12) {
        return 0;
    }

    det = 1.0 / det;
    for (int i = 0; i < 16; i++) inv_out[i] = inv[i] * det;
    return 1;
}

void mult_mat4_vec4(const double m[16], const double v[4], double out[4])
{
    out[0] = m[0]*v[0] + m[4]*v[1] + m[8]*v[2]  + m[12]*v[3];
    out[1] = m[1]*v[0] + m[5]*v[1] + m[9]*v[2]  + m[13]*v[3];
    out[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3];
    out[3] = m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3];
}

void mult_mat4_mat4(const double a[16], const double b[16], double out[16])
{
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out[col*4 + row] =
                a[0*4 + row] * b[col*4 + 0] +
                a[1*4 + row] * b[col*4 + 1] +
                a[2*4 + row] * b[col*4 + 2] +
                a[3*4 + row] * b[col*4 + 3];
        }
    }
}

void vec3_sub(const double a[3], const double b[3], double out[3])
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

void vec3_norm(double v[3])
{
    const double len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len > 1e-12) { v[0] /= len; v[1] /= len; v[2] /= len; }
}

void rotate_point_xyz_deg(double p[3], float rx, float ry, float rz)
{
    const double x0 = p[0], y0 = p[1], z0 = p[2];
    double x = x0, y = y0, z = z0;

    const double ax = degree_to_radian(rx);
    const double ay = degree_to_radian(ry);
    const double az = degree_to_radian(rz);

    {
        const double cy = cos(ax), sy = sin(ax);
        const double y1 = y*cy - z*sy;
        const double z1 = y*sy + z*cy;
        y = y1; z = z1;
    }
    {
        const double cx = cos(ay), sx = sin(ay);
        const double x1 = x*cx + z*sx;
        const double z1 = -x*sx + z*cx;
        x = x1; z = z1;
    }
    {
        const double cz = cos(az), sz = sin(az);
        const double x1 = x*cz - y*sz;
        const double y1 = x*sz + y*cz;
        x = x1; y = y1;
    }

    p[0] = x; p[1] = y; p[2] = z;
}

int ray_sphere_intersect(const double ro[3], const double rd[3],
                                const double c[3], double r,
                                double* out_t)
{
    const double ocx = ro[0] - c[0];
    const double ocy = ro[1] - c[1];
    const double ocz = ro[2] - c[2];
    const double b = ocx*rd[0] + ocy*rd[1] + ocz*rd[2];
    const double cterm = ocx*ocx + ocy*ocy + ocz*ocz - r*r;
    const double disc = b*b - cterm;
    if (disc < 0.0) return 0;
    const double s = sqrt(disc);
    double t = -b - s;
    if (t < 0.0) t = -b + s;
    if (t < 0.0) return 0;
    if (out_t) *out_t = t;
    return 1;
}

int pick_entity(Scene* scene, const Camera* camera,
                int mouse_x, int mouse_y,
                int viewport_x, int viewport_y, int viewport_w, int viewport_h)
{
    if (!scene || !camera) return -1;
    if (viewport_w <= 0 || viewport_h <= 0) return -1;

    if (mouse_x < viewport_x || mouse_x >= viewport_x + viewport_w ||
        mouse_y < viewport_y || mouse_y >= viewport_y + viewport_h) {
        scene->selected_entity = -1;
        return -1;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    const double aspect = (double)viewport_w / (double)viewport_h;
    const double n = 0.10;
    const double f = 200.0;
    const double t = 0.08;
    const double b = -0.08;
    const double r = t * aspect;
    const double l = -r;
    glFrustum(l, r, b, t, n, f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    set_view(camera);

    double proj[16], mv[16], mvp[16], inv_mvp[16];
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    mult_mat4_mat4(proj, mv, mvp);
    if (!invert_matrix_4x4(mvp, inv_mvp)) {
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        return -1;
    }

    const double x_ndc = (2.0 * (double)(mouse_x - viewport_x) / (double)viewport_w) - 1.0;
    const double y_ndc = 1.0 - (2.0 * (double)(mouse_y - viewport_y) / (double)viewport_h);

    double v_near[4] = { x_ndc, y_ndc, -1.0, 1.0 };
    double v_far[4]  = { x_ndc, y_ndc,  1.0, 1.0 };

    double w_near[4], w_far[4];
    mult_mat4_vec4(inv_mvp, v_near, w_near);
    mult_mat4_vec4(inv_mvp, v_far,  w_far);
    if (fabs(w_near[3]) < 1e-12 || fabs(w_far[3]) < 1e-12) {
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        return -1;
    }

    const double p0[3] = { w_near[0]/w_near[3], w_near[1]/w_near[3], w_near[2]/w_near[3] };
    const double p1[3] = { w_far[0]/w_far[3],  w_far[1]/w_far[3],  w_far[2]/w_far[3] };

    double ro[3] = { p0[0], p0[1], p0[2] };
    double rd[3];
    vec3_sub(p1, p0, rd);
    vec3_norm(rd);

    int best_i = -1;
    double best_t = 1e30;

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];

        double c_local[3] = { e->bounds_center_local.x, e->bounds_center_local.y, e->bounds_center_local.z };
        c_local[0] *= e->sx; c_local[1] *= e->sy; c_local[2] *= e->sz;
        rotate_point_xyz_deg(c_local, e->rx, e->ry, e->rz);
        const double c_world[3] = { c_local[0] + e->px, c_local[1] + e->py, c_local[2] + e->pz };

        const double smax = fmax(fmax(fabs(e->sx), fabs(e->sy)), fabs(e->sz));
        const double r_world = (double)e->bounds_radius_local * smax;

        double t_hit;
        if (ray_sphere_intersect(ro, rd, c_world, r_world, &t_hit)) {
            if (t_hit < best_t) {
                best_t = t_hit;
                best_i = i;
            }
        }
    }

    scene->selected_entity = best_i;

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (best_i >= 0) {
        printf("[PICK] selected: #%d (%s)\n", best_i, scene->entities[best_i].type);
    }
    return best_i;
}

