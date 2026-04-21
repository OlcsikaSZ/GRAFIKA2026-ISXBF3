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

    char display_name[64];
    char description[128];

    Model model;
    GLuint texture_id;

    float px, py, pz;
    float rx, ry, rz;
    float sx, sy, sz;

    int animated;
    float anim_angle_deg;

    // Store local bounds for object picking.
    vec3 bounds_center_local;
    float bounds_radius_local;

    // Store local X/Y bounds for collision tests.
    float bounds_min_x_local;
    float bounds_max_x_local;
    float bounds_min_y_local;
    float bounds_max_y_local;

    // Store local Z bounds for grounding and collision.
    float bounds_min_z_local;
    float bounds_max_z_local;

    // Offset the model so it rests correctly on a surface.
    float ground_offset_z;
} Entity;

typedef struct Scene
{
    Entity entities[MAX_ENTITIES];
    int entity_count;

    Material material;
    float light_intensity;
    double time_sec;
    int animation_enabled;

    GLuint floor_tex;
    GLuint wall_tex;
    GLuint ceiling_tex;

    // Store the currently selected entity.
    int selected_entity;

    // Enable or disable planar shadows.
    int shadows_enabled;
} Scene;

// Initialize the scene state and shared textures.
void init_scene(Scene* scene);

// Release all models and textures owned by the scene.
void destroy_scene(Scene* scene);

// Load scene entities from the CSV configuration.
void load_museum_scene(Scene* scene, const char* scene_csv_path);

// Adjust the current scene light intensity.
void change_light(Scene* scene, float delta);

// Update animations and scene time.
void update_scene(Scene* scene, double elapsed_time);

// Render the room, entities, and selection cues.
void render_scene(const Scene* scene);

// Enable or disable animated objects.
void toggle_animation(Scene* scene);

// Enable or disable planar shadows.
void toggle_shadows(Scene* scene);

// Pick an entity from the current mouse position.
int pick_entity(Scene* scene, const Camera* camera,
                int mouse_x, int mouse_y,
                int viewport_x, int viewport_y, int viewport_w, int viewport_h);

void draw_origin(void);
void draw_plane(int n);

// Push the camera out of collidable scene objects.
void resolve_camera_collisions(const Scene* scene, Camera* camera);

#endif /* SCENE_H */
