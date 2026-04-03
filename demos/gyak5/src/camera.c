#include "camera.h"

#include <GL/gl.h>

#include <math.h>

void init_camera(Camera* camera)
{
    camera->position.x = 0.0;
    camera->position.y = 0.0;
    camera->position.z = 2.0;
    camera->rotation.x = 0.0;
    camera->rotation.y = 0.0;
    camera->rotation.z = 0.0;
    camera->speed.x = 0.0;
    camera->speed.y = 0.0;
    camera->speed.z = 0.0;
}

void update_camera(Camera* camera, double time)
{
    double angle = degree_to_radian(camera->rotation.y); // Forgatás jobbra-balra
    double pitch = degree_to_radian(camera->rotation.x); // Döntés fel-le

    // Előre-hátra mozgás (a nézeti irány szerint)
    camera->position.x += cos(angle) * cos(pitch) * camera->speed.y * time;
    camera->position.y += sin(angle) * cos(pitch) * camera->speed.y * time;
    camera->position.z += sin(pitch) * camera->speed.y * time;

    // Oldalirányú mozgás
    double side_angle = angle + M_PI / 2.0;
    camera->position.x += cos(side_angle) * camera->speed.x * time;
    camera->position.y += sin(side_angle) * camera->speed.x * time;

    // Függőleges mozgás (pl. görgővel zoomolás)
    camera->position.z += camera->speed.z * time;
}

void set_view(const Camera* camera)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(-camera->rotation.x, 1.0, 0.0, 0.0); // Döntés fel-le
    glRotatef(-camera->rotation.y, 0.0, 0.0, 1.0); // Forgatás jobbra-balra
    glTranslatef(-camera->position.x, -camera->position.y, -camera->position.z);
}

void rotate_camera(Camera* camera, double horizontal, double vertical)
{
    camera->rotation.y += horizontal; // Balra-jobbra forgatás
    camera->rotation.x += vertical;   // Fel-le döntés

    // 360 fokos forgás megengedése vízszintesen
    if (camera->rotation.y < 0) {
        camera->rotation.y += 360.0;
    }
    if (camera->rotation.y > 360.0) {
        camera->rotation.y -= 360.0;
    }

    // Döntés korlátozása (-80 és +80 fok között)
    if (camera->rotation.x < -80.0) {
        camera->rotation.x = -80.0;
    }
    if (camera->rotation.x > 80.0) {
        camera->rotation.x = 80.0;
    }
}

void set_camera_speed(Camera* camera, double speed)
{
    camera->speed.y = speed;
}

void set_camera_side_speed(Camera* camera, double speed)
{
    camera->speed.x = speed;
}

void set_camera_vertical_speed(Camera* camera, double speed)
{
    camera->speed.z = speed;
}