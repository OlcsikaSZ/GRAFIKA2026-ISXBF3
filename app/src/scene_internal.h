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

void compute_model_bounds(const Model* m,
                          vec3* out_center,
                          float* out_radius,
                          float* out_minx, float* out_maxx,
                          float* out_miny, float* out_maxy,
                          float* out_minz, float* out_maxz);

void apply_transform(const Entity* e);
void draw_shadow_proxy_circle(float radius, int segments);
void set_lighting_with_intensity(float intensity);
void set_material(const Material* material);
void build_shadow_matrix(const float plane[4], const float light[4], float out[16]);
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
void draw_room_world_quads(const Scene* scene);
void draw_debug_axes_and_marker(void);

void set_entity_metadata(Entity* e, const char* model_path);
float find_nearest_pedestal(const Scene* scene, float x, float y, float z_hint);
int is_entity_collidable(const Entity* e);
int invert_matrix_4x4(const double m[16], double invOut[16]);
void mult_mat4_vec4(const double m[16], const double v[4], double out[4]);
void mult_mat4_mat4(const double a[16], const double b[16], double out[16]);
vec3 vec3_sub(vec3 a, vec3 b);
vec3 vec3_norm(vec3 v);
vec3 rotate_point_xyz_deg(vec3 p, float rx_deg, float ry_deg, float rz_deg);
int ray_sphere_intersect(vec3 ro, vec3 rd, vec3 c, float r, float* t_hit);

#endif
