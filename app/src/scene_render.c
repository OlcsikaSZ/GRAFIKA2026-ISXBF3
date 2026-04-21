#include "scene_internal.h"

// Apply an entity transform before drawing its model.
void apply_transform(const Entity* e)
{
    glTranslatef(e->px, e->py, e->pz + e->ground_offset_z);

    glRotatef(e->rx, 1, 0, 0);
    glRotatef(e->ry, 0, 1, 0);
    glRotatef(e->rz, 0, 0, 1);

    glScalef(e->sx, e->sy, e->sz);
}

// Draw a soft circular proxy under an exhibit shadow.
void draw_shadow_proxy_circle(const Entity* e)
{
    // Draw a simple proxy shape for the shadow pass.
    const int SEG = 24;
    const float r = e->bounds_radius_local;
    const float cx = e->bounds_center_local.x;
    const float cy = e->bounds_center_local.y;
    const float z  = e->bounds_min_z_local + 0.002f;

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx, cy, z);
    for (int i = 0; i <= SEG; i++) {
        const float a = (float)i * (2.0f * (float)M_PI / (float)SEG);
        glVertex3f(cx + cosf(a) * r, cy + sinf(a) * r, z);
    }
    glEnd();
}

// Configure the OpenGL lights from the current scene state.
void set_lighting_with_intensity(const Scene* scene)
{
    // Update the scene lights from the current intensity.
    const float intensity = scene->light_intensity;
    float t = intensity / 3.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const float amb = 0.12f + 0.18f * t;
    float ambient_light[]  = { amb, amb, amb, 1.0f };

    const float d = 0.95f * t;
    float diffuse_light[]  = { d, d, d, 1.0f };

    const float s = 0.25f * t;
    float specular_light[] = { s, s, s, 1.0f };

    {
        const float g = 0.12f + 0.10f * t;
        float globalAmb[] = { g, g, g, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    }

    float lamp_pos[3][4];
    int lamp_count = 0;
    for (int i = 0; i < scene->entity_count && lamp_count < 3; i++) {
        const Entity* e = &scene->entities[i];
        if (strcmp(e->type, "lamp") == 0) {
            lamp_pos[lamp_count][0] = e->px;
            lamp_pos[lamp_count][1] = e->py;
            lamp_pos[lamp_count][2] = e->pz - 0.20f;
            lamp_pos[lamp_count][3] = 1.0f;
            lamp_count++;
        }
    }
    if (lamp_count == 0) {
        lamp_pos[0][0] = 0.0f;
        lamp_pos[0][1] = 0.0f;
        lamp_pos[0][2] = (ROOM_H - 0.25f);
        lamp_pos[0][3] = 1.0f;
        lamp_count = 1;
    }

    static const GLenum lights[3] = { GL_LIGHT0, GL_LIGHT1, GL_LIGHT2 };

    for (int li = 0; li < 3; li++) {
        const GLenum L = lights[li];
        if (li < lamp_count) {
            glEnable(L);
            glLightfv(L, GL_AMBIENT,  ambient_light);
            glLightfv(L, GL_DIFFUSE,  diffuse_light);
            glLightfv(L, GL_SPECULAR, specular_light);
            glLightfv(L, GL_POSITION, lamp_pos[li]);

            glLightf(L, GL_SPOT_CUTOFF, 180.0f);
            glLightf(L, GL_SPOT_EXPONENT, 0.0f);

            glLightf(L, GL_CONSTANT_ATTENUATION,  0.8f);
            glLightf(L, GL_LINEAR_ATTENUATION,    0.02f);
            glLightf(L, GL_QUADRATIC_ATTENUATION, 0.002f);
} else {
            glDisable(L);
        }
    }
}

// Upload the shared material parameters to OpenGL.
void set_material(const Material* material)
{
    float ambient_material_color[] = {
        material->ambient.red,
        material->ambient.green,
        material->ambient.blue,
        1.0f
    };

    float diffuse_material_color[] = {
        material->diffuse.red,
        material->diffuse.green,
        material->diffuse.blue,
        1.0f
    };

    float specular_material_color[] = {
        material->specular.red,
        material->specular.green,
        material->specular.blue,
        1.0f
    };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_material_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_material_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_material_color);

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float)material->shininess);
}

