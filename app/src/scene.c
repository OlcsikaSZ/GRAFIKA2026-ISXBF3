#include "scene.h"
#include "csv.h"

#include <obj/load.h>
#include <obj/draw.h>

#include <string.h>
#include <stdio.h>
#include <direct.h>

// Z-up világ: X=bal/jobb, Y=előre/hátra, Z=felfelé (összhangban camera.c-vel)
// A korábbi verzió falait forgatásokkal rajzoltuk. Az gyakorlatban néha "lyukas szobát"
// eredményezett (egyes falak a kamera szögétől függően eltűntek / belógtak).
// Itt direkt világ-koordinátás quadokat rajzolunk: így determinisztikus, mindig zárt szoba.
static void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex);

// DEBUG rajzok (tengely + kis háromszög) — alapból kikapcsoljuk.
// Ha kell, fordításkor add hozzá: -DSHOW_DEBUG_AXES
#ifdef SHOW_DEBUG_AXES
static void draw_debug_axes_and_marker(void);
#endif

static void apply_transform(const Entity* e)
{
    glTranslatef(e->px, e->py, e->pz);

    glRotatef(e->rx, 1, 0, 0);
    glRotatef(e->ry, 0, 1, 0);
    glRotatef(e->rz, 0, 0, 1);

    glScalef(e->sx, e->sy, e->sz);
}

static void set_lighting_with_intensity(float intensity)
{
    // Stabil, "múzeum" jellegű világítás: egy pontfény felülről + erősebb ambient.
    // Az előző verzió spotlámpát próbált használni, de rossz paraméterrel (GL_POSITION kétszer),
    // ami erősen irányfüggő sötétedést okozott.
    float ambient_light[]  = { 0.35f * intensity, 0.35f * intensity, 0.35f * intensity, 1.0f };
    float diffuse_light[]  = { 0.85f * intensity, 0.85f * intensity, 0.85f * intensity, 1.0f };
    float specular_light[] = { 0.25f * intensity, 0.25f * intensity, 0.25f * intensity, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_light);

    // Pontfény a plafon közelében, középen.
    float position[] = { 0.0f, 0.0f, 3.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    // Kapcsoljuk ki a spot módot (180° = nem spot).
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
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
        e->anim_angle_deg = e->ry;

        load_model(&e->model, rows[i].model);

        // textura
        e->texture_id = load_texture((char*)rows[i].texture);

        printf("Loaded entity: %s | model=%s | tex=%s\n", e->type, rows[i].model, rows[i].texture);
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
                e->ry = e->anim_angle_deg;
            }
        }
    }
}

void render_scene(const Scene* scene)
{
    set_material(&scene->material);
    set_lighting_with_intensity(scene->light_intensity);

#ifdef SHOW_DEBUG_AXES
    draw_debug_axes_and_marker();
#endif

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);

    draw_room_world_quads(scene->floor_tex, scene->wall_tex, scene->ceiling_tex);

    // Festmények és tárgyak mind Entity-ként érkeznek a scene.csv-ből.

    // entity-k
    for (int i = 0; i < scene->entity_count; i++) {
        const Entity* e = &scene->entities[i];

        glPushMatrix();
        apply_transform(e);

        // ha textúrázol:
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, e->texture_id);
        glColor3f(1.0f, 1.0f, 1.0f); // ne színezzük el
        draw_model((Model*)&e->model);
        glDisable(GL_TEXTURE_2D);

        //glDisable(GL_TEXTURE_2D);

        glPopMatrix();
    }
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
    const float room_w = 12.0f;
    const float room_h = 4.0f;
    const float half   = room_w * 0.5f;

    // PADLÓ
    glBindTexture(GL_TEXTURE_2D, floor_tex);
    quad_world(-half, -half, 0.0f,
               +half, -half, 0.0f,
               +half, +half, 0.0f,
               -half, +half, 0.0f,
               0.0f, 0.0f, 1.0f,
               8.0f, 8.0f);

    // PLAFON (normál lefelé)
    glBindTexture(GL_TEXTURE_2D, ceiling_tex);
    quad_world(-half, -half, room_h,
               -half, +half, room_h,
               +half, +half, room_h,
               +half, -half, room_h,
               0.0f, 0.0f, -1.0f,
               8.0f, 8.0f);

    // FALAK
    glBindTexture(GL_TEXTURE_2D, wall_tex);

    // HÁTSÓ (Y=-half) normál +Y
    quad_world(-half, -half, 0.0f,
               +half, -half, 0.0f,
               +half, -half, room_h,
               -half, -half, room_h,
               0.0f, 1.0f, 0.0f,
               1.0f, 1.0f);

    // ELSŐ (Y=+half) normál -Y
    quad_world(-half, +half, 0.0f,
               -half, +half, room_h,
               +half, +half, room_h,
               +half, +half, 0.0f,
               0.0f, -1.0f, 0.0f,
               1.0f, 1.0f);

    // BAL (X=-half) normál +X
    quad_world(-half, -half, 0.0f,
               -half, -half, room_h,
               -half, +half, room_h,
               -half, +half, 0.0f,
               1.0f, 0.0f, 0.0f,
               1.0f, 1.0f);

    // JOBB (X=+half) normál -X
    quad_world(+half, -half, 0.0f,
               +half, +half, 0.0f,
               +half, +half, room_h,
               +half, -half, room_h,
               -1.0f, 0.0f, 0.0f,
               1.0f, 1.0f);
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
