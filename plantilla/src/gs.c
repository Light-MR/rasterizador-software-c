    
#include <gs.h>
#include <string.h>

/* ── Estado interno ─────────────────────────────────────────────── */

static u32   *gs_fb;
static u32    gs_fb_w, gs_fb_h;
static u32    gs_vp_x, gs_vp_y, gs_vp_w, gs_vp_h;
static u32    gs_clear_color;
static float  gs_zbuf[GS_DISP_W * GS_DISP_H]; /* buffer de profundidad */

static mat4   gs_matM, gs_matV, gs_matP, gs_MVP;
static int    gs_line_ztest    = 1;    /* 1=normal, 0=siempre dibuja */
static float  gs_alpha         = 1.0f; /* 1.0=opaco, 0.0=invisible   */
static int    gs_backface_cull = 1;    /* 1=cull (default), 0=ambas caras */

/* ── Helpers internos ───────────────────────────────────────────── */

static void recalc_MVP(void) {
    mat4 PV;
    mat4_mul(PV,     gs_matP, gs_matV);
    mat4_mul(gs_MVP, PV,      gs_matM);
}

/* poke: escribe un píxel con alpha blending si gs_alpha < 1 */
static void poke(s32 x, s32 y, u32 src) {
    if (x < 0 || y < 0 || (u32)x >= gs_fb_w || (u32)y >= gs_fb_h) return;
    u32 idx = (u32)y * gs_fb_w + (u32)x;
    gs_fb[idx] = (gs_alpha >= 1.0f) ? src : color_lerp(src, gs_fb[idx], gs_alpha);
}

