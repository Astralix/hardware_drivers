/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#ifndef _gc_glsh_database_h
#define _gc_glsh_database_h

typedef struct glsDATABASE *        glsDATABASE_PTR;
typedef struct glsDATABASE_RECORD * glsDATABASE_RECORD_PTR;

typedef struct glsDATABASE_RECORD
{
    /* Pointer to next record pool in the linked list. */
    glsDATABASE_RECORD_PTR  next;

    /* Current offset into data pool. */
    gctUINT                 offset;

    /* Data pool. */
    gctUINT8                data[32768];
}
glsDATABASE_RECORD;

typedef enum gleDATABASE_OPCODE
{
    glvDATABASE_OPCODE_UNKNWON = 0,
    glvDATABASE_OPCODE_SCISSORS,
    glvDATABASE_OPCODE_POLYGON,
    glvDATABASE_OPCODE_BLEND,
    glvDATABASE_OPCODE_PIXEL,
    glvDATABASE_OPCODE_DEPTH,
    glvDATABASE_OPCODE_STENCIL,
    glvDATABASE_OPCODE_CLEAR,
    glvDATABASE_OPCODE_DRAW_ELEMENTS,
    glvDATABASE_OPCODE_DRAW_ARRAYS,
    glvDATABASE_OPCODE_TEXTURE,
    glvDATABASE_OPCODE_PROGRAM,
    glvDATABASE_OPCODE_UNIFORM,
    glvDATABASE_OPCODE_FLUSH,
    glvDATABASE_OPCODE_FINISH,
}
gleDATABASE_OPCODE;

typedef union gluELEMENT
{
    const GLvoid *      ptr;
    const GLubyte *     ptr8;
    const GLushort *    ptr16;
    const GLuint *      ptr32;
}
gluELEMENT;

typedef struct glsDATABASE_BOX
{
    /* X origin of the box. */
    GLint   x;

    /* Y origin of the box. */
    GLint   y;

    /* Width of the box. */
    GLuint  width;

    /* Height of the box. */
    GLuint  height;
}
glsDATABASE_BOX;

#define GLDB_SCISSOR_ENABLE             (1 << 0)
#define GLDB_SCISSOR_BOX                (1 << 1)

typedef struct glsDATABASE_SCISSORS
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty flags. */
    GLbitfield          flags;

    /* Scissors enable flag. */
    GLboolean           enable;

    /* Scissors. */
    glsDATABASE_BOX     box;
}
glsDATABASE_SCISSORS;


typedef struct glsDATABASE_CLEAR
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Clear masks. */
    GLbitfield          mask;

    /* Clear color value. */
    GLclampf            red, green, blue, alpha;

    /* Clear depth value. */
    GLclampf            depth;

    /* Clear stencil value. */
    GLint               stencil;
}
glsDATABASE_CLEAR;

#define GLDB_POLYGON_CULL_ENABLE        (1 << 0)
#define GLDB_POLYGON_CULL_MODE          (1 << 1)
#define GLDB_POLYGON_CULL_FRONT         (1 << 2)
#define GLDB_POLYGON_LINE_WIDTH         (1 << 3)
#define GLDB_POLYGON_VIEWPORT           (1 << 4)
#define GLDB_POLYGON_DEPTH_RANGE        (1 << 5)
#define GLDB_POLYGON_OFFSET_ENABLE      (1 << 6)
#define GLDB_POLYGON_OFFSET             (1 << 7)

typedef struct glsDATABASE_POLYGON
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty flags. */
    GLbitfield          flags;

    /* Cull enable flag. */
    GLboolean           cullEnable;

    /* Cull mode. */
    GLenum              cullMode;

    /* Cull front type. */
    GLenum              cullFront;

    /* Line width. */
    GLint               lineWidth;

    /* Viewport box. */
    glsDATABASE_BOX     viewport;

    /* Depth near and far values. */
    GLclampf            near;
    GLclampf            far;

    /* Polygon offset enable flag. */
    GLboolean           offsetEnable;

    /* Polygon offset scale and bias. */
    GLfloat             factor;
    GLfloat             units;
}
glsDATABASE_POLYGON;

#define GLDB_BLEND_ENABLE               (1 << 0)
#define GLDB_BLEND_FUNCTION             (1 << 1)
#define GLDB_BLEND_MODE                 (1 << 2)
#define GLDB_BLEND_COLOR                (1 << 3)

typedef struct glsDATABASE_BLEND
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty flags. */
    GLbitfield          flags;

    /* Blend enable flag. */
    GLboolean           enable;

    /* Source and destination blending functions for RGB and alpha. */
    GLenum              funcSourceRGB;
    GLenum              funcSourceAlpha;
    GLenum              funcDestinationRGB;
    GLenum              funcDestinationAlpha;

    /* Blend mode for RGB and alpha. */
    GLenum              modeRGB;
    GLenum              modeAlpha;

    /* Blend color. */
    GLclampf            red;
    GLclampf            green;
    GLclampf            blue;
    GLclampf            alpha;
}
glsDATABASE_BLEND;

