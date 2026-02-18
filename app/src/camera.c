#include "camera.h"

#include <GL/gl.h>

#include <math.h>

void init_camera(Camera* camera)
{
    camera->position.x = 0.0;
    camera->position.y = 0.0;
    // Kényelmes "szemmagasság" Z-up világban
    camera->position.z = 1.70;
    camera->rotation.x = 0.0;
    camera->rotation.y = 0.0;
    camera->rotation.z = 0.0;
    camera->speed.x = 0.0;
    camera->speed.y = 0.0;
    camera->speed.z = 0.0;

    camera->is_preview_visible = false;

    camera->walk_bob_enabled = false;
    camera->walk_phase = 0.0;
    camera->bob_offset = 0.0;
}

void toggle_walk_bob(Camera* camera)
{
    camera->walk_bob_enabled = !camera->walk_bob_enabled;
    // 'Ember mód': állítsuk be a szemmagasságot, és onnantól ne engedjünk repülni.
    if (camera->walk_bob_enabled) {
        const double eye = 1.75;
        camera->position.z = eye;
        camera->speed.z = 0.0;
    }
    camera->walk_phase = 0.0;
    camera->bob_offset = 0.0;
}

// Egyszerű szobahatár (AABB) — MVP ütközés / falon átmenés tiltás
// Fontos: ez a scene.c-ben rajzolt szoba méreteivel van összhangban.
static void clamp_to_room(Camera* camera)
{
    // A scene.c-ben rajzolt szoba mérete: room_half (felezett szélesség / hossz).
    // Itt ezt ugyanúgy kell tartani, különben "kirepülünk" a falakon.
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
    // Fontos: fly módban SE tudjunk átrepülni a padlón/plafonon.
    // "Ember" módban a padló minimuma magasabb (szemmagasság-érzet).
    const double floor_z_min = camera->walk_bob_enabled ? 1.55 : 0.25;
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
    // Séta módban ne legyen vertikális mozgás (ne repüljünk).
    if (!camera->walk_bob_enabled) {
        camera->position.z += camera->speed.z * time;
    }

    clamp_to_room(camera);

    // Walking head-bob (kizárólag vizuális, collision nem érintett)
    // Ha mozogsz X/Y-ban, akkor enyhe bólogatás.
    if (camera->walk_bob_enabled) {
        const double move_mag = fabs(camera->speed.x) + fabs(camera->speed.y);
        if (move_mag > 0.001) {
            // freki a mozgással arányos (kellemesebb érzet)
            const double freq = 6.0;      // rad/sec (kevésbé "ráz")
            const double amp  = 0.010;    // méter
            camera->walk_phase += time * freq;
            camera->bob_offset = sin(camera->walk_phase) * amp;
        } else {
            camera->bob_offset = 0.0;
        }
    } else {
        camera->bob_offset = 0.0;
    }
}

void set_view(const Camera* camera)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(-(camera->rotation.x + 90), 1.0, 0, 0);
    glRotatef(-(camera->rotation.z - 90), 0, 0, 1.0);
    glTranslatef(-camera->position.x, -camera->position.y, (float)(-(camera->position.z + camera->bob_offset)));
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
