#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

#include <stdbool.h>

/**
 * Camera, as a moving point with direction
 */
typedef struct Camera
{
    vec3 position;
    vec3 rotation;
    vec3 speed;
    bool is_preview_visible;

    // "Walking" hatás (fej-bólogatás) – opcionális, külön állapotban tároljuk,
    // hogy az ütközés/klamp a valódi pozícióval számoljon.
    bool walk_bob_enabled;
    double walk_phase;
    double bob_offset;   // aktuális függőleges offset
} Camera;

/**
 * Initialize the camera to the start position.
 */
void init_camera(Camera* camera);

/**
 * Update the position of the camera.
 */
void update_camera(Camera* camera, double time);

/**
 * Apply the camera settings to the view transformation.
 */
void set_view(const Camera* camera);

/**
 * Set the horizontal and vertical rotation of the view angle.
 */
void rotate_camera(Camera* camera, double horizontal, double vertical);

/**
 * Set the speed of forward and backward motion.
 */
void set_camera_speed(Camera* camera, double speed);

/**
 * Set the speed of left and right side steps.
 */
void set_camera_side_speed(Camera* camera, double speed);

/**
 * Set the speed of up and down steps.
 */
void set_camera_vertical_speed(Camera* camera, double speed);

/**
 * Toggle walking head-bob effect.
 */
void toggle_walk_bob(Camera* camera);

void show_texture_preview(void);

#endif /* CAMERA_H */
