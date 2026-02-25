#include "scene.h"
#include "csv.h"

#include <obj/load.h>
#include <obj/draw.h>

#include <string.h>
#include <stdio.h>
#include <direct.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void compute_model_bounds_sphere(const Model* m, vec3* out_center, float* out_radius)
{
    if (!m || !m->vertices || m->n_vertices <= 0) {
        out_center->x = out_center->y = out_center->z = 0.0f;
        *out_radius = 1.0f;
        return;
    }

    float minx = m->vertices[0].x, maxx = m->vertices[0].x;
    float miny = m->vertices[0].y, maxy = m->vertices[0].y;
    float minz = m->vertices[0].z, maxz = m->vertices[0].z;

    for (int i = 1; i < m->n_vertices; i++) {
        const float x = m->vertices[i].x;
        const float y = m->vertices[i].y;
        const float z = m->vertices[i].z;
        if (x < minx) { minx = x; }
        if (x > maxx) { maxx = x; }

        if (y < miny) { miny = y; }
        if (y > maxy) { maxy = y; }

        if (z < minz) { minz = z; }
        if (z > maxz) { maxz = z; }
    }

    out_center->x = (minx + maxx) * 0.5f;
    out_center->y = (miny + maxy) * 0.5f;
    out_center->z = (minz + maxz) * 0.5f;

    const float dx = (maxx - minx);
    const float dy = (maxy - miny);
    const float dz = (maxz - minz);
    *out_radius = 0.5f * sqrtf(dx*dx + dy*dy + dz*dz);
    if (*out_radius < 0.001f) *out_radius = 0.001f;
}

static float compute_model_min_z(const Model* m)
{
    if (!m || !m->vertices || m->n_vertices <= 0) {
        return 0.0f;
    }
    float minz = m->vertices[0].z;
    for (int i = 1; i < m->n_vertices; i++) {
        const float z = m->vertices[i].z;
        if (z < minz) minz = z;
    }
    return minz;
}

// Z-up világ: X=bal/jobb, Y=előre/hátra, Z=felfelé (összhangban camera.c-vel)
// A korábbi verzió falait forgatásokkal rajzoltuk. Az gyakorlatban néha "lyukas szobát"
// eredményezett (egyes falak a kamera szögétől függően eltűntek / belógtak).
// Itt direkt világ-koordinátás quadokat rajzolunk: így determinisztikus, mindig zárt szoba.
static void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex);

// "Corridor" room dimensions (must match camera.c clamp + shadow planes).
#define ROOM_W 10.0f
#define ROOM_L 26.0f
#define ROOM_H 4.0f

// DEBUG rajzok (tengely + kis háromszög) — alapból kikapcsoljuk.
// Ha kell, fordításkor add hozzá: -DSHOW_DEBUG_AXES
#ifdef SHOW_DEBUG_AXES
static void draw_debug_axes_and_marker(void);
#endif

static void apply_transform(const Entity* e)
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

static void draw_shadow_proxy_circle(const Entity* e)
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

static void set_lighting_with_intensity(const Scene* scene)
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

static void set_material(const Material* material)
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

void init_scene(Scene* scene)
{
    memset(scene, 0, sizeof(*scene));
    scene->entity_count = 0;
    scene->light_intensity = 1.0f;
    scene->time_sec = 0.0;
    scene->animation_enabled = 1;
    scene->selected_entity = -1;
    scene->shadows_enabled = 1;

    // anyag (maradhat MVP-ben közös mindenkire)
    scene->material.ambient.red = 0.0f;
    scene->material.ambient.green = 0.0f;
    scene->material.ambient.blue = 0.0f;

    scene->material.diffuse.red = 0.4f;
    scene->material.diffuse.green = 0.8f;
    scene->material.diffuse.blue = 0.8f;

    scene->material.specular.red = 1.0f;
    scene->material.specular.green = 1.0f;
    scene->material.specular.blue = 1.0f;

    scene->material.shininess = 100.0;

    scene->floor_tex = load_texture("assets/textures/floor.jpg");
    // Use JPG textures to avoid libpng DLL issues on some MinGW/SDL2_image setups.
    scene->wall_tex  = load_texture("assets/textures/wall.jpg");
    scene->ceiling_tex = load_texture("assets/textures/ceiling.jpg");
    // Festmények már a scene.csv-ből jönnek (plane.obj + painting*.jpg)
}

void toggle_shadows(Scene* scene)
{
    scene->shadows_enabled = !scene->shadows_enabled;
    printf("Shadows: %s\n", scene->shadows_enabled ? "ON" : "OFF");
}

static void build_shadow_matrix(float out[16], const float plane[4], const float light[4])
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

static int entity_casts_shadow(const Entity* e)
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

