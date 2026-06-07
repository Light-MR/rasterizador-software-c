# CLAUDE.md — Contexto del proyecto

Guía para Claude Code al trabajar en este repositorio.

## Estado actual (junio 2026)

El proyecto evolucionó de MiniPaint (pixel-art 2D) a un **visor 3D con rasterizador
por software** que está siendo preparado para una escena de estanque chino.
El rasterizador software (`gs.c`) es el núcleo académico del Trabajo Terminal.

### Escena objetivo
Estanque chino con koi animados, bote, nenúfares y plantas procedurales.
Modelos externos en formato `.tdm`; plantas generadas en C (`plants.c`, pendiente).

---

## Build Commands

### Windows (MSVC — Visual Studio 2026 Insiders)
```
# El compilador está en VS 2026 Insiders, NO en la ruta estándar de Community.
# Siempre compilar via cmd con vcvars64.bat:
cmd /c "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" && cd /d "<ruta>\plantilla" && cl -Tc src/main.c src/gs.c src/trx.c src/tdm.c src/bmp.c -O2 -I".\include" -I"..\libosw\include" -link User32.lib Gdi32.lib Opengl32.lib Xinput9_1_0.lib ..\libosw\lib\Osw.lib -OUT:prog.exe

# Ejecutar (desde plantilla/):
prog.exe                          # cubo demo
prog.exe assets/koi.tdm          # koi (modelo elegido: koi_fish2)
prog.exe assets/lotus.tdm        # loto
prog.exe assets/boat.tdm         # bote (lento: 928K tris)
```

### Linux (GCC)
```bash
cd libosw && make
cd ../plantilla && ./build_linux.sh
./prog assets/koi.tdm
```

---

## Estructura del repositorio

```
GRAFICACION - copia/
├── libosw/              # Librería de ventana (gitignored — externa)
├── plantilla/           # Aplicación principal
│   ├── src/
│   │   ├── main.c       # Loop principal, controles, viewer 3D
│   │   ├── gs.c         # Rasterizador software (NÚCLEO ACADÉMICO)
│   │   ├── trx.c        # Matemáticas vectores/matrices
│   │   ├── tdm.c        # Cargador de modelos .tdm
│   │   └── bmp.c        # Cargador de texturas BMP
│   ├── include/
│   │   ├── gs.h         # API del rasterizador
│   │   ├── trx.h        # vec3, vec4, mat3, mat4
│   │   ├── tdm.h        # TdmModel
│   │   └── bmp.h        # bmp_Load
│   ├── assets/          # Modelos y texturas (convertidos con gltf2tdm.py)
│   │   ├── koi.tdm + koi.bmp       # koi_fish2 — MODELO ELEGIDO para escena
│   │   ├── koi3.tdm + koi3.bmp     # koi_fish3 (variante)
│   │   ├── koi_rr.tdm + koi_rr.bmp # koi low-poly (desechado)
│   │   ├── koi_orig.tdm            # koi_fish original 22K verts
│   │   ├── lotus.tdm + lotus.bmp   # flor de loto
│   │   └── boat.tdm                # bote (sin textura, 928K tris)
│   ├── PLAN_PLANTAS.md  # Plan detallado del sistema de plantas procedurales
│   └── BITACORA.md      # Registro técnico del desarrollo
├── recursos/
│   └── gltf2tdm.py      # Conversor GLTF → TDM (carpetas de modelos gitignored)
└── BITACORA.md          # Bitácora principal
```

---

## Formato TDM (modelos binarios)

```
Cabecera (16 B): char magic[4]="TDMD", u32 version, u32 n_verts, u32 n_index
Por vértice (36 B): f32 pos[3], f32 norm[3], f32 tex[2], u8 color[4] (RGBA)
Índices: n_index * u32 (triangle list, múltiplo de 3)
```

Conversión desde GLTF:
```bash
python recursos/gltf2tdm.py recursos/koi_fish2/scene.gltf -o plantilla/assets/koi.tdm --tex
# --tex exporta también koi.bmp (textura difusa en RGB 24bpp)
```

**Nota:** Pillow en Windows guarda BMP en orden RGB (no BGR). `tex_texel` lee
`p[0]=R, p[1]=G, p[2]=B` — correcto tal como está.

**Corrección de normales no-uniformes:** `gltf2tdm.py` usa inverse-transpose
(`xf_normal`) para nodos con escala no uniforme (koi_fish3 tiene Y 10× mayor que X,Z).

---

## gs.c — Rasterizador software

