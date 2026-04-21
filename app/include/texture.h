#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/gl.h>

typedef GLubyte Pixel[3];

// Load a texture from file and return its OpenGL id.
GLuint load_texture(char* filename);

#endif /* TEXTURE_H */
