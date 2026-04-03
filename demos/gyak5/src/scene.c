#include "scene.h"

#include <GL/gl.h>
#include <math.h>

#define BOARD_SIZE 8
#define CYLINDER_SIDES 20
#define PI 3.141592653589793

static float sphere_rotation = 0.0f;

void init_scene(Scene* scene)
{
}

void update_scene(Scene* scene)
{
    update_sphere_rotation(elapsed_time);
}

void render_scene(const Scene* scene)
{
    draw_colored_triangle();
    draw_gray_sphere();
    draw_origin();
    draw_chessboard();
    draw_cylinder();
    draw_cone();
glBegin(GL_QUADS);

glColor3f(1, 0, 0);
glVertex3f(0, 0, 0);
glVertex3f(0, 0, 1);
glVertex3f(1, 0, 1);
glVertex3f(1, 0, 0);

glColor3f(0, 1, 0);
glVertex3f(0, 0, 0);
glVertex3f(0, 0, 1);
glVertex3f(1, 0, 1);
glVertex3f(1, 0, 0);

glEnd();

}

void draw_origin()
{
    glBegin(GL_LINES);

    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);

    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);

    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);

    glEnd();
}

void draw_colored_triangle()
{
    glBegin(GL_TRIANGLES);
    
    glColor3f(1.0, 1.0, 0.0); // Sárga
    glVertex3f(-0.5, -0.5, 0.0);

    glColor3f(0.0, 1.0, 1.0); // Cián
    glVertex3f(0.5, -0.5, 0.0);

    glColor3f(1.0, 0.0, 1.0); // Magenta
    glVertex3f(0.0, 0.5, 0.0);

    glEnd();
}

void draw_gray_sphere()
{
    GLUquadric* quad = gluNewQuadric();
    glColor3f(0.5, 0.5, 0.5); // Szürke szín

    glPushMatrix();
    glTranslatef(0.0, 0.0, 0.0); // Középre helyezzük az egységkockában

    // FORGATÁS Y tengely körül
    glRotatef(sphere_rotation, 0.0, 1.0, 0.0);

    gluSphere(quad, 0.3, 10, 10); // Sugár: 0.3, tesszelláció: 10x10
    glPopMatrix();

    gluDeleteQuadric(quad);
}


void update_sphere_rotation(double elapsed_time)
{
    sphere_rotation += 30.0 * elapsed_time; // 30 fok / másodperc sebességgel forog
    if (sphere_rotation > 360.0) {
        sphere_rotation -= 360.0;
    }
}

void draw_chessboard()
{
    float tileSize = 1.0 / BOARD_SIZE;  // Sakktábla méret normalizálása
    int i, j;

    glPushMatrix();
    glTranslatef(-0.5, -0.5, 0.0);  // Középre helyezés

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            if ((i + j) % 2 == 0) {
                glColor3f(1.0, 1.0, 1.0);  // Fehér mező
            } else {
                glColor3f(0.0, 0.0, 0.0);  // Fekete mező
            }

            glBegin(GL_QUADS);
            glVertex3f(i * tileSize, j * tileSize, 0.0);
            glVertex3f((i + 1) * tileSize, j * tileSize, 0.0);
            glVertex3f((i + 1) * tileSize, (j + 1) * tileSize, 0.0);
            glVertex3f(i * tileSize, (j + 1) * tileSize, 0.0);
            glEnd();
        }
    }

    glPopMatrix();
}

void draw_cylinder()
{
    float radius = 0.3;
    float height = 1.0;
    int i;

    glPushMatrix();
    glTranslatef(0.0, 0.0, -0.5);  // Középre helyezés

    glColor3f(0.7, 0.7, 0.7);  // Szürke szín

    glBegin(GL_TRIANGLE_STRIP);
    for (i = 0; i <= CYLINDER_SIDES; i++) {
        float angle = 2.0 * PI * i / CYLINDER_SIDES;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;

        glVertex3f(x, y, 0.0);
        glVertex3f(x, y, height);
    }
    glEnd();

    glPopMatrix();
}

void draw_cone()
{
    float radius = 0.3;
    float height = 0.5;
    int i;

    glPushMatrix();
    glTranslatef(0.0, 0.0, 0.5);  // Középre helyezés

    glColor3f(1.0, 0.5, 0.0);  // Narancssárga szín

    // Alap
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, 0.0);  // Középpont
    for (i = 0; i <= CYLINDER_SIDES; i++) {
        float angle = 2.0 * PI * i / CYLINDER_SIDES;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        glVertex3f(x, y, 0.0);
    }
    glEnd();

    // Oldal
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, height);  // Csúcs
    for (i = 0; i <= CYLINDER_SIDES; i++) {
        float angle = 2.0 * PI * i / CYLINDER_SIDES;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        glVertex3f(x, y, 0.0);
    }
    glEnd();

    glPopMatrix();
}