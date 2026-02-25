#include "load.h"

#include <stdlib.h>
#include <string.h> /* strchr, memcpy */

#define LINE_BUFFER_SIZE 1024

int load_model(Model* model, const char* filename)
{
    FILE* obj_file;
    int success;

    obj_file = fopen(filename, "r");
    printf("Load model '%s' ...\n", filename);
    if (obj_file == NULL) {
        printf("ERROR: Unable to open '%s' file!\n", filename);
        return FALSE;
    }
    printf("Count the elements ...\n");
    count_elements(model, obj_file);
    printf("Allocate memory for model ...\n");
    allocate_model(model);
    printf("Read model data ...\n");
    fseek(obj_file, 0, SEEK_SET);
    success = read_elements(model, obj_file);
    if (success == FALSE) {
        printf("ERROR: Unable to read the model data!\n");
        return FALSE;
    }
    return TRUE;
}

static int count_face_points_in_line(const char* line)
{
    // Counts how many vertex references appear after 'f'.
    // Works for: f v, f v/vt, f v//vn, f v/vt/vn
    int count = 0;
    int i = 0;

    // find 'f'
    while (line[i] != 0 && line[i] != 'f') i++;
    if (line[i] == 0) return 0;
    i++;

    while (line[i] != 0) {
        // skip spaces
        while (line[i] == ' ' || line[i] == '\t') i++;
        if (line[i] == 0 || line[i] == '\n' || line[i] == '\r') break;
        // must start with a digit or '-' for an index
        if (is_numeric(line[i]) == TRUE) {
            count++;
            // skip token
            while (line[i] != 0 && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') i++;
        } else {
            // unknown token, skip
            while (line[i] != 0 && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') i++;
        }
    }
    return count;
}

void count_elements(Model* model, FILE* file)
{
    char line[LINE_BUFFER_SIZE];

    init_model(model);
    while (fgets(line, LINE_BUFFER_SIZE, file) != NULL) {
        switch (calc_element_type(line)) {
        case NONE:
            break;
        case VERTEX:
            ++model->n_vertices;
            break;
        case TEXTURE_VERTEX:
            ++model->n_texture_vertices;
            break;
        case NORMAL:
            ++model->n_normals;
            break;
        case FACE:
        {
            // Faces can be triangles or polygons. We triangulate by fan: (n-2) triangles.
            int n = count_face_points_in_line(line);
            if (n >= 3) {
                model->n_triangles += (n - 2);
            }
            break;
        }
        }
    }
}

static void set_default_slots(Model* model)
{
    // Index 0 is reserved (OBJ indices are 1-based in this loader).
    // Provide safe defaults so missing vt/vn won't crash draw.
    if (model->texture_vertices) {
        model->texture_vertices[0].u = 0.0;
        model->texture_vertices[0].v = 0.0;
    }
    if (model->normals) {
        model->normals[0].x = 0.0;
        model->normals[0].y = 0.0;
        model->normals[0].z = 1.0;
    }
    if (model->vertices) {
        model->vertices[0].x = 0.0;
        model->vertices[0].y = 0.0;
        model->vertices[0].z = 0.0;
    }
}

static int parse_face_point(FacePoint* out, const char* tok)
{
    // Parses: v, v/vt, v//vn, v/vt/vn
    // Missing vt/vn become 0.
    out->vertex_index = 0;
    out->texture_index = 0;
    out->normal_index = 0;

    if (!tok || *tok == 0) return FALSE;

    // v
    out->vertex_index = atoi(tok);

    const char* s1 = strchr(tok, '/');
    if (!s1) {
        return TRUE;
    }
    const char* s2 = strchr(s1 + 1, '/');

    // vt
    if (s2 == NULL) {
        // v/vt
        if (*(s1 + 1) != 0) out->texture_index = atoi(s1 + 1);
        return TRUE;
    }

    // v//vn or v/vt/vn
    if (s2 == s1 + 1) {
        // v//vn
        if (*(s2 + 1) != 0) out->normal_index = atoi(s2 + 1);
        return TRUE;
    }

    // v/vt/vn
    out->texture_index = atoi(s1 + 1);
    if (*(s2 + 1) != 0) out->normal_index = atoi(s2 + 1);
    return TRUE;
}

static int read_face_triangulated(Model* model, int* triangle_index_io, const char* text)
{
    // Triangulate a polygon face line into model->triangles.
    // We split by whitespace after 'f'.
    FacePoint pts[32];
    int n = 0;

    // Find the first token after 'f'
    const char* p = text;
    while (*p && *p != 'f') p++;
    if (*p == 0) return FALSE;
    p++;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0 || *p == '\n' || *p == '\r') break;

        // token start
        const char* start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        const int len = (int)(p - start);
        if (len <= 0) continue;
        if (n >= 32) break;

        char tok[128];
        int cpy = len;
        if (cpy > 127) cpy = 127;
        memcpy(tok, start, (size_t)cpy);
        tok[cpy] = 0;

        if (parse_face_point(&pts[n], tok) == FALSE) return FALSE;
        n++;
    }

    if (n < 3) return FALSE;

    // Fan triangulation: (0, i, i+1)
    for (int i = 1; i < n - 1; i++) {
        Triangle* tri = &model->triangles[*triangle_index_io];
        tri->points[0] = pts[0];
        tri->points[1] = pts[i];
        tri->points[2] = pts[i + 1];
        (*triangle_index_io)++;
    }
    return TRUE;
}