#define GLDB_PIXEL_ENABLE               (1 << 0)
#define GLDB_PIXEL_DITHER               (1 << 1)

typedef struct glsDATABASE_PIXEL
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty flags. */
    GLbitfield          flags;

    /* Color enable flags. */
    GLboolean           red;
    GLboolean           green;
    GLboolean           blue;
    GLboolean           alpha;

    /* Dither flag. */
    GLboolean           dither;
}
glsDATABASE_PIXEL;

#define GLDB_DEPTH_TEST                 (1 << 0)
#define GLDB_DEPTH_FUNC                 (1 << 1)
#define GLDB_DEPTH_MASK                 (1 << 2)

typedef struct glsDATABASE_DEPTH
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty flags. */
    GLbitfield          flags;

    /* Depth test enable flag. */
    GLboolean           test;

    /* Depth function. */
    GLenum              func;

    /* Depth write enable. */
    GLboolean           mask;
}
glsDATABASE_DEPTH;

#define GLDB_STENCIL_TEST               (1 << 0)
#define GLDB_STENCIL_FRONT_FUNCTION     (1 << 1)
#define GLDB_STENCIL_FRONT_MASK         (1 << 2)
#define GLDB_STENCIL_FRONT_OPERATION    (1 << 3)
#define GLDB_STENCIL_BACK_FUNCTION      (1 << 4)
#define GLDB_STENCIL_BACK_MASK          (1 << 5)
#define GLDB_STENCIL_BACK_OPERATION     (1 << 6)

typedef struct glsDATABASE_STENCIL
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Dirty bits. */
    GLbitfield          flags;

    /* Stencil test enable flag. */
    GLboolean           test;

    /* Stencil function for front. */
    GLenum              frontFunction;

    /* Stencil reference values for front. */
    GLuint              frontReference;

    /* Stencil mask for front. */
    GLuint              frontMask;

    /* Stencil operations for front. */
    GLenum              frontOpFail;
    GLenum              frontOpDepthFail;
    GLenum              frontOpDepthPass;

    /* Stencil function for back. */
    GLenum              backFunction;

    /* Stencil reference values for back. */
    GLuint              backReference;

    /* Stencil mask for back. */
    GLuint              backMask;

    /* Stencil operations for back. */
    GLenum              backOpFail;
    GLenum              backOpDepthFail;
    GLenum              backOpDepthPass;
}
glsDATABASE_STENCIL;

typedef struct glsDATABASE_VERTEX
{
    /* Bound vertex array. */
    gcsVERTEXARRAY      vertexArray;

    /* Number of bytes if this is local data. */
    gctSIZE_T           bytes;

    /* Offset into the stream based on the first vertex. */
    gctUINT             offset;
}
glsDATABASE_VERTEX;

typedef struct glsDATABASE_DRAW_ELEMENTS
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Primitive mode. */
    GLenum              mode;

    /* Number of indicies to draw. */
    GLsizei             count;

    /* Index type. */
    GLenum              type;

    /* Offset into index buffer. */
    const GLvoid *      indices;

    /* Bound element array buffer. */
    GLBuffer            elementArrayBuffer;

    /* Number of bytes following this record if the element data is local. */
    gctSIZE_T           elementBytes;

    /* Bound vertex arrays. */
    glsDATABASE_VERTEX  vertices[16];
}
glsDATABASE_DRAW_ELEMENTS;

typedef struct glsDATABASE_DRAW_ARRAYS
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Primitive mode. */
    GLenum              mode;

    /* Starting vertex. */
    GLint               first;

    /* Number of vertices to draw. */
    GLsizei             count;

    /* Bound vertex arrays. */
    glsDATABASE_VERTEX  vertices[16];
}
glsDATABASE_DRAW_ARRAYS;

#define GLDB_TEXTURE_OBJECT         (1 << 0)
#define GLDB_TEXTURE_MIN_FILTER     (1 << 1)
#define GLDB_TEXTURE_MAG_FILTER     (1 << 2)
#define GLDB_TEXTURE_ANISO_FILTER   (1 << 3)
#define GLDB_TEXTURE_WRAP_S         (1 << 4)
#define GLDB_TEXTURE_WRAP_T         (1 << 5)
#define GLDB_TEXTURE_WRAP_R         (1 << 6)
#define GLDB_TEXTURE_MAX_LEVEL      (1 << 7)

typedef struct glsDATABASE_TEXTURE
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Flags. */
    GLbitfield          flags;

    /* Bound sampler number. */
    GLint               sampler;

    /* Texture object. */
    GLTexture           texture;

    /* Texture states. */
    GLenum              minFilter;
    GLenum              magFilter;
    GLenum              anisoFilter;
    GLenum              wrapS;
    GLenum              wrapT;
    GLenum              wrapR;
    GLenum              maxLevel;
}
glsDATABASE_TEXTURE;

