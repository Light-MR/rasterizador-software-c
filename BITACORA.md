# Bitácora — pipeline gráfico por software

Diario del trabajo: desde la corrección de bugs y el algoritmo del pintor
hasta la iluminación actual. Las fórmulas se escriben **desarrolladas** (sin
notación Σ). Convención de rutas: `plantilla/src`, `plantilla/include`.

---

## Parte A — Pizarrón → implementación

Cada concepto del pizarrón, su fórmula, el estado (✅ hecho / 🟡 parcial /
❌ falta) y **cómo/dónde** lo implementamos.

### A.1 Interpolación perspectivamente correcta — ✅

**Fórmula (desarrollada):**

```
a_p = (b0·a0/w0 + b1·a1/w1 + b2·a2/w2)
      ─────────────────────────────────
            (b0/w0 + b1/w1 + b2/w2)
```

donde `b0,b1,b2` son las baricéntricas y `w0,w1,w2` la `w` de clip de cada
vértice. `a_p` es el atributo interpolado del fragmento.

**Cómo/dónde:** en [project()](plantilla/src/gs.c) se dejó de descartar la `w`
del vértice: ahora devuelve también `1/w`. En `__gs_DrawTriangle`, dentro del
bucle por píxel, se calcula el denominador `wsum = b0/w0 + b1/w1 + b2/w2` y los
pesos perspectiva `p0,p1,p2` (cada `bᵢ/wᵢ` dividido entre `wsum`); el color
base se interpola con `p0,p1,p2` en vez de las baricéntricas crudas. La
**profundidad** se deja en interpolación afín a propósito (z/w ya es lineal en
pantalla; es lo correcto para el z-buffer).

### A.2 Pipeline y espacios — ✅

**Pizarrón:** Aplicación → shader de vértices → rasterizador → shader de
fragmentos → framebuffer. Espacios: local → mundo → vista → clip → pantalla.
Buffer constante (luz/material/vista). El fragmento lleva `(n, P)`.
Cálculo de iluminación en espacio global:
`P = M·pos`, `luz = l`, `vista = P − C`, `n = normalMat(M)·normal`.

**Cómo/dónde:** matrices M/V/P y `recalc_MVP` en [gs.c](plantilla/src/gs.c);
se fijan desde [main.c](plantilla/src/main.c) cada frame. El "buffer
constante" se modeló como `GsLight` + `GsMaterial` + `gs_SetLight` /
`gs_SetMaterial` / `gs_SetCameraPos` / `gs_SetLighting`
([gs.h](plantilla/include/gs.h), gs.c). El fragmento recibe `(n, P)`
interpolados en `__gs_DrawTriangle`. Se eligió **espacio mundo**:
`P = M·pos`, `n = normalMat(M)·normal`, `vista = P − C` (con
`gs_SetCameraPos`). Ver detalle de etapas en la Parte D.

### A.3 Componente difuso (Lambert) — ✅

**Fórmula:** el coseno del ángulo entre la luz `l` y la normal `n`:

```
cos θ = (l·n) / (|l|·|n|)      con l, n normalizados →   cos θ = l·n
I_d   = max(l·n, 0)
C_d   = base · max(l·n, 0) · L_d
```

**Cómo/dónde:** rama `lit` de `__gs_DrawTriangle` en
[gs.c](plantilla/src/gs.c). Por píxel se interpolan (perspectiva) la posición
mundo `P` y la normal mundo `N` (renormalizada); `L = normalize(luz.pos − P)`;
`d = max(N·L, 0)`; `color = base · (ambiente + L_d·d)`, con clamp a [0,1]. Se
activa con `gs_DrawElemsLit` desde la rama `show_model` de
[main.c](plantilla/src/main.c).

### A.4 Componente especular — ✅

**Fórmula:** `I_e = max((r·v)^e, 0)`, con `r` = reflexión de la luz sobre `n`,
`v` = dirección a la cámara, `e` = constante de brillo (especularidad).

