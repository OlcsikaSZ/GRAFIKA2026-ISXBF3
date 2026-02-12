#ifndef CSV_H
#define CSV_H

#include <stddef.h>

typedef struct {
    char type[32];
    char model[256];
    char texture[256];
    float px, py, pz;
    float rx, ry, rz;
    float sx, sy, sz;
} SceneRow;

int load_scene_csv(const char* path, SceneRow* out_rows, size_t max_rows, size_t* out_count);

#endif
