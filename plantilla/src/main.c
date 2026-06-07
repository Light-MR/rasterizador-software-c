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

#include <bmp.h>
#include <gs.h>
#include <libosw/osw.h>
#include <stdio.h>
#include <string.h>
#include <tdm.h>
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

/* ── Keycodes (PS/2 scan codes) ─────────────────────────────────── */
#define KEY_LEFT  0x4B
#define KEY_RIGHT 0x4D
#define KEY_UP    0x48
#define KEY_DOWN  0x50
#define KEY_W     0x11
#define KEY_S     0x1F
#define KEY_A     0x1E
#define KEY_D     0x20
#define KEY_Q     0x10
#define KEY_E     0x12
#define KEY_O     0x18 /* toggle perspectiva / ortogonal */
#define KEY_SPACE 0x39 /* toggle auto-spin               */

/* ── Main ───────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
  u32 err = OSW_Init("Cubo 3D", FB_W, FB_H, 0);
  if (err != OSW_OK)
    return (int)err;

  /* Inicializar el sistema gráfico */
  gs_Init(framebuffer, FB_W, FB_H);
  gs_Viewport(0, 0, FB_W * 2, FB_H * 2);
  gs_SetClearColor(0xFF1C1C1C, 1.0f);

  /* Modelo opcional: `prog.exe <ruta.tdm>` carga y renderiza esa malla.
   * Sin argumentos → cubo de siempre. tdm_Load la normaliza al tamaño del
   * cubo unitario, así se reusan los mismos controles de rotación/zoom. */
  TdmModel model;
  int show_model = 0;
  if (argc > 1) {
    if (tdm_Load(argv[1], &model) == 0)
      show_model = 1;
    else
      printf("[main] no se pudo cargar '%s'; mostrando el cubo.\n", argv[1]);
  }

  /* Textura del modelo: busca <mismo_nombre>.bmp junto al .tdm */
  GsTexture tex;
  int has_tex = 0;
  if (show_model) {
    char bmp_path[512];
    strncpy(bmp_path, argv[1], sizeof(bmp_path) - 1);
    bmp_path[sizeof(bmp_path) - 1] = '\0';
    char *dot = strrchr(bmp_path, '.');
    if (dot) strcpy(dot, ".bmp");
    if (bmp_Load(bmp_path, &tex) == 0) {
      tex.filter = GS_FILTER_LINEAR;
      tex.wrap   = GS_WRAP_REPEAT;
      has_tex    = 1;
      printf("[main] textura: %s\n", bmp_path);
    }
  }

  /* Ángulos de rotación (reconstruyen M limpia cada frame) y distancia de
   * cámara */
  float angle_x = 0.0f;
  float angle_y = 0.0f;
  float angle_z = 0.0f;
  float cam_dist = 6.0f;
  int use_ortho = 0; /* 0=perspectiva, 1=ortogonal */
  int auto_spin = 1; /* SPACE para pausar/reanudar  */
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up = {0.0f, 1.0f, 0.0f};

  /* Estado de teclas sostenidas (scan-code → 0/1) */
  static int keys[256];

  OSW_KeyboardSetPolling(1);
  OSW_MouseSetPolling(1);

  while (1) {
    OSW_Poll();

    /* ── Input de teclado ────────────────────────────────────────── */
    /* Actualiza el array de teclas sostenidas y maneja eventos de un solo disparo */
    OSWKeyEvent kev;
    while (OSW_KeyboardGetEvent(&kev)) {
      if (kev.keycode < 256) {
        if (kev.type == OSW_KEYEV_TYPE_PRESSED)  keys[kev.keycode] = 1;
        if (kev.type == OSW_KEYEV_TYPE_RELEASED) keys[kev.keycode] = 0;
      }
      if (kev.type == OSW_KEYEV_TYPE_PRESSED) {
        if (kev.keycode == KEY_O)     use_ortho = !use_ortho;
        if (kev.keycode == KEY_SPACE) auto_spin = !auto_spin;
      }
    }

    /* Rotación y zoom continuos mientras la tecla esté presionada */
    if (keys[KEY_LEFT]  || keys[KEY_A]) angle_y -= 2.0f;
    if (keys[KEY_RIGHT] || keys[KEY_D]) angle_y += 2.0f;
    if (keys[KEY_UP])                   angle_x -= 2.0f;
    if (keys[KEY_DOWN])                 angle_x += 2.0f;
    if (keys[KEY_Q])                    angle_z -= 2.0f;
    if (keys[KEY_E])                    angle_z += 2.0f;
    if (keys[KEY_W]) { cam_dist -= 0.15f; if (cam_dist < 0.3f) cam_dist = 0.3f; }
    if (keys[KEY_S])   cam_dist += 0.15f;

    /* ── Input de mouse ──────────────────────────────────────────── */
    /* Botón izquierdo + arrastrar → rotar; scroll → zoom */
    OSWMouse mouse;
    OSW_MouseGetState(&mouse);
    if (mouse.btn & OSW_MOUSE_BTN0) {
      angle_y += mouse.dx * 0.4f;
      angle_x += mouse.dy * 0.4f;
    }
    if (mouse.scroll) {
      cam_dist -= mouse.scroll * 0.4f;
      if (cam_dist < 0.3f) cam_dist = 0.3f;
    }

    /* Auto-spin (SPACE para pausar) */
    if (auto_spin) angle_y += 0.2f;

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

    if (show_model) {
      /* Modelo .tdm: opaco, z-buffer activo. Sin back-face culling
       * (cull=0) porque el winding de una malla externa es desconocido;
       * así se ve siempre. Iluminación Phong difusa+ambiente (espacio
       * mundo, por fragmento); luz de punto fija en mundo. */
      GsLight luz = {
          {1.5f, 1.8f, 2.5f},
          {0.82f, 0.80f, 0.74f},
          {0.15f, 0.15f, 0.15f},
          1.0f, 1.0f};
      GsMaterial mat = {{0.10f, 0.10f, 0.10f}, 16.0f};
      gs_SetAlpha(1.0f);
      gs_SetLineDepthTest(1);
      gs_SetBackfaceCull(0);
      gs_SetCameraPos(eye);
      gs_SetLight(luz);
      /* Luz de relleno muy suave: solo toca los perfiles laterales */
      GsLight fill = {{-0.6f, -1.0f, -1.2f},
                      {0.05f, 0.06f, 0.09f},
                      {0.0f,  0.0f,  0.0f},
                      0.0f, 0.0f};
      gs_SetFillLight(fill);
      gs_SetMaterial(mat);
      gs_SetLighting(1);
      gs_SetTexture(has_tex ? &tex : NULL);
      gs_DrawElemsLit(GS_TYPE_TRIANGLES, model.verts, model.n_verts,
                      model.index, model.n_index, model.normal,
                      has_tex ? model.texcoord : NULL);
      gs_SetLighting(0);
      gs_SetTexture(NULL);
      gs_ClearFillLight();
    } else if (use_ortho) {
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

    gs_Resolve();
    OSW_VideoDrawBuffer(framebuffer, FB_W, FB_H);
    OSW_VideoSwapBuffers();
  }

  return 0;
}

