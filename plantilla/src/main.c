/*
 * main.c — Cubo 3D con pipeline por software (gs.c)
 *
 * Controles:
 *   Flecha izq/der : rotar en Y
 *   Flecha arr/aba : rotar en X
 *   Q / E          : rotar en Z
 *   W / S          : acercar / alejar la cámara
 *
 * El código de MiniPaint original está preservado en el bloque #if 0
 * al final de este archivo.
 */

#include <gs.h>
#include <libosw/osw.h>
#include <stdio.h>
#include <trx.h>

#define FB_W 640
#define FB_H 480

static u32 framebuffer[FB_W * FB_H];

/* ── Geometría del cubo ─────────────────────────────────────────── */
/*
 * 8 vértices de un cubo unitario centrado en el origen.
 * Colores:
 *   Cara frontal  (z = +0.5) → rojo       {1, 0.1, 0.1}
 *   Resto de caras            → azul gris  {0.2, 0.4, 0.9}
 *
 *      6 ──── 7
 *     /|     /|
 *    2 ──── 3 |
 *    | 4 ───|─5
 *    |/     |/
 *    0 ──── 1
 *
 *  v0..v3 = cara frontal  (z=+0.5)  → rojo
 *  v4..v7 = cara trasera  (z=-0.5)  → azul
 */

/* Un color distinto por cada cara */
#define C_FRONT {1.0f, 0.2f, 0.2f}   /* frontal    — rojo    */
#define C_BACK {0.1f, 0.8f, 0.2f}    /* trasera    — verde   */
#define C_RIGHT {0.2f, 0.3f, 1.0f}   /* derecha    — azul    */
#define C_LEFT {1.0f, 0.85f, 0.0f}   /* izquierda  — amarillo*/
#define C_TOP {0.0f, 0.9f, 0.9f}     /* arriba     — cian    */
#define C_BOT {0.9f, 0.1f, 0.9f}     /* abajo      — magenta */
#define C_WIRE {0.05f, 0.05f, 0.05f} /* bordes     — negro   */

/*
 * 24 vértices (4 por cara, sin compartir entre caras).
 * Orden CCW visto desde afuera para que back-face culling funcione.
 *  Índices por cara: 0-3 frontal, 4-7 trasera, 8-11 derecha,
 *                    12-15 izquierda, 16-19 arriba, 20-23 abajo.
 */
static Vert cube_verts[24] = {
    /* frontal  (z=+0.5) */
    {{-0.5f, -0.5f, 0.5f}, C_FRONT}, /*  0 */
    {{0.5f, -0.5f, 0.5f}, C_FRONT},  /*  1 */
    {{0.5f, 0.5f, 0.5f}, C_FRONT},   /*  2 */
    {{-0.5f, 0.5f, 0.5f}, C_FRONT},  /*  3 */
    /* trasera  (z=-0.5) */
    {{0.5f, -0.5f, -0.5f}, C_BACK},  /*  4 */
    {{-0.5f, -0.5f, -0.5f}, C_BACK}, /*  5 */
    {{-0.5f, 0.5f, -0.5f}, C_BACK},  /*  6 */
    {{0.5f, 0.5f, -0.5f}, C_BACK},   /*  7 */
    /* derecha  (x=+0.5) */
    {{0.5f, -0.5f, 0.5f}, C_RIGHT},  /*  8 */
    {{0.5f, -0.5f, -0.5f}, C_RIGHT}, /*  9 */
    {{0.5f, 0.5f, -0.5f}, C_RIGHT},  /* 10 */
    {{0.5f, 0.5f, 0.5f}, C_RIGHT},   /* 11 */
    /* izquierda(x=-0.5) */
    {{-0.5f, -0.5f, -0.5f}, C_LEFT}, /* 12 */
    {{-0.5f, -0.5f, 0.5f}, C_LEFT},  /* 13 */
    {{-0.5f, 0.5f, 0.5f}, C_LEFT},   /* 14 */
    {{-0.5f, 0.5f, -0.5f}, C_LEFT},  /* 15 */
    /* arriba   (y=+0.5) */
    {{-0.5f, 0.5f, 0.5f}, C_TOP},  /* 16 */
    {{0.5f, 0.5f, 0.5f}, C_TOP},   /* 17 */
    {{0.5f, 0.5f, -0.5f}, C_TOP},  /* 18 */
    {{-0.5f, 0.5f, -0.5f}, C_TOP}, /* 19 */
    /* abajo    (y=-0.5) */
    {{-0.5f, -0.5f, -0.5f}, C_BOT}, /* 20 */
    {{0.5f, -0.5f, -0.5f}, C_BOT},  /* 21 */
    {{0.5f, -0.5f, 0.5f}, C_BOT},   /* 22 */
    {{-0.5f, -0.5f, 0.5f}, C_BOT},  /* 23 */
};

