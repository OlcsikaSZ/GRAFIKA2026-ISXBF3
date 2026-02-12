#include "texture.h"

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

GLuint load_texture(char* filename)
{
    SDL_Surface* loaded = IMG_Load(filename);
    if (!loaded) {
        printf("[ERROR] IMG_Load failed (%s): %s\n", filename, IMG_GetError());
        return 0;
    }

    // Biztos RGBA formátum (stabil minden jpg/png esetén)
    SDL_Surface* surface = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loaded);

    if (!surface) {
        printf("[ERROR] SDL_ConvertSurfaceFormat failed (%s): %s\n", filename, SDL_GetError());
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        surface->w, surface->h,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        surface->pixels
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(surface);
    return tex;
}