void build_shadow_matrix(float out[16], const float plane[4], const float light[4])
{
    // Build the matrix that projects geometry onto the floor plane.
    const float dot =
        plane[0] * light[0] +
        plane[1] * light[1] +
        plane[2] * light[2] +
        plane[3] * light[3];

    out[0]  = dot - light[0] * plane[0];
    out[4]  =     - light[0] * plane[1];
    out[8]  =     - light[0] * plane[2];
    out[12] =     - light[0] * plane[3];

    out[1]  =     - light[1] * plane[0];
    out[5]  = dot - light[1] * plane[1];
    out[9]  =     - light[1] * plane[2];
    out[13] =     - light[1] * plane[3];

    out[2]  =     - light[2] * plane[0];
    out[6]  =     - light[2] * plane[1];
    out[10] = dot - light[2] * plane[2];
    out[14] =     - light[2] * plane[3];

    out[3]  =     - light[3] * plane[0];
    out[7]  =     - light[3] * plane[1];
    out[11] =     - light[3] * plane[2];
    out[15] = dot - light[3] * plane[3];
}

// Return whether an entity should appear in the shadow pass.
int entity_casts_shadow(const Entity* e)
{
    if (strcmp(e->type, "painting") == 0) return 0;
    if (strcmp(e->type, "plane") == 0) return 0;
    if (strcmp(e->type, "lamp") == 0) return 0;
    if (strcmp(e->type, "case_glass") == 0) return 0;
    return 1;
}

// Return whether an entity must be rendered in the glass pass.
int entity_is_transparent(const Entity* e)
{
    return (strcmp(e->type, "case_glass") == 0);
}

// Draw an opaque entity in the main geometry pass.
void draw_entity_opaque(const Entity* e)
{
    // Restore the default state before drawing opaque geometry.
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPushMatrix();
    apply_transform(e);
    glBindTexture(GL_TEXTURE_2D, e->texture_id);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Draw statues double-sided to avoid missing surfaces.
    GLboolean cull_was_enabled = glIsEnabled(GL_CULL_FACE);
    if (strcmp(e->type, "statue") == 0) {
        glDisable(GL_CULL_FACE);
    }
    draw_model((Model*)&e->model);

    if (strcmp(e->type, "statue") == 0 && cull_was_enabled) {
        glEnable(GL_CULL_FACE);
    }
    glPopMatrix();
}