/* 36 índices: 2 triángulos por cara × 6 caras */
static u32 cube_idx[36] = {
    /* frontal   */ 0,  1,  2,  0,  2,  3,
    /* trasera   */ 4,  5,  6,  4,  6,  7,
    /* derecha   */ 8,  9,  10, 8,  10, 11,
    /* izquierda */ 12, 13, 14, 12, 14, 15,
    /* arriba    */ 16, 17, 18, 16, 18, 19,
    /* abajo     */ 20, 21, 22, 20, 22, 23,
};

/* ── Wireframe del cubo (8 vértices compartidos, 12 aristas) ──── */
static Vert wire_verts[8] = {
    {{-0.5f, -0.5f, 0.5f}, C_WIRE},  /* 0 front-BL */
    {{0.5f, -0.5f, 0.5f}, C_WIRE},   /* 1 front-BR */
    {{0.5f, 0.5f, 0.5f}, C_WIRE},    /* 2 front-TR */
    {{-0.5f, 0.5f, 0.5f}, C_WIRE},   /* 3 front-TL */
    {{0.5f, -0.5f, -0.5f}, C_WIRE},  /* 4 back-BR  */
    {{-0.5f, -0.5f, -0.5f}, C_WIRE}, /* 5 back-BL  */
    {{-0.5f, 0.5f, -0.5f}, C_WIRE},  /* 6 back-TL  */
    {{0.5f, 0.5f, -0.5f}, C_WIRE},   /* 7 back-TR  */
};

/* 24 índices: 12 aristas × 2 vértices cada una */
static u32 wire_idx[24] = {
    0, 1, 1, 2, 2, 3, 3, 0, /* cara frontal  */
    4, 5, 5, 6, 6, 7, 7, 4, /* cara trasera  */
    0, 5, 1, 4, 2, 7, 3, 6, /* aristas laterales */
};

/*
 * Aristas de cada cara (4 líneas por cara) usando los mismos cube_verts.
 * Cada arista hereda el color de la cara — sin relleno, sólo contorno.
 * 6 caras × 4 aristas × 2 índices = 48 índices.
 */
static u32 face_edge_idx[48] = {
    /* frontal   */ 0,  1,  1,  2,  2,  3,  3,  0,
    /* trasera   */ 4,  5,  5,  6,  6,  7,  7,  4,
    /* derecha   */ 8,  9,  9,  10, 10, 11, 11, 8,
    /* izquierda */ 12, 13, 13, 14, 14, 15, 15, 12,
    /* arriba    */ 16, 17, 17, 18, 18, 19, 19, 16,
    /* abajo     */ 20, 21, 21, 22, 22, 23, 23, 20,
};

/* ── Keycodes (Win32 VK) ────────────────────────────────────────── */
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_UP 0x48
#define KEY_DOWN 0x50
#define KEY_W 0x11
#define KEY_S 0x1F
#define KEY_Q 0x10
#define KEY_E 0x12
#define KEY_O 0x18 /* toggle perspectiva / ortogonal */

/* ── Main ───────────────────────────────────────────────────────── */