**Cómo/dónde:** rama `lit` de `__gs_DrawTriangle` en
[gs.c](plantilla/src/gs.c). Solo si la cara está iluminada (`N·L > 0`):
`R = vec3_reflect(N, L)`, `V = normalize(camPos − P)`,
`s = max(R·V, 0)^material.shininess · infl`. La posición de cámara llega por
el buffer constante con `gs_SetCameraPos(eye)` desde main.c (esto resuelve el
`vista = P − C` del pizarrón).

### A.5 Color final — ✅

**Fórmula:**

```
C_final = C_e·I_e·L_e  +  C_d·I_d·L_d  +  C_a·I_a
Material = (C_e, C_d, C_a, e)
```

**Cómo/dónde:** los tres términos en la rama `lit` de `__gs_DrawTriangle`
(gs.c): `C_final = base·(C_a·I_a + L·I_d) + C_e·L·I_e`, con clamp a [0,1].
Existe el buffer constante `GsMaterial { vec3 specular(C_e); float
shininess(e); }` con `gs_SetMaterial` (gs.h/gs.c). `C_d` y `C_a` siguen siendo
el color de vértice del `.tdm` (material difuso/ambiente colapsado en el
atributo de vértice); `L` = `GsLight.color`, `C_a·I_a` = `GsLight.ambient`.

### A.6 Atenuación por distancia — ✅

**Fórmula:** `Influencia = (1 / dist(luz, P))^f` (f = "fuerza"). Multiplica
tanto `I_d` como `I_e`.

**Cómo/dónde:** rama `lit` de `__gs_DrawTriangle` (gs.c). Si `w=1` y
`atten_f > 0`: `dist = |luz.pos − P|`, `infl = powf(1/dist, atten_f)`, y se
multiplica a `d` (difuso) y `s` (especular). `atten_f` viene en `GsLight`;
con `atten_f = 0` no hay atenuación. Se subió `GsLight.color` en main.c para
compensar la atenuación.

### A.7 Tipo de luz — ✅

**Fórmula:** la luz se da como `l = (x, y, z, w)`; `w = 0` → luz direccional
(el sol), `w = 1` → luz de punto (se usa `P − l`).

**Cómo/dónde:** campo `w` en `GsLight`. En `__gs_DrawTriangle`: si
`w ≥ 0.5` → luz de punto (`L = normalize(pos − P)`, con atenuación);
si `w < 0.5` → direccional (`L = normalize(pos)`, `pos` es la dirección hacia
la luz, sin atenuación). main.c usa hoy `w = 1`.

### A.8 Texturas (muestreo) — ❌ falta (apuntes del pizarrón)

Spec del pizarrón transcrita; aún no implementado.

**Espacio de texturas normalizado:** las coordenadas `(s, t)` van en
`[0.0, 1.0]`, con origen abajo-izquierda; `(0,0)` esquina inferior izq,
`(1,1)` superior derecha. La UV se interpola **perspectiva-correcta** como
cualquier otro atributo del fragmento.

**Struct de textura:**

```
typedef struct Texture_t {
    u32  w;            // ancho en texels
    u32  h;            // alto
    u32  filter;       // NEAREST | LINEAR
    u32  boundary_s;   // CLAMP | REPEAT | ESPEJO | COLOR_DEFECTO
    u32  boundary_t;
    u32  pix_format;   // 8888ABGR, 888BGR, 565BGR, 565RGB, 1555ABGR…
    u8  *data;         // píxeles
} Texture;
```

**¿Qué pasa si me salgo de la textura? (bordes):**
- **Color por defecto:** devuelve un color fijo.
- **REPEAT:** `s = fmod(s, 1.0)` (parte fraccionaria); igual con `t`.
- **Espejo:** refleja en cada repetición.
- **CLAMP:** `s = clamp(s, 0.0, 1.0)`; igual con `t`.

**Desnormalizar** (de [0,1] a índices de texel):
`s *= (tx->w − 1)`, `t *= (tx->h − 1)`.

**Filtro (¿qué texel devuelvo?):**
- **NEAREST** — 1 consulta: `return tx->data[t·tx->w + s]`. Pixeleado.
- **LINEAR (bilineal)** — 4 consultas + 3 interpolaciones: con
  `frac_s = fmod(s, 1.0)` e `inc_s = (frac_s ≥ 0.5 ? 1 : −1)`,
  tomar `a = data[t·w + s]`, `b = data[t·w + s + inc_s]`,
  `ab = lerp(a, b, frac_s − 0.5)`; repetir el mismo par en la dirección `t`
  y volver a interpolar. Suaviza.