// Draw a transparent glass entity after opaque geometry.
void draw_entity_glass(const Entity* e)
{
    // Render glass after the opaque pass for correct blending.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);

    const float a = 0.18f;
    float amb[]  = { 0.10f, 0.10f, 0.12f, a };
    float dif[]  = { 0.22f, 0.24f, 0.26f, a };
    float spec[] = { 0.90f, 0.90f, 0.90f, 1.0f };
    float emi[]  = { 0.03f, 0.03f, 0.04f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 96.0f);

    glColor4f(1.0f, 1.0f, 1.0f, a);

    glPushMatrix();
    apply_transform(e);
    draw_model((Model*)&e->model);
    glPopMatrix();

    {
        float emi0[] = {0.0f, 0.0f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi0);
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// Render projected exhibit shadows onto the museum floor.
void render_planar_shadows(const Scene* scene)
{
    float lamp_pos[3][4];
    int lamp_count = 0;
    for (int i = 0; i < scene->entity_count && lamp_count < 3; i++) {
        const Entity* e = &scene->entities[i];
        if (strcmp(e->type, "lamp") == 0) {
            lamp_pos[lamp_count][0] = e->px;
            lamp_pos[lamp_count][1] = e->py;
            lamp_pos[lamp_count][2] = e->pz - 0.20f;
            lamp_pos[lamp_count][3] = 1.0f;
            lamp_count++;
        }
    }
    if (lamp_count == 0) {
        lamp_pos[0][0] = 0.0f;
        lamp_pos[0][1] = 0.0f;
        lamp_pos[0][2] = (ROOM_H - 0.25f);
        lamp_pos[0][3] = 1.0f;
        lamp_count = 1;
    }

    if (scene->light_intensity <= 0.001f) {
        return;
    }

    float t = scene->light_intensity / 3.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float alpha_base = (0.72f * t);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    glDepthMask(GL_FALSE);

    // Use one key light as the planar shadow source.

    int key = 0;
    float best_abs_y = fabsf(lamp_pos[0][1]);
    for (int li = 1; li < lamp_count; li++) {
        float ay = fabsf(lamp_pos[li][1]);
        if (ay < best_abs_y) { best_abs_y = ay; key = li; }
    }
    const float* key_light_pos = lamp_pos[key];


    const float* light_pos = key_light_pos;

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];
        if (!entity_casts_shadow(e)) continue;
        if (strcmp(e->type, "pedestal") == 0) continue;

        glColor4f(0.0f, 0.0f, 0.0f, alpha_base);

        {
            const float floor_plane[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
            float m[16];
            build_shadow_matrix(m, floor_plane, light_pos);
            glPushMatrix();
            glMultMatrixf(m);
            glTranslatef(0.0f, 0.0f, 0.0030f);
            apply_transform(e);
            if (e->model.n_vertices > 50000) {
                draw_shadow_proxy_circle(e);
            } else {
                draw_model((Model*)&e->model);
            }
            glPopMatrix();
        }
    }

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];
        if (!entity_casts_shadow(e)) continue;
        if (strcmp(e->type, "pedestal") != 0) continue;

        float pedestal_alpha = alpha_base * 1.10f;
        if (pedestal_alpha > 0.85f) pedestal_alpha = 0.85f;
        glColor4f(0.0f, 0.0f, 0.0f, pedestal_alpha);

        {
            const float floor_plane[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
            float m[16];
            build_shadow_matrix(m, floor_plane, light_pos);
            glPushMatrix();
            glMultMatrixf(m);
            glTranslatef(0.0f, 0.0f, 0.0030f);
            apply_transform(e);
            draw_model((Model*)&e->model);
            glPopMatrix();
        }
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
    glColor4f(1, 1, 1, 1);
}

// Render the room, entities, shadows, and selection feedback.
void render_scene(const Scene* scene)
{
    set_material(&scene->material);
    set_lighting_with_intensity(scene);

#ifdef SHOW_DEBUG_AXES
    draw_debug_axes_and_marker();
#endif

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);

    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    draw_room_world_quads(scene->floor_tex, scene->wall_tex, scene->ceiling_tex);

    if (scene->shadows_enabled) {
        render_planar_shadows(scene);
    }


    for (int i = 0; i < scene->entity_count; i++) {
        if (i == scene->selected_entity) continue;
        const Entity* e = &scene->entities[i];
        if (entity_is_transparent(e)) continue;
        draw_entity_opaque(e);
    }

    for (int i = 0; i < scene->entity_count; i++) {
        if (i == scene->selected_entity) continue;
        const Entity* e = &scene->entities[i];
        if (!entity_is_transparent(e)) continue;
        draw_entity_glass(e);
    }

    if (scene->selected_entity >= 0 && scene->selected_entity < scene->entity_count) {
        const Entity* e = &scene->entities[scene->selected_entity];

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        if (entity_is_transparent(e)) {
            draw_entity_glass(e);
        } else {
            draw_entity_opaque(e);
        }

        glStencilMask(0x00);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);

        if (!entity_is_transparent(e)) {
            glPushMatrix();
            apply_transform(e);
            glScalef(1.05f, 1.05f, 1.05f);
            glColor3f(1.0f, 0.85f, 0.20f);
            draw_model((Model*)&e->model);
            glPopMatrix();
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
}


// Draw a small origin marker for debugging.
void draw_origin(void)
{
    glBegin(GL_LINES);

    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);

    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);

    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);

    glEnd();
}