int main(void) {
  u32 err = OSW_Init("Cubo 3D", FB_W, FB_H, 0);
  if (err != OSW_OK)
    return (int)err;

  /* Inicializar el sistema gráfico */
  gs_Init(framebuffer, FB_W, FB_H);
  gs_Viewport(0, 0, FB_W, FB_H);
  gs_SetClearColor(0xFF1C1C1C, 1.0f);

  /* Ángulos de rotación (reconstruyen M limpia cada frame) y distancia de
   * cámara */
  float angle_x = 0.0f;
  float angle_y = 0.0f;
  float angle_z = 0.0f;
  float cam_dist = 6.0f;
  int use_ortho = 0; /* 0=perspectiva, 1=ortogonal */
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up = {0.0f, 1.0f, 0.0f};

  OSW_KeyboardSetPolling(1);

  while (1) {
    OSW_Poll();

    /* Input: modificar ángulos directamente */
    OSWKeyEvent kev;
    while (OSW_KeyboardGetEvent(&kev)) {
      if (kev.type != OSW_KEYEV_TYPE_PRESSED)
        continue;
      switch (kev.keycode) {
      case KEY_LEFT:
        angle_y -= 10.0f;
        break;
      case KEY_RIGHT:
        angle_y += 10.0f;
        break;
      case KEY_UP:
        angle_x -= 10.0f;
        break;
      case KEY_DOWN:
        angle_x += 10.0f;
        break;
      case KEY_Q:
        angle_z -= 10.0f;
        break;
      case KEY_E:
        angle_z += 10.0f;
        break;
      case KEY_W:
        cam_dist -= 0.2f;
        if (cam_dist < 0.3f)
          cam_dist = 0.3f;
        break;
      case KEY_S:
        cam_dist += 0.2f;
        break;
      case KEY_O:
        use_ortho = !use_ortho;
        break;
      default:
        break;
      }
    }

    /* Auto-spin: 0.5° por frame (≈30°/seg a 60fps, vuelta en ~12 seg) */
    angle_y += 0.1f;

    /* Reconstruir Model matrix desde identidad — sin acumulación, sin deriva */
    mat4 M;
    vec3 axX = {1, 0, 0}, axY = {0, 1, 0}, axZ = {0, 0, 1};
    mat4_identity(M);
    mat4_rotate(M, axX, angle_x);
    mat4_rotate(M, axY, angle_y);
    mat4_rotate(M, axZ, angle_z);
    gs_SetModelMatrix(M);

    /* View matrix — recalcular cada frame para que W/S funcionen */
    vec3 eye = {0.0f, 0.5f, cam_dist};
    mat4 V;
    mat4_lookAt(V, eye, center, up);
    gs_SetViewMatrix(V);

    /* Proyección — recalcular cada frame para que el toggle O y el zoom
     * funcionen */
    {
      mat4 P;
      float aspect = (float)FB_W / (float)FB_H;
      if (use_ortho) {
        /* Tamaño ortogonal equivalente a la perspectiva de 60° a cam_dist */
        float h = cam_dist * 0.268f; /* tan(15°) ≈ 0.268 */
        mat4_ortho(P, -h * aspect, h * aspect, -h, h, 0.1f, 100.0f);
      } else {
      
        mat4_perspective(P, 30.0f, aspect, 0.1f, 100.0f); /* FOV 30° para que el cubo quepa bien a cam_dist=6.0f */
      }
      gs_SetProjMatrix(P);
    }

    gs_Clear();

    if (use_ortho) {
      /* MODO ORTOGONAL: Caras OPACAS con backface culling: cada píxel lo dibuja un triángulo → sin diagonal */
      gs_SetAlpha(1.0f);
      gs_SetBackfaceCull(1);
      gs_DrawElems(GS_TYPE_TRIANGLES, cube_verts, 24, cube_idx, 36);
      /* Aristas con depth test: solo aristas frontales → sin ruido */
      gs_SetLineDepthTest(1);
      gs_DrawElems(GS_TYPE_LINES, cube_verts, 24, face_edge_idx, 48);
    } else {
      /* MODO PERSPECTIVA: caras SEMITRANSPARENTES ordenadas de atrás hacia
       * adelante (painter's algorithm) para una mezcla estable al girar. */

      /* MV = V * M : lleva un punto de espacio modelo a espacio cámara */
      mat4 MV;
      mat4_mul(MV, V, M);

      /* Profundidad (z cámara) del centroide de cada una de las 6 caras */
      int   face_order[6];
      float face_z[6];
      for (int f = 0; f < 6; f++) {
        vec3 ctr = {0.0f, 0.0f, 0.0f};
        for (int k = 0; k < 4; k++) {
          ctr.x += cube_verts[f * 4 + k].pos.x;
          ctr.y += cube_verts[f * 4 + k].pos.y;
          ctr.z += cube_verts[f * 4 + k].pos.z;
        }
        ctr.x *= 0.25f;
        ctr.y *= 0.25f;
        ctr.z *= 0.25f;
        vec3 cam;
        vec3_mat4Mul(&cam, MV, ctr);
        face_z[f]     = cam.z; /* cámara mira a -Z: más negativo = más lejos */
        face_order[f] = f;
      }

      /* Insertion sort: z ascendente → caras lejanas se dibujan primero */
      for (int i = 1; i < 6; i++) {
        int   fi = face_order[i];
        float zi = face_z[fi];
        int   j  = i - 1;
        while (j >= 0 && face_z[face_order[j]] > zi) {
          face_order[j + 1] = face_order[j];
          j--;
        }
        face_order[j + 1] = fi;
      }

      /* Reordenar los 36 índices según el orden de caras resultante */
      u32 sorted_idx[36];
      for (int i = 0; i < 6; i++) {
        int f = face_order[i];
        for (int k = 0; k < 6; k++)
          sorted_idx[i * 6 + k] = cube_idx[f * 6 + k];
      }

      gs_SetAlpha(0.4f);
      gs_SetBackfaceCull(0); /* ambas caras: el orden lo da el sort */
      gs_DrawElems(GS_TYPE_TRIANGLES, cube_verts, 24, sorted_idx, 36);

      /* Aristas sin depth test: las traseras se ven */
      gs_SetAlpha(1.0f);
      gs_SetLineDepthTest(0);
      gs_DrawElems(GS_TYPE_LINES, cube_verts, 24, face_edge_idx, 48);
    }

    OSW_VideoDrawBuffer(framebuffer, FB_W, FB_H);
    OSW_VideoSwapBuffers();
  }

  return 0;
}