static int entity_is_transparent(const Entity* e)
{
    return (strcmp(e->type, "case_glass") == 0);
}

static void draw_entity_opaque(const Entity* e)
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

static int find_nearest_pedestal(const Scene* scene, const Entity* statue)
{
    int best = -1;
    float best_d2 = 1e30f;
    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* p = &scene->entities[i];
        if (strcmp(p->type, "pedestal") != 0) continue;
        const float dx = statue->px - p->px;
        const float dy = statue->py - p->py;
        const float d2 = dx*dx + dy*dy;
        if (d2 < best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

static void draw_entity_glass(const Entity* e)
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

static void render_planar_shadows(const Scene* scene)
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

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];
        if (!entity_casts_shadow(e)) continue;

        const float* light_pos = key_light_pos;

        float per_alpha = alpha_base;
        if (strcmp(e->type, "pedestal") == 0) {
            per_alpha *= 0.75f;
        }
        glColor4f(0.0f, 0.0f, 0.0f, per_alpha);

        // Performance note:
        // Projected planar shadows require re-drawing geometry. With imported
        // high-poly statues this can become very slow. For a stable demo:
        //  - project ONLY to the floor (not to walls)
        //  - and for high-poly models draw a low-poly proxy instead.

        // 1) FLOOR (z=0)
        {
            const float floor_plane[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
            float m[16];
            build_shadow_matrix(m, floor_plane, light_pos);
            glPushMatrix();
            glMultMatrixf(m);
            glTranslatef(0.0f, 0.0f, 0.0030f);
            apply_transform(e);
            // Use full projected geometry for most objects.
            // Only fall back to a cheap circular proxy for extremely high-poly meshes.
            if (e->model.n_vertices > 50000) {
                draw_shadow_proxy_circle(e);
            } else {
                draw_model((Model*)&e->model);
            }
            glPopMatrix();
        }
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
    glColor4f(1, 1, 1, 1);
}

void toggle_animation(Scene* scene)
{
    scene->animation_enabled = !scene->animation_enabled;
    printf("Animation: %s\n", scene->animation_enabled ? "ON" : "OFF");
}

void destroy_scene(Scene* scene)
{
    for (int i = 0; i < scene->entity_count; i++) {
        free_model(&scene->entities[i].model);
        // ha van texture delete függvényed: glDeleteTextures(1, &scene->entities[i].texture_id);
    }
    scene->entity_count = 0;
}

void change_light(Scene* scene, float delta)
{
    scene->light_intensity += delta;
    if (scene->light_intensity < 0.0f) scene->light_intensity = 0.0f;
    if (scene->light_intensity > 3.0f) scene->light_intensity = 3.0f;

    printf("Light intensity: %.2f\n", scene->light_intensity);
}

void load_museum_scene(Scene* scene, const char* scene_csv_path)
{

    static int printed = 0;
    if (!printed) {
        char cwd[512];
        if (_getcwd(cwd, sizeof(cwd))) {
            printf("CWD: %s\n", cwd);
        }
        printed = 1;
    }

    SceneRow rows[MAX_ENTITIES];
    size_t count = 0;

    if (!load_scene_csv(scene_csv_path, rows, MAX_ENTITIES, &count)) {
        printf("[ERROR] Could not load scene csv: %s\n", scene_csv_path);
        return;
    }

    scene->entity_count = 0;

    for (size_t i = 0; i < count; i++) {
        if (scene->entity_count >= MAX_ENTITIES) break;

        Entity* e = &scene->entities[scene->entity_count++];
        memset(e, 0, sizeof(*e));

        strncpy(e->type, rows[i].type, sizeof(e->type) - 1);

        e->px = rows[i].px; e->py = rows[i].py; e->pz = rows[i].pz;
        e->rx = rows[i].rx; e->ry = rows[i].ry; e->rz = rows[i].rz;
        e->sx = rows[i].sx; e->sy = rows[i].sy; e->sz = rows[i].sz;

        e->animated = (strcmp(e->type, "statue") == 0);
        // Z-up world: we spin statues around Z (yaw), so seed animation from rz.
        e->anim_angle_deg = e->rz;

        load_model(&e->model, rows[i].model);

        // Small "extra" for presentation: keep certain pieces static.
        // (e.g., the angel/fairy and the trophy look better as fixed exhibits.)
        if (e->animated) {
            const char* mp = rows[i].model;
            if (mp && (strstr(mp, "fairy") || strstr(mp, "trophy"))) {
                e->animated = 0;
            }
        }

        compute_model_bounds_sphere(&e->model, &e->bounds_center_local, &e->bounds_radius_local);
        e->bounds_min_z_local = compute_model_min_z(&e->model);

        // Auto-grounding for statues:
        // We compute a local min-Z and store an offset so the model's base can sit on a surface.
        // The actual target surface height (pedestal top) is assigned AFTER all entities are loaded
        // (so we can find the nearest pedestal).
        if (strcmp(e->type, "statue") == 0) {
            e->ground_offset_z = (-e->bounds_min_z_local) * e->sz;
        } else {
            e->ground_offset_z = 0.0f;
        }

        // textura
        e->texture_id = load_texture((char*)rows[i].texture);

        printf("Loaded entity: %s | model=%s | tex=%s\n", e->type, rows[i].model, rows[i].texture);
    }

    // Post-process: snap each statue onto the nearest pedestal.
    // This removes the "floating" artifacts when models have different local origins.
    for (int i = 0; i < scene->entity_count; i++) {
        Entity* e = &scene->entities[i];
        if (strcmp(e->type, "statue") != 0) continue;

        const int pidx = find_nearest_pedestal(scene, e);
        if (pidx < 0) continue;
        const Entity* p = &scene->entities[pidx];

        // Pedestal top height in world Z.
        // pedestal.obj is treated as roughly 1 unit tall in local space.
        const float pedestal_top_z = p->pz + 0.5f * p->sz;

        // We want: (e->pz + e->ground_offset_z) + (minZ * scaleZ) == pedestal_top_z.
        // Since ground_offset_z == -minZ*scaleZ, we can simply set e->pz = pedestal_top_z.
        e->pz = pedestal_top_z;
    }
}

void update_scene(Scene* scene, double elapsed_time)
{
    scene->time_sec += elapsed_time;

    // időalapú anim: statue forog
    if (scene->animation_enabled) {
        for (int i = 0; i < scene->entity_count; i++) {
            Entity* e = &scene->entities[i];
            if (e->animated) {
                e->anim_angle_deg += (float)(elapsed_time * 20.0); // 20 deg/sec
                // wrap
                if (e->anim_angle_deg > 360.0f) e->anim_angle_deg -= 360.0f;
                if (e->anim_angle_deg < 0.0f)   e->anim_angle_deg += 360.0f;
                // Z-up world: yaw around the UP axis (Z rotation in apply_transform order)
                // so the statue rotates "on the pedestal" instead of tumbling.
                e->rz = e->anim_angle_deg;
            }
        }
    }
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

static int invert_matrix_4x4(const double m[16], double inv_out[16])
{
    // Adapted from the classic GLU inversion snippet (public domain style).
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

static void mult_mat4_vec4(const double m[16], const double v[4], double out[4])
{
    // Column-major OpenGL matrix
    out[0] = m[0]*v[0] + m[4]*v[1] + m[8]*v[2]  + m[12]*v[3];
    out[1] = m[1]*v[0] + m[5]*v[1] + m[9]*v[2]  + m[13]*v[3];
    out[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3];
    out[3] = m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3];
}

static void mult_mat4_mat4(const double a[16], const double b[16], double out[16])
{
    // out = a*b (column-major)
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

static void vec3_sub(const double a[3], const double b[3], double out[3])
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static void vec3_norm(double v[3])
{
    const double len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len > 1e-12) { v[0] /= len; v[1] /= len; v[2] /= len; }
}

static void rotate_point_xyz_deg(double p[3], float rx, float ry, float rz)
{
    // Apply the same rotation order as apply_transform(): X then Y then Z
    const double x0 = p[0], y0 = p[1], z0 = p[2];
    double x = x0, y = y0, z = z0;

    const double ax = degree_to_radian(rx);
    const double ay = degree_to_radian(ry);
    const double az = degree_to_radian(rz);

    // X
    {
        const double cy = cos(ax), sy = sin(ax);
        const double y1 = y*cy - z*sy;
        const double z1 = y*sy + z*cy;
        y = y1; z = z1;
    }
    // Y
    {
        const double cx = cos(ay), sx = sin(ay);
        const double x1 = x*cx + z*sx;
        const double z1 = -x*sx + z*cx;
        x = x1; z = z1;
    }
    // Z
    {
        const double cz = cos(az), sz = sin(az);
        const double x1 = x*cz - y*sz;
        const double y1 = x*sz + y*cz;
        x = x1; y = y1;
    }

    p[0] = x; p[1] = y; p[2] = z;
}

static int ray_sphere_intersect(const double ro[3], const double rd[3],
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

    // Ignore clicks outside the viewport
    if (mouse_x < viewport_x || mouse_x >= viewport_x + viewport_w ||
        mouse_y < viewport_y || mouse_y >= viewport_y + viewport_h) {
        scene->selected_entity = -1;
        return -1;
    }

    // Recreate the same projection+view matrices used for rendering
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

    // NDC coordinates
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

        // world center = T + R * (S * local_center)
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

static void quad_world(float x1, float y1, float z1,
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

static void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex)
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
static void draw_debug_axes_and_marker(void)
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