typedef struct glsDATABASE_PROGRAM
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Current program. */
    GLProgram           program;
}
glsDATABASE_PROGRAM;

typedef struct glsDATABASE_UNIFORM
{
    /* Opcode. */
    gleDATABASE_OPCODE  opcode;

    /* Uniform. */
    gctINT              uniform;

    /* Number of data bytes. */
    gctSIZE_T           bytes;
}
glsDATABASE_UNIFORM;

typedef struct glsDATABASE_STATES
{
    /* Scissors. */
    glsDATABASE_SCISSORS    scissors;

    /* Polygon offset. */
    glsDATABASE_POLYGON     polygon;

    /* Blending. */
    glsDATABASE_BLEND       blending;

    /* Pixel. */
    glsDATABASE_PIXEL       pixel;

    /* Depth. */
    glsDATABASE_DEPTH       depth;

    /* Stencil. */
    glsDATABASE_STENCIL     stencil;
}
glsDATABASE_STATES;

typedef struct glsDATABASE_DEPENDENCY * glsDATABASE_DEPENDENCY_PTR;
typedef struct glsDATABASE_DEPENDENCY
{
    /* Pointer to next dependency structure. */
    glsDATABASE_DEPENDENCY_PTR  next;

    /* Number of dependencies. */
    gctINT                      count;

    /* List of dependencies. */
    glsDATABASE_PTR             dependencies[4];
}
glsDATABASE_DEPENDENCY;

typedef struct glsDATABASE_PROGRAM_ARRAY * glsDATABASE_PROGRAM_ARRAY_PTR;
typedef struct glsDATABASE_PROGRAM_ARRAY
{
    /* Next program array in the link. */
    glsDATABASE_PROGRAM_ARRAY_PTR   next;

    /* Array of programs. */
    GLProgram                       programs[128];
}
glsDATABASE_PROGRAM_ARRAY;

typedef struct glsDATABASE_TEXTURE_ARRAY * glsDATABASE_TEXTURE_ARRAY_PTR;
typedef struct glsDATABASE_TEXTURE_ARRAY
{
    /* Next texture array in the link. */
    glsDATABASE_TEXTURE_ARRAY_PTR   next;

    /* Array of textures. */
    GLTexture                       textures[128];
}
glsDATABASE_TEXTURE_ARRAY;

typedef struct glsDATABASE
{
    /* Owner of the database. */
    GLFramebuffer                   owner;

    /* Linked list of databases. */
    glsDATABASE_PTR                 prev;
    glsDATABASE_PTR                 next;

    /* Reference counter. */
    gctINT                          reference;

    /* Initial clear record. */
    glsDATABASE_CLEAR               clear;

    /* Initial state record. */
    glsDATABASE_STATES              initialStates;

    /* Current state record. */
    glsDATABASE_STATES              currentStates;

    /* List of programs. */
    glsDATABASE_PROGRAM_ARRAY_PTR   programs;

    /* Current program. */
    GLProgram                       currentProgram;

    /* List of textures. */
    glsDATABASE_TEXTURE_ARRAY_PTR   textures;

    /* Dependencies. */
    glsDATABASE_DEPENDENCY_PTR      dependencies;

    /* Pointer to first data pool in the linked list. */
    glsDATABASE_RECORD_PTR          first;

    /* Pointer to last data pool in the linked list. */
    glsDATABASE_RECORD_PTR          last;

    /* Size of the database. */
    gctSIZE_T                       size;

    /* Current play offset. */
    gctUINT                         playOffset;

    /* Last draw elements. */
    glsDATABASE_DRAW_ELEMENTS *     lastDrawElements;

    /* Counter. */
    gctUINT                         counter;
}
glsDATABASE;

/* Functions. */
gceSTATUS glshAddClear(GLContext Context, GLbitfield Mask);
gceSTATUS glshAddDrawArrays(GLContext Context, GLenum Mode, GLint First,
                            GLsizei Count);
gceSTATUS glshAddDrawElements(GLContext Context, GLenum Mode, GLsizei Count,
                              GLenum Type, const GLvoid * Indices);
gceSTATUS glshAddFinish(GLContext Context);
gceSTATUS glshAddFlush(GLContext Context);
gceSTATUS glshCheckDependency(GLContext Context);
gceSTATUS glshCleanDatabase(glsDATABASE_PTR Database);
gceSTATUS glshPlayDatabase(GLContext Context, glsDATABASE_PTR Database);
gceSTATUS glshRemoveDatabase(GLFramebuffer Framebuffer);
gceSTATUS glshSelectDatabase(GLFramebuffer Framebuffer,
                             glsDATABASE_PTR * Database);

#endif /* _gc_glsh_database_h */