static u32 vec3_to_argb(vec3 c) {
    u32 r = (u32)(c.x * 255.0f) & 0xFF;
    u32 g = (u32)(c.y * 255.0f) & 0xFF;
    u32 b = (u32)(c.z * 255.0f) & 0xFF;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/* Transforma un vértice por gs_MVP (precalculada) y lo lleva a
   coordenadas de pantalla. También devuelve la profundidad en [0,1]
   para el z-buffer. Retorna 0 si el vértice está detrás de la cámara. */
static int project(vec3 pos, s32 *sx, s32 *sy, float *depth) {
    vec3  clip;
    float w = vec3_mat4Mul(&clip, gs_MVP, pos);
    if (w <= 0.0f) return 0;

    vec3 ndc = vec3_homogenize(clip, w);   /* clip / w → [-1, 1] */

    *sx    = (s32)(( ndc.x * 0.5f + 0.5f)         * (float)gs_vp_w + (float)gs_vp_x);
    *sy    = (s32)((1.0f - (ndc.y * 0.5f + 0.5f)) * (float)gs_vp_h + (float)gs_vp_y);
    *depth = ndc.z * 0.5f + 0.5f;         /* [-1,1] → [0,1]  (0=cerca, 1=lejos) */
    return 1;
}

/* Función de arista para coordenadas baricéntricas */
static s32 edge_fn(s32 ax, s32 ay, s32 bx, s32 by, s32 px, s32 py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

/* ── Inicialización ─────────────────────────────────────────────── */

void gs_Init(u32 *fb, u32 fb_w, u32 fb_h) {
    gs_fb   = fb;
    gs_fb_w = fb_w;
    gs_fb_h = fb_h;
    mat4_identity(gs_matM);
    mat4_identity(gs_matV);
    mat4_identity(gs_matP);
    mat4_identity(gs_MVP);
}

/* ── Viewport y clear ───────────────────────────────────────────── */

void gs_Viewport(u32 x, u32 y, u32 w, u32 h) {
    gs_vp_x = x;  gs_vp_y = y;
    gs_vp_w = w;  gs_vp_h = h;
}

void gs_SetClearColor(u32 clear_color, f32 z_clear) {
    (void)z_clear;
    gs_clear_color = clear_color;
}

/* Limpia el framebuffer (color) y el z-buffer (profundidad = 1.0 = infinito) */
void gs_Clear(void) {
    for (u32 j = gs_vp_y; j < gs_vp_y + gs_vp_h; j++) {
        for (u32 i = gs_vp_x; i < gs_vp_x + gs_vp_w; i++) {
            gs_fb  [j * gs_fb_w + i] = gs_clear_color;
            gs_zbuf[j * gs_fb_w + i] = 1.0f;
        }
    }
}

/* ── Matrices del pipeline ──────────────────────────────────────── */

void gs_SetModelMatrix(mat4 m) { memcpy(gs_matM, m, sizeof(mat4)); recalc_MVP(); }
void gs_SetViewMatrix (mat4 m) { memcpy(gs_matV, m, sizeof(mat4)); recalc_MVP(); }
void gs_SetProjMatrix (mat4 m) { memcpy(gs_matP, m, sizeof(mat4)); recalc_MVP(); }

/* ── Acceso al framebuffer ──────────────────────────────────────── */

void gs_PokePixel(u32 x, u32 y, u32 color) { poke((s32)x, (s32)y, color); }
u32* gs_GetFramebuffer(void)               { return gs_fb; }
void gs_DrawBuffer(void)                   { /* main.c llama a OSW directamente */ }
void sg_UseProgram(VertShader *vsh, FragShader *fsh) { (void)vsh; (void)fsh; }
void gs_SetLineDepthTest(int e)            { gs_line_ztest    = e; }
void gs_SetAlpha(float a)                  { gs_alpha         = a; }
void gs_SetBackfaceCull(int e)             { gs_backface_cull = e; }

/* ── Primitivas ─────────────────────────────────────────────────── */

static void __gs_DrawPoint(Vert *v0) {
    s32 sx, sy;  float depth;
    if (!project(v0->pos, &sx, &sy, &depth)) return;

    u32 idx = (u32)sy * gs_fb_w + (u32)sx;
    if (depth >= gs_zbuf[idx]) return;
    gs_zbuf[idx] = depth;
    poke(sx, sy, vec3_to_argb(v0->color));
}

static void __gs_DrawLine(Vert *v0, Vert *v1) {
    s32 x0, y0, x1, y1;  float d0, d1;
    if (!project(v0->pos, &x0, &y0, &d0)) return;
    if (!project(v1->pos, &x1, &y1, &d1)) return;

    s32 dx = x1-x0; if (dx<0) dx=-dx;
    s32 dy = y1-y0; if (dy<0) dy=-dy;
    s32 sx = (x0<x1)?1:-1;
    s32 sy = (y0<y1)?1:-1;
    s32 err = dx-dy;

    s32   steps = dx>dy ? dx : dy;
    float inv   = steps>0 ? 1.0f/(float)steps : 0.0f;
    int   step  = 0;

    while (1) {
        float t = (float)step * inv;
        float d = d0 + t*(d1-d0);
        u32 bidx = (u32)y0 * gs_fb_w + (u32)x0;
        if (x0>=0 && y0>=0 && (u32)x0<gs_fb_w && (u32)y0<gs_fb_h) {
            if (!gs_line_ztest || d <= gs_zbuf[bidx]) {
                if (gs_line_ztest) gs_zbuf[bidx] = d;
                vec3 c = {
                    v0->color.x + t*(v1->color.x - v0->color.x),
                    v0->color.y + t*(v1->color.y - v0->color.y),
                    v0->color.z + t*(v1->color.z - v0->color.z)
                };
                poke(x0, y0, vec3_to_argb(c));
            }
        }
        if (x0==x1 && y0==y1) break;
        s32 e2 = err*2;
        if (e2>-dy){err-=dy; x0+=sx;}
        if (e2< dx){err+=dx; y0+=sy;}
        step++;
    }
}

static void __gs_DrawTriangle(Vert *v0, Vert *v1, Vert *v2) {

    /* 1. Proyectar los 3 vértices */
    s32 sx0,sy0, sx1,sy1, sx2,sy2;
    float z0, z1, z2;
    if (!project(v0->pos, &sx0, &sy0, &z0)) return;
    if (!project(v1->pos, &sx1, &sy1, &z1)) return;
    if (!project(v2->pos, &sx2, &sy2, &z2)) return;

    /* 2. Back-face culling (área con signo del triángulo proyectado)
     *    area > 0 → vértices CCW → cara frontal
     *    area < 0 → vértices CW  → cara trasera
     *    area = 0 → triángulo degenerado → descartar siempre            */
    s32 area = edge_fn(sx0,sy0, sx1,sy1, sx2,sy2);
    if (area == 0) return;
    /* CONVENCIÓN: project() invierte Y (NDC→pantalla), por lo que los triángulos
     * CCW en 3D (caras frontales) resultan con area < 0 en pantalla.
     * area < 0 → CCW en 3D → cara FRONTAL   (is_back=1 para el test de aristas)
     * area > 0 → CW  en 3D → cara TRASERA   (is_back=0)                         */
    int is_back = 0;
    if (area < 0) {
        /* cara frontal */
        if (gs_backface_cull == 2) return;   /* modo 2: cull frontales */
        is_back = 1;
        area = -area;
    } else {
        /* cara trasera */
        if (gs_backface_cull == 1) return;   /* modo 1: cull traseras (default) */
    }

    /* 3. Bounding box recortado al viewport */
    s32 minX = sx0<sx1?sx0:sx1; if(sx2<minX) minX=sx2;
    s32 minY = sy0<sy1?sy0:sy1; if(sy2<minY) minY=sy2;
    s32 maxX = sx0>sx1?sx0:sx1; if(sx2>maxX) maxX=sx2;
    s32 maxY = sy0>sy1?sy0:sy1; if(sy2>maxY) maxY=sy2;

    s32 vpx1 = (s32)(gs_vp_x+gs_vp_w-1);
    s32 vpy1 = (s32)(gs_vp_y+gs_vp_h-1);
    if(minX<(s32)gs_vp_x) minX=(s32)gs_vp_x;
    if(minY<(s32)gs_vp_y) minY=(s32)gs_vp_y;
    if(maxX>vpx1)         maxX=vpx1;
    if(maxY>vpy1)         maxY=vpy1;

    float inv_area = 1.0f/(float)area;

    /* 4. Rasterizar con baricéntricas + test de profundidad */
    for (s32 py=minY; py<=maxY; py++) {
        for (s32 px=minX; px<=maxX; px++) {
            s32 e0 = edge_fn(sx1,sy1, sx2,sy2, px,py);
            s32 e1 = edge_fn(sx2,sy2, sx0,sy0, px,py);
            s32 e2 = edge_fn(sx0,sy0, sx1,sy1, px,py);
            /* Para caras traseras las funciones de arista están invertidas */
            if (is_back) { e0=-e0; e1=-e1; e2=-e2; }
            if (e0<0 || e1<0 || e2<0) continue;

            float b0 = (float)e0 * inv_area;
            float b1 = (float)e1 * inv_area;
            float b2 = (float)e2 * inv_area;

            /* Profundidad interpolada */
            float depth = b0*z0 + b1*z1 + b2*z2;
            u32   bidx  = (u32)py * gs_fb_w + (u32)px;
            if (depth >= gs_zbuf[bidx]) continue;
            /* Sólo actualizar zbuf en objetos opacos; los transparentes no
               bloquean lo que hay detrás de ellos                        */
            if (gs_alpha >= 1.0f) gs_zbuf[bidx] = depth;

            vec3 c;
            c.x = b0*v0->color.x + b1*v1->color.x + b2*v2->color.x;
            c.y = b0*v0->color.y + b1*v1->color.y + b2*v2->color.y;
            c.z = b0*v0->color.z + b1*v1->color.z + b2*v2->color.z;
            poke(px, py, vec3_to_argb(c)); 
        }
    }
}

/* ── DrawArrays / DrawElems ─────────────────────────────────────── */

void gs_DrawArrays(u32 prim_type, Vert *v_arr, u32 v_count) {
    u32 i;
    switch (prim_type) {
        case GS_TYPE_POINT:
            for (i=0; i<v_count; i++)
                __gs_DrawPoint(&v_arr[i]);
            break;
        case GS_TYPE_LINES:
            for (i=0; i+1<v_count; i+=2)
                __gs_DrawLine(&v_arr[i], &v_arr[i+1]);
            break;
        case GS_TYPE_TRIANGLES:
            for (i=0; i+2<v_count; i+=3)
                __gs_DrawTriangle(&v_arr[i], &v_arr[i+1], &v_arr[i+2]);
            break;
        default: break;
    }
}

void gs_DrawElems(u32 prim_type, Vert *v_arr, u32 v_count,
                  u32 *i_arr, u32 i_count) {
    (void)v_count;
    u32 i;
    switch (prim_type) {
        case GS_TYPE_POINT:
            for (i=0; i<i_count; i++)
                __gs_DrawPoint(&v_arr[i_arr[i]]);
            break;
        case GS_TYPE_LINES:
            for (i=0; i+1<i_count; i+=2)
                __gs_DrawLine(&v_arr[i_arr[i]], &v_arr[i_arr[i+1]]);
            break;
        case GS_TYPE_TRIANGLES:
            for (i=0; i+2<i_count; i+=3)
                __gs_DrawTriangle(&v_arr[i_arr[i]],
                                  &v_arr[i_arr[i+1]],
                                  &v_arr[i_arr[i+2]]);
            break;
        default: break;
    }
}