int read_elements(Model* model, FILE* file)
{
    char line[LINE_BUFFER_SIZE];
    int vertex_index;
    int texture_index;
    int normal_index;
    int triangle_index;
    int success;

    allocate_model(model);
    set_default_slots(model);
    vertex_index = 1;
    texture_index = 1;
    normal_index = 1;
    triangle_index = 0;
    while (fgets(line, LINE_BUFFER_SIZE, file) != NULL) {
        switch (calc_element_type(line)) {
        case NONE:
            break;
        case VERTEX:
            success = read_vertex(&(model->vertices[vertex_index]), line);
            if (success == FALSE) {
                printf("Unable to read vertex data!\n");
                return FALSE;
            }
            ++vertex_index;
            break;
        case TEXTURE_VERTEX:
            success = read_texture_vertex(&(model->texture_vertices[texture_index]), line);
            if (success == FALSE) {
                printf("Unable to read texture vertex data!\n");
                return FALSE;
            }
            ++texture_index;
            break;
        case NORMAL:
            success = read_normal(&(model->normals[normal_index]), line);
            if (success == FALSE) {
                printf("Unable to read normal vector data!\n");
                return FALSE;
            }
            ++normal_index;
            break;
        case FACE:
            success = read_face_triangulated(model, &triangle_index, line);
            if (success == FALSE) {
                printf("Unable to read face data!\n");
                return FALSE;
            }
            break;
        }
    }
    return TRUE;
}

ElementType calc_element_type(const char* text)
{
    int i;

    i = 0;
    while (text[i] != 0) {
        if (text[i] == 'v') {
            if (text[i + 1] == 't') {
                return TEXTURE_VERTEX;
            }
            else if (text[i + 1] == 'n') {
                return NORMAL;
            }
            else {
                return VERTEX;
            }
        }
        else if (text[i] == 'f') {
            return FACE;
        }
        else if (text[i] != ' ' && text[i] != '\t') {
            return NONE;
        }
        ++i;
    }
    return NONE;
}

int read_vertex(Vertex* vertex, const char* text)
{
    int i;

    i = 0;
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        vertex->x = atof(&text[i]);
    }
    else {
        printf("The x value of vertex is missing!\n");
        return FALSE;
    }
    while (text[i] != 0 && text[i] != ' ') {
        ++i;
    }
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        vertex->y = atof(&text[i]);
    }
    else {
        printf("The y value of vertex is missing!\n");
        return FALSE;
    }
    while (text[i] != 0 && text[i] != ' ') {
        ++i;
    }
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        vertex->z = atof(&text[i]);
    }
    else {
        printf("The z value of vertex is missing!\n");
        return FALSE;
    }
    return TRUE;
}

int read_texture_vertex(TextureVertex* texture_vertex, const char* text)
{
    int i;

    i = 0;
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        texture_vertex->u = atof(&text[i]);
    }
    else {
        printf("The u value of texture vertex is missing!\n");
        return FALSE;
    }
    while (text[i] != 0 && text[i] != ' ') {
        ++i;
    }
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        texture_vertex->v = atof(&text[i]);
    }
    else {
        printf("The v value of texture vertex is missing!\n");
        return FALSE;
    }
    return TRUE;
}

int read_normal(Vertex* normal, const char* text)
{
    int i;

    i = 0;
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        normal->x = atof(&text[i]);
    }
    else {
        printf("The x value of normal vector is missing!\n");
        return FALSE;
    }
    while (text[i] != 0 && text[i] != ' ') {
        ++i;
    }
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        normal->y = atof(&text[i]);
    }
    else {
        printf("The y value of normal vector is missing!\n");
        return FALSE;
    }
    while (text[i] != 0 && text[i] != ' ') {
        ++i;
    }
    while (text[i] != 0 && is_numeric(text[i]) == FALSE) {
        ++i;
    }
    if (text[i] != 0) {
        normal->z = atof(&text[i]);
    }
    else {
        printf("The z value of normal vector is missing!\n");
        return FALSE;
    }
    return TRUE;
}

// Old triangle-only reader removed; the loader now supports triangulating polygon faces.

int is_numeric(char c)
{
    if ((c >= '0' && c <= '9') || c == '-' || c == '.') {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