/* ================================================================
   MINIPAINT — código original preservado
   Para reactivar: cambiar #if 0 por #if 1
   ================================================================ */
#if 0

#include <libosw/osw.h>
#include <stdio.h>

#define FB_H 240
#define FB_W 320
#define PALETA_W 80

u32 framebuffer[FB_W * FB_H];

u32 current_color = 0xFFEED6DC;
u32 prev_Color    = 0xFFC9B7E8;

#define BOX_H 24
#define BOX_W (PALETA_W / 2)
#define PAL_COLS 8
#define PAL_ROWS 8
#define PAL_START_Y BOX_H
#define CELL_W (PALETA_W / PAL_COLS)
#define CELL_H 10
#define PAL_GRID_H (PAL_ROWS * CELL_H)

static const u32 palette[PAL_ROWS * PAL_COLS] = {
    0xFFFDEBD0, 0xFFF5CBA7, 0xFFE8B88A, 0xFFD4956B,
    0xFFC07850, 0xFFA0522D, 0xFF6B3A2A, 0xFF3B1F12,
    0xFFFF0000, 0xFFCC0000, 0xFF880000, 0xFFFF4444,
    0xFFFF8800, 0xFFFF6600, 0xFFCC4400, 0xFFFF2266,
    0xFFFFFF00, 0xFFFFDD00, 0xFFFFAA00, 0xFFFFCC44,
    0xFFF0E68C, 0xFFDAA520, 0xFFB8860B, 0xFFFFF8DC,
    0xFF00FF00, 0xFF00CC00, 0xFF008800, 0xFF004400,
    0xFF44FF44, 0xFF88CC44, 0xFF228B22, 0xFF2E8B57,
    0xFF0088FF, 0xFF0044CC, 0xFF002288, 0xFF87CEEB,
    0xFF4488FF, 0xFF1E90FF, 0xFF000066, 0xFF00CCFF,
    0xFFFF00FF, 0xFFAA00AA, 0xFF660088, 0xFFCC44FF,
    0xFFFF88CC, 0xFFFF4488, 0xFFEE82EE, 0xFF9966CC,
    0xFF8B4513, 0xFFA0522D, 0xFFD2691E, 0xFFDEB887,
    0xFF3E1C00, 0xFF5C3317, 0xFF8B7355, 0xFFD2B48C,
    0xFF000000, 0xFF1C1C1C, 0xFF444444, 0xFF888888,
    0xFFBBBBBB, 0xFFDDDDDD, 0xFFFFFFFF, 0xFFFF0088,
};

#define KEYCODE_S 0x1F

void fillArea(u32 color, u32 x, u32 y, u32 w, u32 h){
    for(u32 j = y; j < y + h; j++)
        for(u32 i = x; i < x + w; i++)
            framebuffer[j*FB_W+i] = color;
}

void set_pixel(s32 x, s32 y, u32 color){
    if(x < 0 || y < 0 || x>=FB_W || y >= FB_H) return;
    framebuffer[y*FB_W+x] = color;
}

u32 esLienzo(s32 x, s32 y){
    if (x >(s32)PALETA_W) return 1;
    return 0;
}

