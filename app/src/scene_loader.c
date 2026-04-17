#include "scene_internal.h"

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

        // Fill friendly UI labels for the info panel.
        set_entity_metadata(e, rows[i].model);

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

        // model bounds (sphere for picking + full AABB for collision/grounding)
        {
            float minx, maxx, miny, maxy, minz, maxz;
            compute_model_bounds(&e->model,
                                 &e->bounds_center_local,
                                 &e->bounds_radius_local,
                                 &minx, &maxx, &miny, &maxy, &minz, &maxz);
            e->bounds_min_x_local = minx;
            e->bounds_max_x_local = maxx;
            e->bounds_min_y_local = miny;
            e->bounds_max_y_local = maxy;
            e->bounds_min_z_local = minz;
            e->bounds_max_z_local = maxz;
        }

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

