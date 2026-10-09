// Stub de DECLARACOES apenas.
#pragma once
#include <cstddef>
#include <cstdint>
typedef unsigned int GLenum, GLuint, GLbitfield; typedef int GLint, GLsizei; typedef unsigned char GLboolean, GLubyte; typedef float GLfloat; typedef double GLdouble; typedef char GLchar; typedef ptrdiff_t GLsizeiptr; typedef void GLvoid;
enum : unsigned { GL_ALPHA_TEST = 2 };
enum : unsigned { GL_AMBIENT_AND_DIFFUSE = 3 };
enum : unsigned { GL_ARRAY_BUFFER = 4 };
enum : unsigned { GL_ATI_ = 5 };
enum : unsigned { GL_BLEND = 6 };
enum : unsigned { GL_CLAMP = 7 };
enum : unsigned { GL_CLAMP_TO_EDGE = 8 };
enum : unsigned { GL_COLOR_ARRAY = 9 };
enum : unsigned { GL_COLOR_BUFFER_BIT = 10 };
enum : unsigned { GL_COLOR_MATERIAL = 11 };
enum : unsigned { GL_COMPILE = 12 };
enum : unsigned { GL_COMPILE_STATUS = 13 };
enum : unsigned { GL_CONSTANT_ALPHA = 14 };
enum : unsigned { GL_CULL_FACE = 15 };
enum : unsigned { GL_DEPTH_BUFFER_BIT = 16 };
enum : unsigned { GL_STENCIL_BUFFER_BIT = 1024, GL_STENCIL_TEST = 0xB90, GL_ALWAYS = 0x207, GL_NOTEQUAL = 0x205, GL_KEEP = 0x1E00, GL_REPLACE = 0x1E01 };
void glStencilFunc(unsigned, int, unsigned); void glStencilOp(unsigned, unsigned, unsigned); void glClearStencil(int);
enum : unsigned { GL_DEPTH_TEST = 17 };
enum : unsigned { GL_EXTENSIONS = 18 };
enum : unsigned { GL_FALSE = 19 };
enum : unsigned { GL_FLOAT = 20 };
enum : unsigned { GL_FOG = 21 };
enum : unsigned { GL_FOG_COLOR = 22 };
enum : unsigned { GL_FOG_END = 23 };
enum : unsigned { GL_FOG_HINT = 24 };
enum : unsigned { GL_FOG_MODE = 25 };
enum : unsigned { GL_FOG_START = 26 };
enum : unsigned { GL_FRAGMENT_SHADER = 27 };
enum : unsigned { GL_FRONT_AND_BACK = 28 };
enum : unsigned { GL_GREATER = 29 };
enum : unsigned { GL_LEQUAL = 30 };
enum : unsigned { GL_LIGHTING = 31 };
enum : unsigned { GL_LIGHT_MODEL_AMBIENT = 32 };
enum : unsigned { GL_LINEAR = 33 };
enum : unsigned { GL_LINES = 34 };
enum : unsigned { GL_LINK_STATUS = 35 };
enum : unsigned { GL_MODELVIEW = 36 };
enum : unsigned { GL_MODELVIEW_MATRIX = 37 };
enum : unsigned { GL_NEAREST = 38 };
enum : unsigned { GL_NICEST = 39 };
enum : unsigned { GL_NVX_ = 40 };
enum : unsigned { GL_ONE = 41 };
enum : unsigned { GL_ONE_MINUS_CONSTANT_ALPHA = 42 };
enum : unsigned { GL_ONE_MINUS_DST_COLOR = 43 };
enum : unsigned { GL_ONE_MINUS_SRC_ALPHA = 44 };
enum : unsigned { GL_ONE_MINUS_SRC_COLOR = 45 };
enum : unsigned { GL_POLYGON_OFFSET_FILL = 46 };
enum : unsigned { GL_PROJECTION = 47 };
enum : unsigned { GL_QUADS = 48 };
enum : unsigned { GL_RENDERER = 49 };
enum : unsigned { GL_REPEAT = 50 };
enum : unsigned { GL_RGBA = 51 };
enum : unsigned { GL_SCISSOR_TEST = 52 };
enum : unsigned { GL_SRC_ALPHA = 53 };
enum : unsigned { GL_STATIC_DRAW = 54 };
enum : unsigned { GL_TEXTURE_2D = 55 };
enum : unsigned { GL_TEXTURE_COORD_ARRAY = 56 };
enum : unsigned { GL_TEXTURE_MAG_FILTER = 57 };
enum : unsigned { GL_TEXTURE_MIN_FILTER = 58 };
enum : unsigned { GL_TEXTURE_WRAP_S = 59 };
enum : unsigned { GL_TEXTURE_WRAP_T = 60 };
enum : unsigned { GL_TRIANGLE_FAN = 61 };
enum : unsigned { GL_TRIANGLE_STRIP = 62 };
enum : unsigned { GL_TRUE = 63 };
enum : unsigned { GL_UNPACK_ALIGNMENT = 64 };
enum : unsigned { GL_UNSIGNED_BYTE = 65 };
enum : unsigned { GL_VENDOR = 66 };
enum : unsigned { GL_VERSION = 67 };
enum : unsigned { GL_VERTEX_ARRAY = 68 };
enum : unsigned { GL_VERTEX_SHADER = 69 };
enum : unsigned { GL_VIEWPORT = 70 };
extern "C++" {
void glAlphaFunc(GLenum, GLfloat); void glBegin(GLenum); void glEnd(); void glBindTexture(GLenum, GLuint); void glBlendFunc(GLenum, GLenum);
void glCallList(GLuint); void glClear(GLbitfield); void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat); void glColor3f(GLfloat, GLfloat, GLfloat); void glColor4f(GLfloat, GLfloat, GLfloat, GLfloat);
void glColorMask(GLboolean, GLboolean, GLboolean, GLboolean); void glColorMaterial(GLenum, GLenum); void glColorPointer(GLint, GLenum, GLsizei, const void*);
void glDepthFunc(GLenum); void glDepthMask(GLboolean); void glDisable(GLenum); void glEnable(GLenum); void glDisableClientState(GLenum); void glEnableClientState(GLenum);
void glDrawArrays(GLenum, GLint, GLsizei); void glDrawElements(GLenum, GLsizei, GLenum, const void*); void glEndList(); void glNewList(GLuint, GLenum); GLuint glGenLists(GLsizei);
void glFogf(GLenum, GLfloat); void glFogfv(GLenum, const GLfloat*); void glFogi(GLenum, GLint); void glFrustum(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
void glGenTextures(GLsizei, GLuint*); void glGetFloatv(GLenum, GLfloat*); void glGetIntegerv(GLenum, GLint*); const GLubyte* glGetString(GLenum);
void glHint(GLenum, GLenum); GLboolean glIsEnabled(GLenum); void glLightModelfv(GLenum, const GLfloat*); void glLineWidth(GLfloat); void glLoadIdentity(); void glMatrixMode(GLenum);
void glOrtho(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble); void glPixelStorei(GLenum, GLint); void glPolygonOffset(GLfloat, GLfloat); void glPopMatrix(); void glPushMatrix();
void glRotatef(GLfloat, GLfloat, GLfloat, GLfloat); void glScalef(GLfloat, GLfloat, GLfloat); void glScissor(GLint, GLint, GLsizei, GLsizei);
void glTexCoord2f(GLfloat, GLfloat); void glTexCoordPointer(GLint, GLenum, GLsizei, const void*); void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void glTexParameteri(GLenum, GLenum, GLint); void glTranslatef(GLfloat, GLfloat, GLfloat); void glVertex2f(GLfloat, GLfloat); void glVertex3d(GLdouble, GLdouble, GLdouble); void glVertex3f(GLfloat, GLfloat, GLfloat);
void glVertexPointer(GLint, GLenum, GLsizei, const void*); void glViewport(GLint, GLint, GLsizei, GLsizei); void glBlendColor(GLfloat, GLfloat, GLfloat, GLfloat);
}
extern "C++" {
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint); typedef void (*PFNGLUNIFORM1FPROC)(GLint, GLfloat); typedef void (*PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum); typedef void (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*); typedef void (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*); typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*); typedef GLuint (*PFNGLCREATEPROGRAMPROC)();
typedef void (*PFNGLATTACHSHADERPROC)(GLuint, GLuint); typedef void (*PFNGLLINKPROGRAMPROC)(GLuint); typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*); typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*); }
extern "C++" {
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint*); typedef void (*PFNGLBINDBUFFERPROC)(GLenum, GLuint); typedef void (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (*PFNGLBLENDCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat); typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
}

unsigned glGetError(); enum { GL_NO_ERROR = 0 }; void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
void glDeleteTextures(GLsizei, const GLuint*);
enum { GL_TEXTURE_BINDING_2D = 0x8069 };

#ifndef APIENTRY
#define APIENTRY

// --- acrescentados: usados pelo main_loop.inc (stubs antigos nao os declaravam) ---
#define GL_ALL_ATTRIB_BITS 0xFFFFFFFF
#define GL_CLIENT_ALL_ATTRIB_BITS 0xFFFFFFFF
void glPushAttrib(unsigned); void glPopAttrib(); void glPushClientAttrib(unsigned); void glPopClientAttrib(); void glClipPlane(GLenum, const GLdouble*);
#endif
void glLoadMatrixf(const GLfloat*); void glDrawBuffer(GLenum); void glReadBuffer(GLenum);
