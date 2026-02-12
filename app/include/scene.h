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
} Entity;

typedef struct Scene
{
    Entity entities[MAX_ENTITIES];
    int entity_count;

    Material material;

    float light_intensity;

    // idő alapú animhoz: eltelt idő (összegzett)
    double time_sec;

    GLuint floor_tex;
    GLuint wall_tex;
    GLuint ceiling_tex;
    GLuint painting_tex;

} Scene;

void init_scene(Scene* scene);
void destroy_scene(Scene* scene);

void load_museum_scene(Scene* scene, const char* scene_csv_path);

void change_light(Scene* scene, float delta);

void update_scene(Scene* scene, double elapsed_time);
void render_scene(const Scene* scene);

void draw_origin(void);
void draw_plane(int n);

#endif /* SCENE_H */
