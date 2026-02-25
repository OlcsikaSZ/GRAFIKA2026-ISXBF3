#include "draw.h"

#include <GL/gl.h>

void draw_model(const Model* model)
{
    draw_triangles(model);
}

void draw_triangles(const Model* model)
{
    int i, k;
    int vertex_index, texture_index, normal_index;
    float x, y, z, u, v;

    glBegin(GL_TRIANGLES);

    for (i = 0; i < model->n_triangles; ++i) {
        for (k = 0; k < 3; ++k) {

            normal_index = model->triangles[i].points[k].normal_index;
            if (model->n_normals > 0 && normal_index > 0 && normal_index <= model->n_normals) {
                x = model->normals[normal_index].x;
                y = model->normals[normal_index].y;
                z = model->normals[normal_index].z;
            } else {
                // Safe default normal (Z-up)
                x = 0.0f; y = 0.0f; z = 1.0f;
            }
            glNormal3f(x, y, z);

            texture_index = model->triangles[i].points[k].texture_index;
            if (model->n_texture_vertices > 0 && texture_index > 0 && texture_index <= model->n_texture_vertices) {
                u = model->texture_vertices[texture_index].u;
                v = model->texture_vertices[texture_index].v;
            } else {
                u = 0.0f; v = 0.0f;
            }
            glTexCoord2f(u, 1.0f - v);

            vertex_index = model->triangles[i].points[k].vertex_index;
            if (vertex_index > 0 && vertex_index <= model->n_vertices) {
                x = model->vertices[vertex_index].x;
                y = model->vertices[vertex_index].y;
                z = model->vertices[vertex_index].z;
                glVertex3f(x, y, z);
            } else {
                // Skip invalid vertex
                glVertex3f(0.0f, 0.0f, 0.0f);
            }
        }
    }

    glEnd();
}