**Framebuffer interno:** buffer SSAA 2× (1280×960) → `gs_Resolve()` → 640×480 display.
**Pixel:** `0xAARRGGBB`. Color lineal 0..1 por vértice/fragmento.

### Pipeline por fragmento (`__gs_DrawTriangle`)
1. Proyectar 3 vértices por MVP, guardar `1/w` (interpolación perspectiva-correcta).
2. Back-face culling por área con signo.
3. Bounding-box recortado al viewport.
4. Por cada píxel: baricéntricas → z-buffer → pesos perspectiva → interpolar color/UV/normal.
5. Textura: si `gs_tex != NULL`, muestrear (NEAREST o LINEAR, CLAMP/REPEAT/MIRROR).
6. Phong por fragmento: `C = base·(C_a·I_a + L·I_d) + C_e·L·I_e` con clamp [0,1].
7. Fill light opcional (`gs_SetFillLight`): segunda luz difusa pura desde dirección contraria.

### API relevante (gs.h)
```c
void gs_SetLight(GsLight l);          // luz principal (pos, color, ambient, w, atten_f)
void gs_SetFillLight(GsLight l);      // luz de relleno (w=0 direccional, solo difusa)
void gs_ClearFillLight(void);
void gs_SetMaterial(GsMaterial m);    // specular + shininess
void gs_SetTexture(GsTexture *t);     // NULL = sin textura
void gs_SetLighting(int e);           // 0=off, 1=on
void gs_SetBackfaceCull(int e);       // 1=cull traseras (default), 0=ambas caras
void gs_SetAlpha(float a);            // 1.0=opaco, menor=semitransparente
void gs_DrawElemsLit(..., n_arr, t_arr); // draw con normales + UV
void gs_Resolve(void);                // SSAA 2× → framebuffer display
```

### Parámetros de luz actuales (main.c)
```c
GsLight luz = {
    {1.5f, 1.8f, 2.5f},
    {0.82f, 0.80f, 0.74f},
    {0.15f, 0.15f, 0.15f},
    1.0f, 1.0f };
GsMaterial mat = {{0.10f, 0.10f, 0.10f}, 16.0f};
GsLight fill = {{-0.6f, -1.0f, -1.2f}, {0.05f, 0.06f, 0.09f}, {0,0,0}, 0.0f, 0.0f};
```

---

## Controles del viewer (main.c)

```
Flechas / A-D        rotar en Y
Flechas arr/aba      rotar en X
Q / E                rotar en Z
W / S                acercar/alejar
Mouse BTN0 + drag    rotar libre
Scroll               zoom
O                    toggle perspectiva/ortogonal
SPACE                toggle auto-spin
```

Implementación: `keys[256]` bool para teclas sostenidas; `OSW_MouseGetState` para drag/scroll.

---

## Pendiente (próximas tareas)

1. **Sistema de plantas procedurales** — ver `plantilla/PLAN_PLANTAS.md`.
   Archivos a crear: `plantilla/src/plants.c`, `plantilla/include/plants.h`.
   Añadir `src/plants.c` a `build_win.bat`.

2. **Variante OpenGL (Fase 2)** — implementar en Linux (más simple que Windows para shaders).
   `OSW_FLAG_USE_OPENGL = 0x1` ya existe en `OSW_Init`.
   Plan: `gs_gl.c` con misma API que `gs.c`, PBR GLSL, carga TDM→VBO.

3. **Escena del estanque** — agua animada, koi en movimiento,
   integrar plantas + bote + koi en un solo render.

---

## Notas importantes

- **No pintar ojos en modelos koi.** Intentos anteriores fallidos (UV equivocada).
  El ojo visible es geometry-bump iluminado por Phong. Dejarlo así.
- **koi_orig.bmp es 49 MB** — gitignored. Regenerar con:
  `python recursos/gltf2tdm.py recursos/koi_fish/scene.gltf -o plantilla/assets/koi_orig.tdm --tex`
- **Carpetas GLTF en recursos/ son gitignored** (~273 MB). Solo `gltf2tdm.py` se versiona.
- **Modelo elegido para la escena:** `koi.tdm` (koi_fish2, 5371 verts, 9970 tris, textura 12MB).
- **Gamma correction:** se intentó (sqrt en salida + c² en textura) pero degradó los colores
  del koi. La corrección de gamma completa requiere tonemapping y es mejor hacerla en OpenGL.
  El fill light sí se mantuvo (muy suave: {0.05,0.06,0.09}).
