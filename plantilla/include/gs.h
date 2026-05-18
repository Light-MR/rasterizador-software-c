/*
 * gs.h: Graphics system functions
 * v1.0 - public domain
 */

#ifndef __GS_H__
#define __GS_H__

#include <trx.h>
#include <libosw/types.h>
#include <libosw/video.h>

#define GS_DISP_W	640
#define GS_DISP_H	480

/* ==========================
   Vertex definition: position + color
   ========================== */
typedef struct Vert_t {
    vec3 pos;     /* Position in model/world space */
    vec3 color;   /* RGB color (0.0 – 1.0 per channel) */
} Vert;

/* Temporal typedefs for shaders (unused for now) */
typedef void VertShader;
typedef void FragShader;

/* ========================== */

enum PrimType {
	GS_TYPE_POINT,
	GS_TYPE_LINES,
	GS_TYPE_TRIANGLES,
	GS_TYPE_MAX
};

/* Graphics related functions */
void gs_Viewport(u32 x, u32 y, u32 w, u32 h);
void gs_DrawBuffer(void);
void gs_Clear(void);
void gs_SetClearColor(u32 clear_color, f32 z_clear);
void sg_UseProgram(VertShader *vsh, FragShader *fsh);

void gs_PokePixel(u32 x, u32 y, u32 color);
void gs_DrawArrays(u32 prim_type, Vert *v_arr, u32 v_count);
void gs_DrawElems(u32 prim_type, Vert *v_arr, u32 v_count, u32 *i_arr, u32 i_count);

/* Access to internal framebuffer for OSW presentation */
u32* gs_GetFramebuffer(void);

/* Inicialización: conecta gs con el framebuffer de main.c */
void gs_Init(u32 *fb, u32 fb_w, u32 fb_h);

/* Matrices del pipeline — cada Set* recalcula MVP = P*V*M internamente */
void gs_SetModelMatrix(mat4 m);
void gs_SetViewMatrix(mat4 m);
void gs_SetProjMatrix(mat4 m);

/* Depth test para líneas: 1=normal (default), 0=siempre dibuja (ver aristas traseras) */
void gs_SetLineDepthTest(int enable);

/* Alpha global: 1.0=opaco (default), 0.4=40% transparencia, etc.
   Los objetos transparentes no escriben al z-buffer.             */
void gs_SetAlpha(float a);

/* Back-face culling: 1=activo (default), 0=dibuja ambas caras
   Necesario desactivar para ver caras traseras con transparencia */
void gs_SetBackfaceCull(int enable);

#endif /*__GS_H__*/
