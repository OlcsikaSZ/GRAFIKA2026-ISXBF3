#include "camera.h"

#include <GL/gl.h>

#include <math.h>

void init_camera(Camera* camera)
{
    camera->position.x = 0.0;
    camera->position.y = 0.0;
    // Kényelmes "szemmagasság" Z-up világban
    camera->position.z = 1.6;
    camera->rotation.x = 0.0;
    camera->rotation.y = 0.0;
    camera->rotation.z = 0.0;
    camera->speed.x = 0.0;
    camera->speed.y = 0.0;
    camera->speed.z = 0.0;

    camera->is_preview_visible = false;
}

// Egyszerű szobahatár (AABB) — MVP ütközés / falon átmenés tiltás
// Fontos: ez a scene.c-ben rajzolt szoba méreteivel van összhangban.
static void clamp_to_room(Camera* camera)
{
    const double room_half = 6.0;   // room_w = 12.0 -> fele
    const double wall_pad  = 0.25;  // ennyire maradjunk a faltól, hogy ne vágjon a near plane
    const double min_x = -room_half + wall_pad;
    const double max_x =  room_half - wall_pad;
    const double min_y = -room_half + wall_pad;
    const double max_y =  room_half - wall_pad;

    if (camera->position.x < min_x) camera->position.x = min_x;
    if (camera->position.x > max_x) camera->position.x = max_x;
    if (camera->position.y < min_y) camera->position.y = min_y;
    if (camera->position.y > max_y) camera->position.y = max_y;

    // Ne essünk a padló alá, és ne menjünk bele a plafonba.
    // A camera->position.z a "szem" magassága. Alapból 1.6-on áll (kellemes járás),
    // de engedjük "lehajolni" is, csak ne menjen át a padlón.
    const double floor_z_min = 0.20;     // padló felett (ne lássunk át alatta)
    const double ceil_z_max  = 4.0 - 0.30; // hagyjunk elég helyet a near-plane miatt (ne vágja le a plafont)

    if (camera->position.z < floor_z_min) camera->position.z = floor_z_min;
    if (camera->position.z > ceil_z_max)  camera->position.z = ceil_z_max;
}

void update_camera(Camera* camera, double time)
{
    double angle;
    double side_angle;

    angle = degree_to_radian(camera->rotation.z);
    side_angle = degree_to_radian(camera->rotation.z + 90.0);

    camera->position.x += cos(angle) * camera->speed.y * time;
    camera->position.y += sin(angle) * camera->speed.y * time;
    camera->position.x += cos(side_angle) * camera->speed.x * time;
    camera->position.y += sin(side_angle) * camera->speed.x * time;
    camera->position.z += camera->speed.z * time;

    clamp_to_room(camera);
}

void set_view(const Camera* camera)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(-(camera->rotation.x + 90), 1.0, 0, 0);
    glRotatef(-(camera->rotation.z - 90), 0, 0, 1.0);
    glTranslatef(-camera->position.x, -camera->position.y, -camera->position.z);
}

void rotate_camera(Camera* camera, double horizontal, double vertical)
{
    camera->rotation.z += horizontal;
    camera->rotation.x += vertical;

    if (camera->rotation.z < 0) {
        camera->rotation.z += 360.0;
    }

    if (camera->rotation.z > 360.0) {
        camera->rotation.z -= 360.0;
    }

    if (camera->rotation.x < 0) {
        camera->rotation.x += 360.0;
    }

    if (camera->rotation.x > 360.0) {
        camera->rotation.x -= 360.0;
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

void show_texture_preview()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(-1, 1, -3);
    glTexCoord2f(1, 0);
    glVertex3f(1, 1, -3);
    glTexCoord2f(1, 1);
    glVertex3f(1, -1, -3);
    glTexCoord2f(0, 1);
    glVertex3f(-1, -1, -3);
    glEnd();

    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}
