#include "csv.h"
#include <stdio.h>
#include <string.h>

// Remove trailing line breaks from a CSV line.
static void trim_newline(char* s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[n-1] = 0;
        n--;
    }
}

// Load scene entities from the CSV configuration file.
int load_scene_csv(const char* path, SceneRow* out_rows, size_t max_rows, size_t* out_count) {
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[1024];
    size_t count = 0;

    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    while (fgets(line, sizeof(line), f)) {
        if (count >= max_rows) break;
        trim_newline(line);

        SceneRow r;
        memset(&r, 0, sizeof(r));

        int ok = sscanf(line,
            " %31[^,],%255[^,],%255[^,],%f,%f,%f,%f,%f,%f,%f,%f,%f",
            r.type, r.model, r.texture,
            &r.px, &r.py, &r.pz,
            &r.rx, &r.ry, &r.rz,
            &r.sx, &r.sy, &r.sz
        );

        if (ok == 12) {
            out_rows[count++] = r;
        }
    }

    fclose(f);
    *out_count = count;
    return 1;
}