// Draw a simple debug plane grid.
void draw_plane(int n)
{
    glColor3f(1, 0, 0);
    glNormal3f(0, 0, 1);

    double step = 1.0 / n;
    for (int i = 0; i <= n; ++i) {
        double y = (double)i / n;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= n; ++j) {
            double x = (double)j / n;
            glVertex3f((float)x, (float)y, 0.0f);
            glVertex3f((float)x, (float)(y + step), 0.0f);
        }
        glEnd();
    }
}

void quad_world(float x1, float y1, float z1,
                       float x2, float y2, float z2,
                       float x3, float y3, float z3,
                       float x4, float y4, float z4,
                       float nx, float ny, float nz,
                       float u_rep, float v_rep)
{
    glBegin(GL_QUADS);
    glNormal3f(nx, ny, nz);

    glTexCoord2f(0.0f, 0.0f);    glVertex3f(x1, y1, z1);
    glTexCoord2f(u_rep, 0.0f);   glVertex3f(x2, y2, z2);
    glTexCoord2f(u_rep, v_rep);  glVertex3f(x3, y3, z3);
    glTexCoord2f(0.0f, v_rep);   glVertex3f(x4, y4, z4);
    glEnd();
}

// Draw the textured floor, walls, and ceiling quads.
void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex)
{
    const float half_w = ROOM_W * 0.5f;
    const float half_l = ROOM_L * 0.5f;

    const float tile = 2.0f;
    const float rep_w = ROOM_W / tile;
    const float rep_l = ROOM_L / tile;
    const float rep_h = ROOM_H / tile;

    glBindTexture(GL_TEXTURE_2D, floor_tex);
    quad_world(-half_w, -half_l, 0.0f,
               +half_w, -half_l, 0.0f,
               +half_w, +half_l, 0.0f,
               -half_w, +half_l, 0.0f,
               0.0f, 0.0f, 1.0f,
               rep_w, rep_l);

    glBindTexture(GL_TEXTURE_2D, ceiling_tex);
    quad_world(-half_w, -half_l, ROOM_H,
               -half_w, +half_l, ROOM_H,
               +half_w, +half_l, ROOM_H,
               +half_w, -half_l, ROOM_H,
               0.0f, 0.0f, -1.0f,
               rep_w, rep_l);

    // Draw the room walls.
    glDisable(GL_COLOR_MATERIAL);
    {
        float wall_amb[] = {0.18f, 0.18f, 0.18f, 1.0f};
        float wall_dif[] = {0.95f, 0.95f, 0.95f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_amb);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_dif);
    }

    glBindTexture(GL_TEXTURE_2D, wall_tex);

    quad_world(-half_w, -half_l, 0.0f,
               +half_w, -half_l, 0.0f,
               +half_w, -half_l, ROOM_H,
               -half_w, -half_l, ROOM_H,
               0.0f, 1.0f, 0.0f,
               rep_w, rep_h);

    quad_world(-half_w, +half_l, 0.0f,
               -half_w, +half_l, ROOM_H,
               +half_w, +half_l, ROOM_H,
               +half_w, +half_l, 0.0f,
               0.0f, -1.0f, 0.0f,
               rep_w, rep_h);

    quad_world(-half_w, -half_l, 0.0f,
               -half_w, -half_l, ROOM_H,
               -half_w, +half_l, ROOM_H,
               -half_w, +half_l, 0.0f,
               1.0f, 0.0f, 0.0f,
               rep_l, rep_h);

    quad_world(+half_w, -half_l, 0.0f,
               +half_w, +half_l, 0.0f,
               +half_w, +half_l, ROOM_H,
               +half_w, -half_l, ROOM_H,
               -1.0f, 0.0f, 0.0f,
               rep_l, rep_h);

    glEnable(GL_COLOR_MATERIAL);
}

#ifdef SHOW_DEBUG_AXES

// Draw axis lines and a center marker for debugging.
void draw_debug_axes_and_marker(void)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(1, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 1, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1);
    glEnd();

    glBegin(GL_TRIANGLES);
    glColor3f(1,0,0); glVertex3f(0,0,-2);
    glColor3f(0,1,0); glVertex3f(1,0,-2);
    glColor3f(0,0,1); glVertex3f(0,1,-2);
    glEnd();

    glColor3f(1,1,1);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}
#endif

