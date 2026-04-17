#include "scene_internal.h"

void apply_transform(const Entity* e)
{
    // If the entity has auto-grounding enabled (e.g., imported statues),
    // we add a per-entity Z offset so the model's local min-Z sits on the
    // intended surface (pedestal top).
    glTranslatef(e->px, e->py, e->pz + e->ground_offset_z);

    glRotatef(e->rx, 1, 0, 0);
    glRotatef(e->ry, 0, 1, 0);
    glRotatef(e->rz, 0, 0, 1);

    glScalef(e->sx, e->sy, e->sz);
}

void draw_shadow_proxy_circle(const Entity* e)
{
    // Fast shadow proxy (triangle fan) to avoid drawing high-poly models
    // multiple times in the shadow pass.
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

void set_lighting_with_intensity(const Scene* scene)
{
    // Stabil, "múzeum" jellegű világítás: egy pontfény felülről + erősebb ambient.
    // Az előző verzió spotlámpát próbált használni, de rossz paraméterrel (GL_POSITION kétszer),
    // ami erősen irányfüggő sötétedést okozott.
    // Ambient: keep some base so "0" intensity doesn't go full black,
    // but still let the ceiling naturally stay darker (it mostly gets ambient only).
    // IMPORTANT: in fixed pipeline, light components are clamped to [0..1].
    // If diffuse/specular go above 1.0, everything saturates and you feel "no change".
    // So we normalize the UI intensity [0..3] -> [0..1] for the actual OpenGL light.
    const float intensity = scene->light_intensity;
    float t = intensity / 3.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Base ambient: keep the scene readable even at 0 intensity,
    // but still let the light slider matter.
    const float amb = 0.12f + 0.18f * t;
    float ambient_light[]  = { amb, amb, amb, 1.0f };

    // Diffuse stays below 1.0 to avoid saturation ("0.9 fölött nincs különbség").
    const float d = 0.95f * t;
    float diffuse_light[]  = { d, d, d, 1.0f };

    const float s = 0.25f * t;
    float specular_light[] = { s, s, s, 1.0f };

    // Global ambient to avoid the "everything is black" look in a corridor.
    // This is separate from per-light ambient.
    {
        const float g = 0.12f + 0.10f * t;
        float globalAmb[] = { g, g, g, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    }

    // Multi-light corridor setup: use up to 3 ceiling fixtures.
    // (Still fixed pipeline, so we stay within GL_LIGHT0..GL_LIGHT2.)
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

    // Helper: configure one OpenGL light.
    static const GLenum lights[3] = { GL_LIGHT0, GL_LIGHT1, GL_LIGHT2 };
    // Point light setup (stable in fixed pipeline): no spotlight cone.
// focus

    for (int li = 0; li < 3; li++) {
        const GLenum L = lights[li];
        if (li < lamp_count) {
            glEnable(L);
            glLightfv(L, GL_AMBIENT,  ambient_light);
            glLightfv(L, GL_DIFFUSE,  diffuse_light);
            glLightfv(L, GL_SPECULAR, specular_light);
            glLightfv(L, GL_POSITION, lamp_pos[li]);

            // --- POINT LIGHT (stable), no spotlight cone ---
            glLightf(L, GL_SPOT_CUTOFF, 180.0f);
            glLightf(L, GL_SPOT_EXPONENT, 0.0f);

            // Attenuation tuned for corridor scale (avoid "no light" and avoid hard saturation)
            glLightf(L, GL_CONSTANT_ATTENUATION,  0.8f);
            glLightf(L, GL_LINEAR_ATTENUATION,    0.02f);
            glLightf(L, GL_QUADRATIC_ATTENUATION, 0.002f);
} else {
            glDisable(L);
        }
    }
}

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
    // Classic planar shadow projection matrix.
    // plane = [A,B,C,D] in world space (Ax+By+Cz+D=0)
    // light = [x,y,z,w] in world space (w=1 point light, w=0 directional)
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

int entity_casts_shadow(const Entity* e)
{
    // Don't cast shadows for wall paintings/planes.
    if (strcmp(e->type, "painting") == 0) return 0;
    if (strcmp(e->type, "plane") == 0) return 0;
    if (strcmp(e->type, "lamp") == 0) return 0;
    // Glass display cases should not cast a strong planar shadow.
    if (strcmp(e->type, "case_glass") == 0) return 0;
    // Everything else can cast.
    return 1;
}

int entity_is_transparent(const Entity* e)
{
    return (strcmp(e->type, "case_glass") == 0);
}

void draw_entity_opaque(const Entity* e)
{
    // Safety: glass/blending pass must not leak state into opaque rendering.
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPushMatrix();
    apply_transform(e);
    glBindTexture(GL_TEXTURE_2D, e->texture_id);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Some imported OBJ models have inconsistent winding / normals.
    // With backface culling enabled this can look like "holes" (missing triangles).
    // For statues we draw two-sided to avoid that artifact.
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

void draw_entity_glass(const Entity* e)
{
    // Transparent "vitrine" glass.
    // Key points (fixed pipeline):
    //   - draw AFTER all opaque objects (done in render_scene)
    //   - enable blending
    //   - disable depth writes (but keep depth test)
    //   - disable textures (so we don't get a "white painted cube")
    //   - disable color material (otherwise glColor overrides material)
    //   - draw two-sided (glass should be visible from inside too)
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);

    // A clearer glass look: low diffuse, strong specular, modest ambient.
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

    // Reset emission so it doesn't "stick" to later materials.
    {
        float emi0[] = {0.0f, 0.0f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi0);
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

void render_planar_shadows(const Scene* scene)
{
    // Collect up to 3 lamps from scene.csv.
    float lamp_pos[3][4];
    int lamp_count = 0;
    for (int i = 0; i < scene->entity_count && lamp_count < 3; i++) {
        const Entity* e = &scene->entities[i];
        if (strcmp(e->type, "lamp") == 0) {
            lamp_pos[lamp_count][0] = e->px;
            lamp_pos[lamp_count][1] = e->py;
            lamp_pos[lamp_count][2] = e->pz - 0.20f; // slightly below the fixture
            lamp_pos[lamp_count][3] = 1.0f;
            lamp_count++;
        }
    }
    if (lamp_count == 0) {
        // Fallback: one light near the ceiling, center.
        lamp_pos[0][0] = 0.0f;
        lamp_pos[0][1] = 0.0f;
        lamp_pos[0][2] = (ROOM_H - 0.25f);
        lamp_pos[0][3] = 1.0f;
        lamp_count = 1;
    }

    // If the light is "off", don't draw projected shadows.
    if (scene->light_intensity <= 0.001f) {
        return;
    }

    // Shadow strength SHOULD increase with intensity (simple, intuitive mapping).
    // Map [0..3] -> [0..0.72].
    float t = scene->light_intensity / 3.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float alpha_base = (0.72f * t);

    // Render dark, translucent projected geometry onto planes.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Prevent z-fighting with receiver surfaces.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    glDepthMask(GL_FALSE);

    // Multiple lights would create multiple shadows. For a clean "museum" look (and to
    // avoid confusing/tricky multi-shadow situations, use ONE "key" lamp for shadows.
    // The other lamps still contribute to lighting, but only the key lamp casts planar shadows.

    // Pick the lamp closest to the corridor center (|y| minimal) as the key light.
    int key = 0;
    float best_abs_y = fabsf(lamp_pos[0][1]);
    for (int li = 1; li < lamp_count; li++) {
        float ay = fabsf(lamp_pos[li][1]);
        if (ay < best_abs_y) { best_abs_y = ay; key = li; }
    }
    const float* key_light_pos = lamp_pos[key];

    // NOTE:
    // Planar (projected) shadows are a fast approximation. One common artifact is that
    // tall objects placed on a pedestal can "paint" their full shadow onto the floor,
    // even where the pedestal itself would block the light.
    //
    // Without switching to full shadow mapping, a practical fix is to draw pedestal
    // shadows *after* other objects (and slightly stronger). That way the pedestal's
    // projected shadow acts like an occluder mask over the statue shadow.

    const float* light_pos = key_light_pos;

    // Pass 1: everything except pedestals
    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];
        if (!entity_casts_shadow(e)) continue;
        if (strcmp(e->type, "pedestal") == 0) continue;

        glColor4f(0.0f, 0.0f, 0.0f, alpha_base);

        // 1) FLOOR (z=0)
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

    // Pass 2: pedestals last (masking/occluding)
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

    /* Stencil-outline highlight support */
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);                 /* default: don't write to stencil */
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    draw_room_world_quads(scene->floor_tex, scene->wall_tex, scene->ceiling_tex);

    if (scene->shadows_enabled) {
        render_planar_shadows(scene);
    }

    // Festmények és tárgyak mind Entity-ként érkeznek a scene.csv-ből.

    // entity-k
    for (int i = 0; i < scene->entity_count; i++) {
        if (i == scene->selected_entity) continue;
        const Entity* e = &scene->entities[i];
        if (entity_is_transparent(e)) continue;
        draw_entity_opaque(e);
    }

    // Transparent pass (e.g., glass display cases).
    for (int i = 0; i < scene->entity_count; i++) {
        if (i == scene->selected_entity) continue;
        const Entity* e = &scene->entities[i];
        if (!entity_is_transparent(e)) continue;
        draw_entity_glass(e);
    }

    /* Draw selected normally + write stencil = 1 */
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

        /* Outline pass: slightly scaled copy where stencil != 1 */
        glStencilMask(0x00);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);

        // Outline for transparent objects isn't very readable; keep it for opaque only.
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

// ---- Picking helpers ----

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

void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex)
{
    // Corridor-scale room (meters)
    const float half_w = ROOM_W * 0.5f;
    const float half_l = ROOM_L * 0.5f;

    // Texture tiling: keep texel density consistent when we change room size.
    // One tile roughly every 2 meters.
    const float tile = 2.0f;
    const float rep_w = ROOM_W / tile;
    const float rep_l = ROOM_L / tile;
    const float rep_h = ROOM_H / tile;

    // PADLÓ
    glBindTexture(GL_TEXTURE_2D, floor_tex);
    quad_world(-half_w, -half_l, 0.0f,
               +half_w, -half_l, 0.0f,
               +half_w, +half_l, 0.0f,
               -half_w, +half_l, 0.0f,
               0.0f, 0.0f, 1.0f,
               rep_w, rep_l);

    // PLAFON (normál lefelé)
    glBindTexture(GL_TEXTURE_2D, ceiling_tex);
    quad_world(-half_w, -half_l, ROOM_H,
               -half_w, +half_l, ROOM_H,
               +half_w, +half_l, ROOM_H,
               +half_w, -half_l, ROOM_H,
               0.0f, 0.0f, -1.0f,
               rep_w, rep_l);

    // FALAK
    // Give walls a slightly stronger ambient/diffuse material so they look consistent even in low light.
    glDisable(GL_COLOR_MATERIAL);
    {
        float wall_amb[] = {0.18f, 0.18f, 0.18f, 1.0f};
        float wall_dif[] = {0.95f, 0.95f, 0.95f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_amb);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_dif);
    }

    glBindTexture(GL_TEXTURE_2D, wall_tex);

    // HÁTSÓ (Y=-half) normál +Y
    quad_world(-half_w, -half_l, 0.0f,
               +half_w, -half_l, 0.0f,
               +half_w, -half_l, ROOM_H,
               -half_w, -half_l, ROOM_H,
               0.0f, 1.0f, 0.0f,
               rep_w, rep_h);

    // ELSŐ (Y=+half) normál -Y
    quad_world(-half_w, +half_l, 0.0f,
               -half_w, +half_l, ROOM_H,
               +half_w, +half_l, ROOM_H,
               +half_w, +half_l, 0.0f,
               0.0f, -1.0f, 0.0f,
               rep_w, rep_h);

    // BAL (X=-half) normál +X
    quad_world(-half_w, -half_l, 0.0f,
               -half_w, -half_l, ROOM_H,
               -half_w, +half_l, ROOM_H,
               -half_w, +half_l, 0.0f,
               1.0f, 0.0f, 0.0f,
               rep_l, rep_h);

    // JOBB (X=+half) normál -X
    quad_world(+half_w, -half_l, 0.0f,
               +half_w, +half_l, 0.0f,
               +half_w, +half_l, ROOM_H,
               +half_w, -half_l, ROOM_H,
               -1.0f, 0.0f, 0.0f,
               rep_l, rep_h);

    glEnable(GL_COLOR_MATERIAL);
}

#ifdef SHOW_DEBUG_AXES

void draw_debug_axes_and_marker(void)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Tengelyek (0,0,0)-ból
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(1, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 1, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1);
    glEnd();

    // Kis marker háromszög (debug)
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