**Formato del pixel:** la textura puede venir empaquetada distinto
(`u32` = 8A8B8G8R / 8888ABGR; `u16` = 5B6G5R / 565RGB; 1555ABGR). Hay que
guardar `w`/`h` y `pix_format` para desempaquetar al consultar.

**Frag shader (pizarrón):**
`vec2 uv = buffer_attrib.uv;  vec4 color = sampleTex(tex, uv);  return color;`
La textura es parte del **buffer constante** (uniforms); la UV es un
**atributo por-vértice**.

**Dónde irá en NUESTRO pipeline (pendiente):**
[tdm.c](plantilla/src/tdm.c) ya parsea `model.texcoord` (las UV del `.tdm`,
hoy sin uso). Falta: (a) llevar la UV al fragmento — array paralelo como se
hizo con las normales en `gs_DrawElemsLit`, o ampliar `Vert`; (b) un struct
`Texture` + cargador del `bob_esponja/bob_esponja_tex.bmp`; (c) muestrear en
la rama del shader de fragmentos de `__gs_DrawTriangle` interpolando la UV con
los pesos perspectiva `p0,p1,p2` y multiplicar el texel por el color
iluminado.

---

## Parte B — Cronología del trabajo

1. **Corrección de bugs de compilación.** En [gs.c](plantilla/src/gs.c) el
   bloque de declaraciones `static` (framebuffer, viewport, matrices, flags)
   estaba textualmente revuelto e impedía compilar. Se reordenó. Resultado:
   compila limpio.

