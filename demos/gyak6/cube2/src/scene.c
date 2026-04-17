#include "scene.h"

#include <obj/load.h>
#include <obj/draw.h>
#include <math.h>

void init_scene(Scene* scene)
{
    // load_model(&(scene->cube), "assets/models/cube.obj");
    // scene->texture_id = load_texture("assets/textures/cube.png");
    load_model(&(scene->cube), "assets/models/cat.obj");
    scene->texture_id = load_texture("assets/textures/cube.png");

    glBindTexture(GL_TEXTURE_2D, scene->texture_id);

    scene->material.ambient.red = 0.0;
    scene->material.ambient.green = 0.0;
    scene->material.ambient.blue = 0.0;

    scene->material.diffuse.red = 1.0;
    scene->material.diffuse.green = 1.0;
    scene->material.diffuse.blue = 0.0;

    scene->material.specular.red = 0.0;
    scene->material.specular.green = 0.0;
    scene->material.specular.blue = 0.0;

    scene->material.shininess = 0.0;
    scene->light_intensity = 1.0f;
}

void set_lighting(Scene* scene, float t)
{

    float ambient_light[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float diffuse_light[] = {
        light_intensity, light_intensity, light_intensity, 1.0f
    };
    float specular_light[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float position[] = { sinf(t) * 10.0f, 5.0f, cosf(t) * 10.0f, 1.0f };

    float diffuse_light[] = {
        fabs(sinf(t)) * scene->light_intensity,
        fabs(sinf(t + 2.0f)) * scene->light_intensity,
        fabs(sinf(t + 4.0f)) * scene->light_intensity,
        1.0f
    };

    float mat_specular[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    float mat_shininess[] = { 128.0f };  // maximum csillogás

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_light);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

void set_material(const Material* material)
{
    float ambient_material_color[] = {
        material->ambient.red,
        material->ambient.green,
        material->ambient.blue
    };

    float diffuse_material_color[] = {
        material->diffuse.red,
        material->diffuse.green,
        material->diffuse.blue
    };

    float specular_material_color[] = {
        material->specular.red,
        material->specular.green,
        material->specular.blue
    };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_material_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_material_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_material_color);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &(material->shininess));
}

void update_scene(Scene* scene)
{
}

void render_scene(const Scene* scene)
{
    set_material(&(scene->material));
    draw_origin();
    draw_model(&(scene->cube));
    set_lighting(scene, SDL_GetTicks() / 1000.0f);
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
