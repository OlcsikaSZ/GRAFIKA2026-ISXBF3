#ifndef SCENE_INTERNAL_H
#define SCENE_INTERNAL_H

#include "scene.h"
#include "csv.h"

#include <obj/load.h>
#include <obj/draw.h>

#include <string.h>
#include <stdio.h>
#include <direct.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Room dimensions in world coordinates.
#define ROOM_W 10.0f
#define ROOM_L 26.0f
#define ROOM_H 4.0f

// Bounding helpers used for picking and collisions.
void compute_model_bounds(const Model* m,
                          vec3* out_center,
                          float* out_radius,
                          float* out_minx, float* out_maxx,
                          float* out_miny, float* out_maxy,
                          float* out_minz, float* out_maxz);

// Rendering and transform helpers.
void apply_transform(const Entity* e);
void draw_shadow_proxy_circle(const Entity* e);
void set_lighting_with_intensity(const Scene* scene);
void set_material(const Material* material);
void build_shadow_matrix(float out[16], const float plane[4], const float light[4]);
int entity_casts_shadow(const Entity* e);
int entity_is_transparent(const Entity* e);
void draw_entity_opaque(const Entity* e);
void draw_entity_glass(const Entity* e);
void render_planar_shadows(const Scene* scene);
void quad_world(float x1,float y1,float z1,
                float x2,float y2,float z2,
                float x3,float y3,float z3,
                float x4,float y4,float z4,
                float nx,float ny,float nz,
                float u_repeat,float v_repeat);
void draw_room_world_quads(GLuint floor_tex, GLuint wall_tex, GLuint ceiling_tex);
void draw_debug_axes_and_marker(void);

// Scene setup and metadata helpers.
void set_entity_metadata(Entity* e, const char* model_path);
int find_nearest_pedestal(const Scene* scene, const Entity* statue);
int is_entity_collidable(const Entity* e);

// Math helpers for picking and intersection tests.
int invert_matrix_4x4(const double m[16], double invOut[16]);
void mult_mat4_vec4(const double m[16], const double v[4], double out[4]);
void mult_mat4_mat4(const double a[16], const double b[16], double out[16]);
void vec3_sub(const double a[3], const double b[3], double out[3]);
void vec3_norm(double v[3]);
void rotate_point_xyz_deg(double p[3], float rx_deg, float ry_deg, float rz_deg);
int ray_sphere_intersect(const double ro[3], const double rd[3],
                         const double c[3], double r, double* t_hit);

#endif