2. **"Se enciman los colores" → algoritmo del pintor.** Diagnóstico: no era un
   bug, era la transparencia del 40 % intencional. La inestabilidad al girar
   venía de que las caras transparentes **no escriben el z-buffer** (gs.c), así
   que la mezcla dependía del orden de los índices, no de la profundidad.
   Solución en la rama perspectiva del cubo de [main.c](plantilla/src/main.c):
   se calcula el centroide de cada cara en espacio cámara, se ordenan por z
   (insertion sort), se reconstruyen los índices (`sorted_idx`) y se dibujan
   de atrás hacia adelante (painter's algorithm). Decisión del usuario:
   mantener alpha 0.4.

3. **Lector `.tdm` + versionado git (hito de soporte).** Se inicializó git
   como red de seguridad (con `.gitignore` para artefactos de build y la
   `libosw/` externa). Se creó el módulo genérico
   [tdm.c](plantilla/src/tdm.c) / [tdm.h](plantilla/include/tdm.h): parsea
   posición, color, normal y texcoord, valida el formato y normaliza la
   geometría al tamaño del cubo unitario. Selección por argumento de línea de
   comandos (`prog.exe <ruta.tdm>`; sin args = cubo). Relevante para lo que
   sigue porque la iluminación consume `model.normal`.

4. **Interpolación perspectiva.** Ver Parte A.1.

5. **Iluminación Phong difusa + ambiente.** Ver Parte A.2/A.3/A.5. Resumen:
   `GsLight` como buffer constante; `gs_DrawElemsLit` toma un array paralelo
   de normales y calcula `normalMat(M)` una sola vez por draw; iluminación por
   fragmento en espacio mundo; aplicada **solo al `.tdm`** (el cubo quedó
   intacto, sin regresión).

6. **Phong completo del pizarrón.** Se añadió el especular (`vec3_reflect`,
   `gs_SetCameraPos`), el buffer `GsMaterial` (`C_e`, `e`), la atenuación
   `(1/dist)^f` y el tipo de luz por `w` (direccional/punto). El color final
   ya es `C_e·I_e·L + C_d·I_d·L + C_a·I_a`. Sigue solo en el `.tdm`.

---

## Parte C — Qué falta

- **`C_d`/`C_a` como material real** separado del color de vértice (hoy el
  difuso/ambiente sale del atributo de vértice del `.tdm`; `GsMaterial` solo
  lleva `C_e` y `e`).
- **Varias luces** (hoy una sola; el modelo soporta direccional o punto vía
  `w` pero no acumula múltiples).
- **Texturas:** las UV ya se parsean en el `.tdm`, pero `Vert` no las lleva
  ni se muestrea la textura `bob_esponja/bob_esponja_tex.bmp`. Spec completa
  en **A.8**.
- **Mallas externas:** winding mixto / sombreado a dos caras (hoy `cull=0`,
  las caras opuestas a la luz quedan solo en ambiente).
- **Iluminación en el cubo** (hoy solo en el `.tdm`).
- Opcional: interpolación perspectiva también en líneas y puntos.

---

## Parte D — Notas de conceptos (a qué etapa pertenece cada cosa)

Este renderer es por software (sin GPU); las "etapas" son funciones en
[gs.c](plantilla/src/gs.c).

| Etapa | Qué hace aquí | Dónde |
|---|---|---|
| **Aplicación** | Define geometría (cubo / modelo `.tdm`), matrices M/V/P por frame, y la luz. Decide qué dibujar y el estado (`gs_Set*`). | [main.c](plantilla/src/main.c) |
| **Buffer constante / uniforms** | Estado global, no por-vértice: luz, material y posición de cámara (y, a futuro, textura). | `GsLight`/`GsMaterial`/`gs_camPos` + `gs_SetLight`/`gs_SetMaterial`/`gs_SetCameraPos`/`gs_SetLighting` (gs.h/gs.c) |
| **Shader de vértices** | Transforma cada vértice por MVP, calcula `1/w`; si hay luz, `P = M·pos` y `n = normalMat(M)·normal`. | `project()` + parte por-vértice de `__gs_DrawTriangle` (gs.c) |
| **Rasterizador** | Back-face culling (área con signo), bounding box recortado al viewport, baricéntricas (`edge_fn`), test de profundidad (z-buffer). | `__gs_DrawTriangle` (gs.c) |
| **Shader de fragmentos** | Pesos perspectiva `p0,p1,p2`, interpolación de color y de `(n,P)`, iluminación difusa+ambiente, clamp. | bucle por píxel de `__gs_DrawTriangle` (gs.c) |
| **Framebuffer / merge** | Escribe el píxel con alpha blending y z-buffer. | `poke` + `gs_zbuf` (gs.c) |

**Los espacios (y dónde ocurren):**

- **Local / modelo:** vértices tal cual (cubo hardcodeado; el `.tdm` ya viene
  normalizado por `tdm_Load`).
- **Mundo:** ×M. Aquí se hace la iluminación (decisión: espacio global):
  `P = M·pos`, `n = normalMat(M)·normal`.
- **Vista / cámara:** ×V (`mat4_lookAt` en main.c). El algoritmo del pintor
  del cubo usa la z en este espacio (centroide de cara).
- **Clip:** ×P; aparece la `w` (la que usa la interpolación perspectiva).
- **NDC** (Normalized Device Coordinates / coordenadas normalizadas de
  dispositivo): resultado de dividir clip ÷w (`vec3_homogenize` en
  `project`). Es un cubo canónico con cada eje en el rango [-1, 1],
  independiente de la resolución de pantalla, justo antes de convertir a
  píxeles.
- **Pantalla:** mapeo al viewport (en `project`).
- `recalc_MVP()` precompone `P·V·M`; las matrices se fijan con
  `gs_SetModelMatrix` / `gs_SetViewMatrix` / `gs_SetProjMatrix`.

**Atributo por-vértice vs constante:** `pos`, `color` y (vía array paralelo)
`normal` son **por-vértice** y se interpolan en el fragmento; la luz y el
material son **constantes** (buffer constante), iguales para todos los
fragmentos de un mismo draw.

---

## Parte E — Conceptos ampliados

### E.1 Buffer constante (uniforms): pizarrón → código

**Pizarrón:** existe un "buffer constante / uniforms" con **Luz, Material**
(y, a futuro, **Textura**). El `frag_shader(x, y, buffer_attrib)` lee ese
estado: lo *constante* viene del buffer; lo *por-vértice* (UV, etc.) viene
interpolado en `buffer_attrib`.

**Cómo lo pusimos en código:** el buffer constante son variables `static` en
[gs.c](plantilla/src/gs.c) — `GsLight gs_light`, `GsMaterial gs_material`,
`vec3 gs_camPos`, `int gs_lighting` — escritas con los setters
`gs_SetLight` / `gs_SetMaterial` / `gs_SetCameraPos` / `gs_SetLighting`
(declarados en [gs.h](plantilla/include/gs.h)). [main.c](plantilla/src/main.c)
los fija una vez por frame antes del `gs_DrawElemsLit`; la rama `lit` de
`__gs_DrawTriangle` los lee igual para todos los fragmentos del draw.

**Constante vs por-vértice:** `pos`, `color`, `normal` son **por-vértice**
(uno por vértice, se interpolan). Luz, material y cámara son **constantes**
(un valor por draw). La diferencia es clave: el costo de cambiar un uniform es
ínfimo; un atributo nuevo implica tocar el formato de vértice / un array
paralelo.

### E.2 Phong vs Gouraud (relación con el pizarrón)

El pizarrón dibuja el pipeline como **shader de vértices → rasterizador →
shader de fragmentos**, y marca que el fragmento lleva `(n, P)`. Ahí está
exactamente la diferencia entre los dos modelos de sombreado:

- **Gouraud (por vértice):** la iluminación se evalúa en el **shader de
  vértices**, una vez por cada uno de los 3 vértices; el rasterizador
  **interpola el color ya iluminado**. Barato (3 evaluaciones por triángulo),
  pero el brillo especular se "rompe"/facetea en mallas de pocos polígonos
  porque el pico del especular puede caer entre vértices y perderse.
- **Phong (por fragmento):** el shader de vértices solo prepara `(n, P)`; el
  rasterizador **interpola `n` y `P`**, y la iluminación se evalúa en el
  **shader de fragmentos**, una vez por píxel. Más caro, pero el especular y
  la curvatura salen suaves y correctos.

**Qué elegimos y dónde:** **Phong por fragmento**. En
[gs.c](plantilla/src/gs.c), `__gs_DrawTriangle` calcula `P` y `n` en mundo por
vértice (parte "shader de vértices") y, en el bucle por píxel, interpola
`(n, P)` con los pesos perspectiva `p0,p1,p2` y evalúa difuso+especular+
ambiente (parte "shader de fragmentos"). Reusar esos pesos es lo que hace
barato el salto a Phong. Gouraud no se implementó; habría requerido evaluar la
luz antes de rasterizar e interpolar solo el color (como ya se interpola hoy
el color base).

### E.3 Diferencia: espacio mundo vs espacio vista

El pizarrón da las dos formas de hacer el mismo cálculo (todo desarrollado):

- **Global / mundo:** `P = M·Vin`, `luz = l`, `vista = P − C`,
  `n = normalMat(M)·normal`.
- **Vista / cámara:** `P = V·M·Vin`, `luz = V·l`, `vista = −P` (la cámara
  está en el origen), `n = normalMat(V·M)·normal`.

**Diferencias prácticas:**
- En **vista** no hace falta pasar la posición de cámara (el vector vista es
  `−P` porque la cámara está en el origen), pero **la luz debe transformarse
  por `V` cada frame** (la `V` cambia con el zoom/cámara).
- En **mundo** la luz se define en coordenadas intuitivas y **no** se toca al
  mover la cámara, pero el especular **necesita la posición de cámara**
  (`vista = P − C`).
- Las normales se transforman con `normalMat` de `M` (mundo) o de `V·M`
  (vista); en ambos casos es la traspuesta de la inversa de la 3×3.

**Qué elegimos e implicaciones en el código:** **espacio mundo**. Por eso en
[gs.c](plantilla/src/gs.c): existe `gs_camPos` + `gs_SetCameraPos` (se usa en
`vista = camPos − P`); la luz **no** se multiplica por `V` (se usa
`gs_light.pos` tal cual); y `gs_normalMat` se calcula con
`mat4_normalMat(M)` (no de `V·M`) una vez por draw en `gs_DrawElemsLit`.
