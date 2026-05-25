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

typedef struct Vert_t {
    vec3 pos;     /* Position in model/world space */
    vec3 color;   /* RGB color (0.0 – 1.0 per channel) */
} Vert;

/* Buffer constante de iluminación (estado global, no por-vértice).
   Espacio MUNDO. Por ahora: luz de punto difusa + término ambiente. */
typedef struct GsLight_t {
    vec3  pos;      /* w=1: posición (mundo); w=0: dirección HACIA la luz */
    vec3  color;    /* color/intensidad de la luz (difuso y especular, L) */
    vec3  ambient;  /* término ambiente colapsado (C_a·I_a) */
    float w;        /* 0 = direccional (sol), 1 = luz de punto */
    float atten_f;  /* exponente f de atenuación (1/dist)^f; solo si w=1 */
} GsLight;

/* Material (buffer constante). El color difuso/ambiente sale del color de
   vértice; aquí va lo que el pizarrón llama C_e y e. */
typedef struct GsMaterial_t {
    vec3  specular;   /* C_e: color del brillo especular */
    float shininess;  /* e: exponente de especularidad */
} GsMaterial;

/* Textura (buffer constante). Datos como BMP 24bpp: BGR, fila 0 = abajo. */
enum { GS_FILTER_NEAREST, GS_FILTER_LINEAR };
enum { GS_WRAP_CLAMP, GS_WRAP_REPEAT, GS_WRAP_MIRROR };

typedef struct GsTexture_t {
    u32            w, h;   /* dimensiones en texels */
    unsigned char *data;   /* w*h*3, BGR, fila 0 = abajo */
    int            filter; /* GS_FILTER_* */
    int            wrap;   /* GS_WRAP_* (igual para s y t) */
} GsTexture;

enum PrimType {
	GS_TYPE_POINT,
	GS_TYPE_LINES,
	GS_TYPE_TRIANGLES,
	GS_TYPE_MAX
};

void gs_Viewport(u32 x, u32 y, u32 w, u32 h);
void gs_Clear(void);
void gs_SetClearColor(u32 clear_color, f32 z_clear);

void gs_PokePixel(u32 x, u32 y, u32 color);
void gs_DrawArrays(u32 prim_type, Vert *v_arr, u32 v_count);
void gs_DrawElems(u32 prim_type, Vert *v_arr, u32 v_count, u32 *i_arr, u32 i_count);

/* Igual que gs_DrawElems pero con arrays paralelos de normales y UV (1 por
   vértice, mismos índices que v_arr; ambos NULL permitidos).
   Solo GS_TYPE_TRIANGLES se ilumina/texturiza. */
void gs_DrawElemsLit(u32 prim_type, Vert *v_arr, u32 v_count,
                     u32 *i_arr, u32 i_count, vec3 *n_arr, vec3 *t_arr);

/* Buffer constante de luz (espacio mundo) */
void gs_SetLight(GsLight light);
/* Buffer constante de material */
void gs_SetMaterial(GsMaterial m);
/* Posición de la cámara en mundo (necesaria para el especular) */
void gs_SetCameraPos(vec3 cam);
/* Buffer constante de textura. NULL = sin textura (default). */
void gs_SetTexture(GsTexture *tex);
/* Iluminación por fragmento: 0=off (default), 1=on */
void gs_SetLighting(int enable);

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

/* SSAA 2×: downsample del buffer interno (2×) al framebuffer de display.
   Llamar justo antes de OSW_VideoDrawBuffer cada frame. */
void gs_Resolve(void);

#endif /*__GS_H__*/
