#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"
#include <stdbool.h>

typedef struct Camera
{
    vec3 position;
    vec3 rotation;
    vec3 speed;
    bool is_preview_visible;

    bool walk_bob_enabled;
    double walk_phase;
    double bob_offset;
} Camera;

// Set the camera to its initial position and orientation.
void init_camera(Camera* camera);

// Move the camera based on its current speed values.
void update_camera(Camera* camera, double time);

// Apply the camera transform to the current view matrix.
void set_view(const Camera* camera);

// Rotate the camera while clamping the vertical angle.
void rotate_camera(Camera* camera, double horizontal, double vertical);

// Set forward and backward movement speed.
void set_camera_speed(Camera* camera, double speed);

// Set left and right strafe speed.
void set_camera_side_speed(Camera* camera, double speed);

// Set vertical movement speed.
void set_camera_vertical_speed(Camera* camera, double speed);

// Enable or disable the walking bob effect.
void toggle_walk_bob(Camera* camera);

// Keep the camera inside the room limits.
void clamp_camera_to_room(Camera* camera);

void show_texture_preview(void);

#endif /* CAMERA_H */
