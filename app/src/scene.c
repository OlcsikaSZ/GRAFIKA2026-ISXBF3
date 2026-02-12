#include "scene.h"
#include "csv.h"

#include <obj/load.h>
#include <obj/draw.h>

#include <string.h>
#include <stdio.h>
#include <direct.h>

// Z-up világ: X=bal/jobb, Y=előre/hátra, Z=felfelé (összhangban camera.c-vel)
static void draw_textured_quad_centered_xy(float w, float h, float u_repeat, float v_repeat);
static void draw_room(GLuint floor_tex, GLuint wall_tex);

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
    float ambient_light[]  = { 0.5f * intensity, 0.5f * intensity, 0.5f * intensity, 1.0f };
    float diffuse_light[]  = { 1.0f * intensity, 1.0f * intensity, 1.0f * intensity, 1.0f };
    float specular_light[] = { 1.0f * intensity, 1.0f * intensity, 1.0f * intensity, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_light);

    float position[] = { 0.0f, 3.0f, 2.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    float direction[] = { 0.2f, -1.0f, -0.2f, 0.0f }; // w=0 -> directional
    glLightfv(GL_LIGHT0, GL_POSITION, direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 35.0f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 10.0f);
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
    scene->wall_tex  = load_texture("assets/textures/wall.jpg");
    scene->painting_tex = load_texture("assets/textures/painting1.jpg");
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
    for (int i = 0; i < scene->entity_count; i++) {
        Entity* e = &scene->entities[i];
        if (e->animated) {
            e->ry = (float)(scene->time_sec * 20.0); // 20 deg/sec
        }
    }
}

void render_scene(const Scene* scene)
{
    set_material(&scene->material);
    set_lighting_with_intensity(scene->light_intensity);

    draw_origin();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);
    glColor3f(1,0,0); glVertex3f(0,0,-2);
    glColor3f(0,1,0); glVertex3f(1,0,-2);
    glColor3f(0,0,1); glVertex3f(0,1,-2);
    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);

    draw_room(scene->floor_tex, scene->wall_tex);

    // KÉP 1 a hátsó falon (Y = -6 környéke), kicsit a fal elé tolva
    glBindTexture(GL_TEXTURE_2D, scene->painting_tex);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    // Hátsó fal síkja: X-Z, befelé +Y. A kép legyen ~1.5m magasan, középen.
    glTranslatef(0.0f, -5.95f, 1.6f);
    glRotatef(90.0f, 1, 0, 0);       // XY -> XZ
    glRotatef(180.0f, 0, 0, 1);      // normál +Y felé
    draw_textured_quad_centered_xy(3.0f, 2.0f, 1.0f, 1.0f);
    glPopMatrix();

    // padló (később textúrázzuk)
    /*glPushMatrix();
    glTranslatef(-5.0f, -1.0f, -5.0f);
    glScalef(10.0f, 10.0f, 1.0f);
    draw_plane(50);
    glPopMatrix();*/

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

static void draw_textured_quad_centered_xy(float w, float h, float u_repeat, float v_repeat)
{
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);

    glTexCoord2f(0, 0);               glVertex3f(-w/2, -h/2, 0);
    glTexCoord2f(u_repeat, 0);        glVertex3f( w/2, -h/2, 0);
    glTexCoord2f(u_repeat, v_repeat); glVertex3f( w/2,  h/2, 0);
    glTexCoord2f(0, v_repeat);        glVertex3f(-w/2,  h/2, 0);

    glEnd();
}

static void draw_room(GLuint floor_tex, GLuint wall_tex)
{
    // Z-up szoba paraméterek
    const float room_w = 12.0f;
    const float room_h = 4.0f;
    const float half   = room_w * 0.5f;

    glPushMatrix();

    // PADLÓ: XY sík, Z=0
    glBindTexture(GL_TEXTURE_2D, floor_tex);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    draw_textured_quad_centered_xy(room_w, room_w, 8.0f, 8.0f);
    glPopMatrix();

    // FAL TEXTÚRA
    glBindTexture(GL_TEXTURE_2D, wall_tex);

    // HÁTSÓ FAL (Y = -half), befelé +Y
    glPushMatrix();
    glTranslatef(0.0f, -half, room_h * 0.5f);
    glRotatef(90.0f, 1, 0, 0);       // XY -> XZ (normál -Y)
    glRotatef(180.0f, 0, 0, 1);      // normál +Y
    draw_textured_quad_centered_xy(room_w, room_h, 1.0f, 1.0f);
    glPopMatrix();

    // ELSŐ FAL (Y = +half), befelé -Y
    glPushMatrix();
    glTranslatef(0.0f, half, room_h * 0.5f);
    glRotatef(90.0f, 1, 0, 0);       // normál -Y
    draw_textured_quad_centered_xy(room_w, room_h, 1.0f, 1.0f);
    glPopMatrix();

    // BAL FAL (X = -half), befelé +X
    glPushMatrix();
    glTranslatef(-half, 0.0f, room_h * 0.5f);
    glRotatef(90.0f, 1, 0, 0);       // XY -> XZ
    glRotatef(-90.0f, 0, 0, 1);      // XZ -> YZ, normál +X
    draw_textured_quad_centered_xy(room_w, room_h, 1.0f, 1.0f);
    glPopMatrix();

    // JOBB FAL (X = +half), befelé -X
    glPushMatrix();
    glTranslatef(half, 0.0f, room_h * 0.5f);
    glRotatef(90.0f, 1, 0, 0);
    glRotatef(90.0f, 0, 0, 1);       // normál -X
    draw_textured_quad_centered_xy(room_w, room_h, 1.0f, 1.0f);
    glPopMatrix();

    glPopMatrix();
}