#ifndef GLUTIL_H
#define GLUTIL_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#ifndef WIN32
#include <GL/glx.h>
#endif
#include "mafs.h"

#define glerr() _check_gl_error(__FILE__,__LINE__)
void _check_gl_error(const char *file, int line);

void glInit(void);

/* #define glUnlock() _glUnlock(__FILE__, __LINE__) */
/* void _glUnlock(const char *file, int line); */
/* #define glLock() _glLock(__FILE__, __LINE__) */
/* void _glLock(const char *file, int line); */

#endif /* !GLUTIL_H */
