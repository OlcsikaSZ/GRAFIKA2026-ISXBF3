#include "scene_internal.h"

// Compute local-space bounds used for picking and collisions.
void compute_model_bounds(const Model* m,
                                 vec3* out_center,
                                 float* out_radius,
                                 float* out_minx, float* out_maxx,
                                 float* out_miny, float* out_maxy,
                                 float* out_minz, float* out_maxz)
{
    if (!m || !m->vertices || m->n_vertices <= 0) {
        out_center->x = out_center->y = out_center->z = 0.0f;
        *out_radius = 1.0f;
        *out_minx = *out_miny = *out_minz = -0.5f;
        *out_maxx = *out_maxy = *out_maxz =  0.5f;
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

    *out_minx = minx; *out_maxx = maxx;
    *out_miny = miny; *out_maxy = maxy;
    *out_minz = minz; *out_maxz = maxz;
}

// Assign display labels and descriptions based on entity type.
void set_entity_metadata(Entity* e, const char* model_path)
{
    if (!e) return;

    snprintf(e->display_name, sizeof(e->display_name), "%s", e->type);
    snprintf(e->description, sizeof(e->description), "Exhibit object");

    const char* mp = model_path ? model_path : "";

    if (strcmp(e->type, "painting") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Painting");
        snprintf(e->description, sizeof(e->description), "Wall-mounted artwork");
    } else if (strcmp(e->type, "pedestal") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Pedestal");
        snprintf(e->description, sizeof(e->description), "Marble stand for exhibits");
    } else if (strcmp(e->type, "case_base") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Display Base");
        snprintf(e->description, sizeof(e->description), "Vitrine base (marble)");
    } else if (strcmp(e->type, "case_glass") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Glass Case");
        snprintf(e->description, sizeof(e->description), "Transparent cover (glass)");
    } else if (strcmp(e->type, "duck") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Duck Exhibit");
        snprintf(e->description, sizeof(e->description), "Textured model in glass case");
    } else if (strcmp(e->type, "lamp") == 0) {
        snprintf(e->display_name, sizeof(e->display_name), "Ceiling Lamp");
        snprintf(e->description, sizeof(e->description), "Main light source");
    } else if (strcmp(e->type, "statue") == 0) {
        if (strstr(mp, "david")) {
            snprintf(e->display_name, sizeof(e->display_name), "David Statue");
            snprintf(e->description, sizeof(e->description), "Classic sculpture (rotatable)");
        } else if (strstr(mp, "statue_classic")) {
            snprintf(e->display_name, sizeof(e->display_name), "Classic Statue");
            snprintf(e->description, sizeof(e->description), "Stone exhibit (rotatable)");
        } else if (strstr(mp, "fairy")) {
            snprintf(e->display_name, sizeof(e->display_name), "Fairy Statue");
            snprintf(e->description, sizeof(e->description), "Decorative sculpture");
        } else if (strstr(mp, "trophy")) {
            snprintf(e->display_name, sizeof(e->display_name), "Trophy");
            snprintf(e->description, sizeof(e->description), "Award-style exhibit");
        } else {
            snprintf(e->display_name, sizeof(e->display_name), "Statue");
            snprintf(e->description, sizeof(e->description), "Sculpture exhibit");
        }
    }
}

// Find the closest pedestal for automatic statue placement.
int find_nearest_pedestal(const Scene* scene, const Entity* statue)
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

// Initialize scene defaults, materials, and room textures.
void init_scene(Scene* scene)
{
    memset(scene, 0, sizeof(*scene));
    scene->entity_count = 0;
    scene->light_intensity = 1.0f;
    scene->time_sec = 0.0;
    scene->animation_enabled = 1;
    scene->selected_entity = -1;
    scene->shadows_enabled = 1;

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
    scene->ceiling_tex = load_texture("assets/textures/ceiling.jpg");
}

// Toggle planar shadow rendering on or off.
void toggle_shadows(Scene* scene)
{
    scene->shadows_enabled = !scene->shadows_enabled;
    printf("Shadows: %s\n", scene->shadows_enabled ? "ON" : "OFF");
}

// Toggle the animated exhibit rotation state.
void toggle_animation(Scene* scene)
{
    scene->animation_enabled = !scene->animation_enabled;
    printf("Animation: %s\n", scene->animation_enabled ? "ON" : "OFF");
}

// Free loaded models and textures owned by the scene.
void destroy_scene(Scene* scene)
{
    if (scene == NULL) {
        return;
    }

    for (int i = 0; i < scene->entity_count; i++) {
        free_model(&scene->entities[i].model);

        if (scene->entities[i].texture_id != 0) {
            glDeleteTextures(1, &scene->entities[i].texture_id);
            scene->entities[i].texture_id = 0;
        }
    }

    if (scene->floor_tex != 0) {
        glDeleteTextures(1, &scene->floor_tex);
        scene->floor_tex = 0;
    }

    if (scene->wall_tex != 0) {
        glDeleteTextures(1, &scene->wall_tex);
        scene->wall_tex = 0;
    }

    if (scene->ceiling_tex != 0) {
        glDeleteTextures(1, &scene->ceiling_tex);
        scene->ceiling_tex = 0;
    }

    scene->entity_count = 0;
    scene->selected_entity = -1;
}

// Adjust the global scene light intensity.
void change_light(Scene* scene, float delta)
{
    scene->light_intensity += delta;
    if (scene->light_intensity < 0.0f) scene->light_intensity = 0.0f;
    if (scene->light_intensity > 3.0f) scene->light_intensity = 3.0f;

    printf("Light intensity: %.2f\n", scene->light_intensity);
}

// Update per-frame scene animation state.
void update_scene(Scene* scene, double elapsed_time)
{
    scene->time_sec += elapsed_time;

    if (scene->animation_enabled) {
        for (int i = 0; i < scene->entity_count; i++) {
            Entity* e = &scene->entities[i];
            if (e->animated) {
                e->anim_angle_deg += (float)(elapsed_time * 20.0);
                if (e->anim_angle_deg > 360.0f) e->anim_angle_deg -= 360.0f;
                if (e->anim_angle_deg < 0.0f)   e->anim_angle_deg += 360.0f;
                e->rz = e->anim_angle_deg;
            }
        }
    }
}