void dibujaLinea(s32 x0, s32 y0, s32 x1, s32 y1){
    s32 dx = x1-x0; if(dx<0) dx=-dx;
    s32 dy = y1-y0; if(dy<0) dy=-dy;
    s32 sx = (x0<x1)?1:-1;
    s32 sy = (y0<y1)?1:-1;
    s32 err = dx-dy;
    while(1){
        if(esLienzo(x0,y0)) set_pixel(x0,y0,current_color);
        if(x0==x1 && y0==y1) break;
        s32 e2 = err*2;
        if(e2>-dy){err-=dy; x0+=sx;}
        if(e2< dx){err+=dx; y0+=sy;}
    }
}

void dibujaPaleta(void){
    for(u32 row=0;row<PAL_ROWS;row++)
        for(u32 col=0;col<PAL_COLS;col++)
            fillArea(palette[row*PAL_COLS+col],col*CELL_W+1,PAL_START_Y+row*CELL_H+1,CELL_W-1,CELL_H-1);
}

void dibujaCajas(void){
    fillArea(current_color,1,1,BOX_W-2,BOX_H-2);
    fillArea(prev_Color,BOX_W+1,1,BOX_W-2,BOX_H-2);
}

void dibujaSeparador(void){
    fillArea(0xFF1C1C1C,PALETA_W,0,1,FB_H);
}

void GuardarDibujo(const char* nombre){
    FILE* f=fopen(nombre,"wb");
    if(!f){printf("Error al guardar\n");return;}
    fprintf(f,"P6\n%d %d\n255\n",FB_W,FB_H);
    for(u32 i=0;i<FB_W*FB_H;i++){
        u32 px=framebuffer[i];
        u8 r=(px>>16)&0xFF, g=(px>>8)&0xFF, b=px&0xFF;
        fwrite(&r,1,1,f); fwrite(&g,1,1,f); fwrite(&b,1,1,f);
    }
    fclose(f);
    printf("Guardado: %s\n",nombre);
}

int main(){
    u32 err=OSW_Init("MiniPaint",FB_W,FB_H,0);
    if(err!=OSW_OK) return err;
    fillArea(0xFF1C1C1C,PALETA_W+1,0,FB_W-PALETA_W-1,FB_H);
    fillArea(0xFF1C1317,0,BOX_H+PAL_GRID_H,PALETA_W,FB_H-BOX_H-PAL_GRID_H);
    dibujaPaleta(); dibujaCajas(); dibujaSeparador();
    OSW_MouseSetPolling(1);
    OSW_KeyboardSetPolling(1);
    OSWMouse mouse;
    u32 btn_prev=0;
    s32 prev_x=-1,prev_y=-1;
    while(1){
        OSW_Poll();
        OSWKeyEvent kev;
        while(OSW_KeyboardGetEvent(&kev))
            if(kev.type==OSW_KEYEV_TYPE_PRESSED && kev.keycode==KEYCODE_S)
                GuardarDibujo("dibujo.ppm");
        OSW_MouseGetState(&mouse);
        u32 btn=mouse.btn&OSW_MOUSE_BTN0;
        if(btn && !btn_prev){
            u32 mx=(u32)mouse.x, my=(u32)mouse.y;
            if(mx<PALETA_W){
                if(my<BOX_H){ u32 t=current_color; current_color=prev_Color; prev_Color=t; dibujaCajas(); }
                else if(my>=PAL_START_Y && my<PAL_START_Y+PAL_GRID_H){
                    u32 col=mx/CELL_W, row=(my-PAL_START_Y)/CELL_H;
                    prev_Color=current_color;
                    current_color=palette[row*PAL_COLS+col];
                    dibujaCajas();
                }
            }
        }
        btn_prev=btn;
        if(btn){
            s32 mx=(s32)mouse.x, my=(s32)mouse.y;
            if(esLienzo(mx,my)){
                if(prev_x>=0) dibujaLinea(prev_x,prev_y,mx,my);
                else set_pixel(mx,my,current_color);
                prev_x=mx; prev_y=my;
            } else { prev_x=-1; prev_y=-1; }
        } else { prev_x=-1; prev_y=-1; }
        OSW_VideoDrawBuffer(framebuffer,FB_W,FB_H);
        OSW_VideoSwapBuffers();
    }
    return 0;
}

#endif /* MINIPAINT */
