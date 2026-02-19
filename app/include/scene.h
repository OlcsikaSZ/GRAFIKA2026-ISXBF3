#ifndef SCENE_H
#define SCENE_H

#include "camera.h"
#include "texture.h"
#include "utils.h"

#include <obj/model.h>
#include <SDL2/SDL_opengl.h>

#define MAX_ENTITIES 64

typedef struct Entity
{
    char type[32];

    Model model;
    GLuint texture_id;

    float px, py, pz;
    float rx, ry, rz;
    float sx, sy, sz;

    int animated; // 1 = forog

    // Animáció állapot (fok). Azért tároljuk külön, hogy pause/resume után
    // ugyanonnan folytassa, és ne ugorjon "időből számolt" szögre.
    float anim_angle_deg;

    /* Local-space bounding sphere for picking */
    vec3 bounds_center_local;
    float bounds_radius_local;
} Entity;

typedef struct Scene
{
    Entity entities[MAX_ENTITIES];
    int entity_count;

    Material material;

    float light_intensity;

    // idő alapú animhoz: eltelt idő (összegzett)
    double time_sec;

    // Animáció kapcsoló (pl. szobor forgatás indítás/megállítás)
    int animation_enabled;

    GLuint floor_tex;
    GLuint wall_tex;
    GLuint ceiling_tex;
    // Festmények is Entity-ként jönnek a scene.csv-ből.

    /* Picking */
    int selected_entity;

    /* Simple projected shadows */
    int shadows_enabled;

} Scene;

void init_scene(Scene* scene);
void destroy_scene(Scene* scene);

void load_museum_scene(Scene* scene, const char* scene_csv_path);

void change_light(Scene* scene, float delta);

void update_scene(Scene* scene, double elapsed_time);
void render_scene(const Scene* scene);

// Egyszerű animáció kapcsoló (pl. statue forgás)
void toggle_animation(Scene* scene);

/* Toggle simple projected shadows (planar). */
void toggle_shadows(Scene* scene);

/* Returns picked entity index, or -1 if none. Also sets scene->selected_entity. */
int pick_entity(Scene* scene, const Camera* camera,
                int mouse_x, int mouse_y,
                int viewport_x, int viewport_y, int viewport_w, int viewport_h);

void draw_origin(void);
void draw_plane(int n);

#endif /* SCENE_H */
