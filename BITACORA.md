# Bitácora — Pipeline Gráfico por Software
**Trabajo Terminal | ESCOM-IPN**

Registro del desarrollo: decisiones de diseño, notas técnicas y su
correspondencia directa con el código. Las fórmulas se escriben **desarrolladas**
(sin notación Σ). Rutas relativas a la raíz del repositorio.

---

## Parte A — Notas → Implementación

Cada concepto muestra: la nota técnica, el estado ([x] / [-] / [ ]) y
exactamente **dónde y cómo** vive en el código.

---

### A.1 Interpolación perspectivamente correcta — [x]

**Nota:**

```
a_p = (b0·a0/w0 + b1·a1/w1 + b2·a2/w2)
      ─────────────────────────────────
            (b0/w0 + b1/w1 + b2/w2)
```

`b0,b1,b2` = coordenadas baricéntricas del fragmento;
`w0,w1,w2` = componente `w` de clip de cada vértice;
`a_p` = valor interpolado del atributo en el fragmento.

**Implementación:**

- [`plantilla/src/gs.c`](plantilla/src/gs.c) — `project()`: conserva `1/w`
  por vértice (`iw0,iw1,iw2`).
- `__gs_DrawTriangle`, bucle por píxel: calcula `wsum = b0/w0 + b1/w1 + b2/w2`
  y los pesos `p0,p1,p2 = bᵢ/wᵢ ÷ wsum`; color, normales y UV se interpolan
  con `p0,p1,p2` en vez de las baricéntricas crudas.
- La **profundidad** usa interpolación afín a propósito (`z/w` ya es lineal en
  pantalla; es la convención correcta para el z-buffer).

→ Ver nota [E.2](#e2-phong-vs-gouraud) para por qué reutilizar estos pesos
fue clave al añadir iluminación.

---

### A.2 Pipeline y espacios — [x]

**Nota:**

```
Aplicación → vert shader → rasterizador → frag shader → framebuffer

Local → Mundo (×M) → Vista (×V) → Clip (×P) → NDC (÷w) → Pantalla
```

Buffer constante: Luz, Material, Cámara, Textura.
El fragmento lleva `(n, P)` para iluminación en espacio mundo:
`P = M·pos`, `n = normalMat(M)·normal`, `vista = P − C`.

**Implementación:**

- Matrices `M/V/P` y `recalc_MVP` en [`plantilla/src/gs.c`](plantilla/src/gs.c);
  se actualizan desde [`plantilla/src/main.c`](plantilla/src/main.c) cada frame.
- Buffer constante → variables `static` en `gs.c`: `GsLight`, `GsMaterial`,
  `gs_camPos`, `GsTexture`, escritas con `gs_Set*`
  (declaradas en [`plantilla/include/gs.h`](plantilla/include/gs.h)).
- El fragmento recibe `(n, P)` interpolados en `__gs_DrawTriangle`.
- Se eligió **espacio mundo** (ver [E.3](#e3-espacio-mundo-vs-espacio-vista)).

Tabla de etapas detallada → [Parte D](#parte-d--etapas-del-pipeline).

---

### A.3 Componente difusa (Lambert) — [x]

**Nota:**

```
cos θ = l·n          (l, n normalizados)
I_d   = max(l·n, 0)
C_d   = base · I_d · L_d
```

**Implementación:**

- Rama `lit` de `__gs_DrawTriangle` ([gs.c](plantilla/src/gs.c)).
- Por fragmento: `L = normalize(luz.pos − P)`;
  `d = max(N·L, 0)`;
  `color += base · L · d`.
- Se activa con `gs_DrawElemsLit` desde la rama `show_model` de
  [main.c](plantilla/src/main.c).

→ Ver nota [E.2](#e2-phong-vs-gouraud).

---

### A.4 Componente especular — [x]

**Nota:**

```
r = reflect(l, n) = 2·(n·l)·n − l
I_e = max(r·v, 0)^e
```

`r` = reflexión de la luz sobre `n`; `v` = dirección hacia la cámara; `e` = exponente.

**Implementación:**

- Rama `lit` de `__gs_DrawTriangle` ([gs.c](plantilla/src/gs.c)).
- Solo si `N·L > 0`:
  `R = vec3_reflect(N, L)`;
  `V = normalize(camPos − P)`;
  `s = max(R·V, 0)^material.shininess · infl`.
- `camPos` llega por buffer constante con `gs_SetCameraPos(eye)` (main.c).

---

### A.5 Color final (Phong completo) — [x]

**Nota:**

```
C_final = C_e·I_e·L  +  C_d·I_d·L  +  C_a·I_a
```

`C_e` = color especular del material; `C_d/C_a` = color difuso/ambiente;
`L` = color de la luz; `I_a` = intensidad ambiente.

**Implementación:**

- Rama `lit` de `__gs_DrawTriangle` ([gs.c](plantilla/src/gs.c)):
  `C_final = base·(C_a·I_a + L·I_d) + C_e·L·I_e`, con clamp a [0,1].
- `GsMaterial { vec3 specular (C_e); float shininess (e); }` con `gs_SetMaterial`.
- `C_d / C_a` = color de vértice del `.tdm` (colapsados en el atributo);
  `L` = `GsLight.color`; `C_a·I_a` = `GsLight.ambient`.

→ Ver nota [E.2](#e2-phong-vs-gouraud).

---

### A.6 Atenuación por distancia — [x]

**Nota:**

```
Influencia = (1 / dist(luz, P))^f
```

`f` = exponente de fuerza. Multiplica tanto `I_d` como `I_e`.

**Implementación:**

- Rama `lit` de `__gs_DrawTriangle` ([gs.c](plantilla/src/gs.c)).
- Si `w = 1` y `atten_f > 0`:
  `dist = |luz.pos − P|`;
  `infl = powf(1/dist, atten_f)`;
  se multiplica a los términos difuso y especular.
- `atten_f` en `GsLight`; con `atten_f = 0` → sin atenuación.

---

### A.7 Tipo de luz — [x]

**Nota:**

```
l = (x, y, z, w)
  w = 0  →  luz direccional (sol): L = normalize(pos)
  w = 1  →  luz de punto:          L = normalize(pos − P), con atenuación
```

**Implementación:**

- Campo `w` en `GsLight` ([gs.h](plantilla/include/gs.h)).
- `__gs_DrawTriangle`: si `w ≥ 0.5` → luz de punto; si `w < 0.5` → direccional.
- [main.c](plantilla/src/main.c) usa hoy `w = 1` (punto).

---

### A.8 Texturas (muestreo) — [x]

**Nota:**

Espacio de texturas normalizado `(s, t) ∈ [0, 1]`, origen abajo-izquierda.
UV interpolada perspectiva-correcta como cualquier otro atributo del fragmento.

```
typedef struct Texture_t {
    u32  w, h;
    u32  filter;       // NEAREST | LINEAR
    u32  boundary_s;   // CLAMP | REPEAT | ESPEJO | COLOR_DEFECTO
    u32  boundary_t;
    u32  pix_format;   // 8888ABGR, 888BGR, 565BGR …
    u8  *data;
} Texture;
```

Modos de borde:
- **CLAMP:** `s = clamp(s, 0, 1)`
- **REPEAT:** `s = s − floor(s)`
- **ESPEJO:** refleja en cada repetición
- **COLOR POR DEFECTO:** devuelve un color fijo

Desnormalizar: `s *= (w − 1)`, `t *= (h − 1)`

Filtros:
- **NEAREST** — 1 consulta: `data[t·w + s]`. Pixeleado.
- **LINEAR (bilineal)** — 4 consultas + 3 `lerp` (en s dos veces, luego en t). Suavizado.

Frag shader:
```
vec2 uv    = buffer_attrib.uv;
vec4 color = sampleTex(tex, uv);
return color;
```

**Implementación:**

- `GsTexture { u32 w,h; u8 *data; int filter; int wrap; }` + `gs_SetTexture`
  ([gs.h](plantilla/include/gs.h) / [gs.c](plantilla/src/gs.c)) — buffer constante,
  mismo patrón que `GsLight`/`GsMaterial`.
- Cargador BMP genérico [bmp.c](plantilla/src/bmp.c) / [bmp.h](plantilla/include/bmp.h):
  valida 24bpp BI_RGB, guarda bottom-up con stride alineado a 4 bytes.
  El asset resultó en orden RGB (no BGR) → `tex_texel` lee `p[0]=R, p[1]=G, p[2]=B`.
- UV entra por **array paralelo** `t_arr` (`model.texcoord`, ya parseado por `tdm.c`);
  `gs_DrawElemsLit` recibe el parámetro `t_arr`.
- En `__gs_DrawTriangle`: `tex_wrap` (CLAMP/REPEAT/espejo) → desnormalizar
  `s·(w−1)`, `t·(h−1)` → `tex_sample` NEAREST o LINEAR. UV interpolada con
  `p0,p1,p2`. El texel **reemplaza el color base** antes del bloque de luz →
  Phong lo modula: `final = texel·(C_a·I_a + L·I_d) + C_e·L·I_e`.
- [main.c](plantilla/src/main.c) rama `show_model`: `bmp_Load` de
  `bob_esponja_tex.bmp` (LINEAR, REPEAT), `gs_SetTexture`,
  `gs_DrawElemsLit(... model.normal, model.texcoord)`.

---

## Parte B — Historial de implementación

1. **Corrección de bugs de compilación.** El bloque de declaraciones `static`
   en [gs.c](plantilla/src/gs.c) estaba revuelto e impedía compilar. Se reordenó.

2. **Algoritmo del pintor (transparencia estable).** La inestabilidad al girar
   el cubo transparente venía de que las caras no escriben el z-buffer. Solución
   en la rama perspectiva de [main.c](plantilla/src/main.c): centroide de cada
   cara en espacio cámara, insertion sort por z, reconstrucción de `sorted_idx`,
   dibujo de atrás hacia adelante. Alpha fijo en 0.4.

3. **Lector `.tdm` + versionado git.** Git inicializado con `.gitignore` para
   artefactos de build y `libosw/`. Módulo genérico
   [tdm.c](plantilla/src/tdm.c) / [tdm.h](plantilla/include/tdm.h): parsea
   posición, color, normal y texcoord; normaliza la geometría al cubo unitario.
   Carga por argumento de línea de comandos (`prog.exe <ruta.tdm>`).

4. **Interpolación perspectiva.** Ver [A.1](#a1-interpolación-perspectivamente-correcta----).

5. **Phong difuso + ambiente.** Ver [A.2](#a2-pipeline-y-espacios----) /
   [A.3](#a3-componente-difusa-lambert----) / [A.5](#a5-color-final-phong-completo----).
   `gs_DrawElemsLit` con array paralelo de normales; `normalMat(M)` calculado
   una sola vez por draw. Solo en el `.tdm`; cubo sin regresión.

6. **Phong completo.** Especular (`vec3_reflect`, `gs_SetCameraPos`), buffer
   `GsMaterial`, atenuación `(1/dist)^f`, tipo de luz por `w`. Ver
   [A.4](#a4-componente-especular----) – [A.7](#a7-tipo-de-luz----).

7. **Texturas.** Módulo `bmp.c/bmp.h`, `GsTexture` + `gs_SetTexture`, UV por
   array paralelo, `tex_sample` con CLAMP/REPEAT/espejo y NEAREST/LINEAR.
   UV perspectiva-correcta; texel modulado por Phong. Solo en el `.tdm`.
   Ver [A.8](#a8-texturas-muestreo----).

8. **Conversor GLTF → TDM (`recursos/gltf2tdm.py`).** Recorre la jerarquía
   de nodos del GLTF aplicando matrices mundo acumuladas. UV flip (`v = 1−v`)
   para que el origen quede abajo-izquierda (convenio BMP/OpenGL). Normales
   transformadas con **inverse-transpose** (`xf_normal`) para soportar escala
   no uniforme (koi_fish3 tiene Y ×10). Textura exportada como BMP 24bpp RGB
   (Pillow en Windows escribe RGB, no BGR; `tex_texel` lee `p[0]=R` — correcto).
   Uso: `python gltf2tdm.py scene.gltf -o out.tdm --tex`

9. **Controles interactivos continuos.** Array `keys[256]` de teclas sostenidas
   (scan codes PS/2); se rellena en el loop de eventos `OSW_KeyboardGetEvent`.
   Mouse drag con `OSW_MouseGetState` (btn0 = rotar, scroll = zoom).
   Toggle auto-spin con SPACE; toggle ortogonal/perspectiva con O.

10. **Luz de relleno (`gs_SetFillLight`).** Segunda luz difusa pura (sin especular),
    independiente de la luz principal. `w=0` direccional, `w=1` punto. Se aplica
    después del cálculo principal usando el color base guardado (`c_base`).
    Permite simular rebote de agua/cielo sin cambiar la API de `gs_SetLight`.
    Parámetros actuales: dirección {−0.6, −1.0, −1.2}, intensidad muy baja
    {0.05, 0.06, 0.09} para no revelar degradados de textura en el vientre del koi.

11. **Exploración de corrección gamma.** Se probó linearizar textura en `tex_texel`
    (`c² ≈ sRGB→linear`) y encodificar salida en `vec3_to_argb` (`√c ≈ linear→sRGB`).
    Resultado: el `c²` aplastaba valores medios a casi negro (zonas naranjas del koi
    aparecían como manchas negras) y el `√c` levantaba sombras revelando degradados
    de textura no deseados. Conclusión: gamma completa requiere tonemapping y es
    mejor implementarla en la variante OpenGL (Fase 2). La corrección se revirtió;
    solo se conservó el fill light.

---

## Parte C — Pendiente

- Sistema de plantas procedurales — ver `plantilla/PLAN_PLANTAS.md`.
- Variante OpenGL con PBR (Fase 2) — implementar en Linux, usar `OSW_FLAG_USE_OPENGL`.
- Escena completa del estanque chino: koi animados, bote, plantas, agua.
- Múltiples luces en el rasterizador software (hoy: principal + 1 fill).
- Gamma correction + tonemapping correctos (pendiente para variante OpenGL).

---

## Parte D — Etapas del Pipeline

Este renderer es por software (sin GPU); las "etapas" son funciones en
[gs.c](plantilla/src/gs.c).

| Etapa | Función | Dónde |
|---|---|---|
| **Aplicación** | Define geometría, matrices M/V/P por frame, estado (`gs_Set*`). | [main.c](plantilla/src/main.c) |
| **Buffer constante** | Estado global por draw: luz, material, cámara, textura. | `GsLight`/`GsMaterial`/`gs_camPos`/`GsTexture` + setters (gs.h/gs.c) |
| **Shader de vértices** | Transforma por MVP, calcula `1/w`; si hay luz: `P = M·pos`, `n = normalMat(M)·normal`. | `project()` + parte por-vértice de `__gs_DrawTriangle` (gs.c) |
| **Rasterizador** | Back-face culling (área con signo), bounding box recortado al viewport, baricéntricas (`edge_fn`), z-buffer. | `__gs_DrawTriangle` (gs.c) |
| **Shader de fragmentos** | Pesos perspectiva `p0,p1,p2`, interpolación de color/`(n,P)`/UV, textura, Phong, clamp. | Bucle por píxel de `__gs_DrawTriangle` (gs.c) |
| **Framebuffer / merge** | Alpha blending y escritura con z-buffer. | `poke` + `gs_zbuf` (gs.c) |

**Espacios:**

| Espacio | Transformación | Uso |
|---|---|---|
| **Local** | — | Vértices del modelo tal cual |
| **Mundo** | ×M | Iluminación: `P = M·pos`, `n = normalMat(M)·normal` |
| **Vista** | ×V | Centroide para painter's algorithm (cubo) |
| **Clip** | ×P | Genera la `w` que usa la interpolación perspectiva |
| **NDC** | ÷w | Cubo canónico [-1,1] independiente de resolución |
| **Pantalla** | mapeo viewport | Píxeles finales |

**NDC** (Normalized Device Coordinates): resultado de `clip ÷ w`
(`vec3_homogenize` en `project`). Coordenadas normalizadas de dispositivo:
cada eje en [-1, 1], independiente de la resolución, justo antes de mapear a píxeles.

**Atributo por-vértice vs constante:** `pos`, `color`, `normal`, `UV` son
**por-vértice** (uno por vértice, se interpolan en el fragmento).
Luz, material y textura son **constantes** (igual para todos los fragmentos del draw).

---

## Parte E — Notas conceptuales

### E.1 Buffer constante (uniforms): nota → código

**Nota:** existe un "buffer constante / uniforms" con **Luz, Material**
(**Textura** implementada — ver [A.8](#a8-texturas-muestreo----)).
El `frag_shader(x, y, buffer_attrib)` lee ese estado: lo *constante* viene del
buffer; lo *por-vértice* (UV, etc.) viene interpolado en `buffer_attrib`.

**En el código:** variables `static` en [gs.c](plantilla/src/gs.c):
`GsLight gs_light`, `GsMaterial gs_material`, `vec3 gs_camPos`,
`GsTexture *gs_tex`, `int gs_lighting`. Escritas con
`gs_SetLight` / `gs_SetMaterial` / `gs_SetCameraPos` / `gs_SetTexture` /
`gs_SetLighting` ([gs.h](plantilla/include/gs.h)).
[main.c](plantilla/src/main.c) los fija una vez por frame antes del draw;
`__gs_DrawTriangle` los lee igual para todos los fragmentos.

**Por-vértice vs constante:** `pos`, `color`, `normal` son **por-vértice**
(un valor por vértice, se interpolan). Luz, material, cámara y textura son
**constantes** (un valor por draw). Cambiar un uniform es gratuito; un atributo
nuevo implica tocar el formato de vértice o añadir un array paralelo.

### E.2 Phong vs Gouraud

La nota marca el pipeline como **vert shader → rasterizador → frag shader**
y especifica que el fragmento lleva `(n, P)`. Ahí está la diferencia:

- **Gouraud (por vértice):** iluminación evaluada en el *vert shader* (3 veces
  por triángulo); el rasterizador interpola el **color ya iluminado**. Barato,
  pero el especular se pierde/facetea en mallas de pocos polígonos.
- **Phong (por fragmento):** el *vert shader* solo prepara `(n, P)`; el
  rasterizador interpola `n` y `P`; la iluminación se evalúa en el *frag shader*
  (una vez por píxel). Más costoso, pero el especular y la curvatura salen correctos.

**Decisión:** **Phong por fragmento.** En `__gs_DrawTriangle` ([gs.c](plantilla/src/gs.c)):
`P` y `n` se calculan por vértice (parte "vert shader"), se interpolan con
`p0,p1,p2` y el Phong se evalúa en el bucle por píxel (parte "frag shader").
Los pesos perspectiva ya existían de [A.1](#a1-interpolación-perspectivamente-correcta----),
por lo que el salto a Phong no requirió infraestructura nueva.

### E.3 Espacio mundo vs espacio vista

La nota presenta las dos formas equivalentes:

| | Espacio mundo | Espacio vista |
|---|---|---|
| Posición fragmento | `P = M·Vin` | `P = V·M·Vin` |
| Dirección luz | `L = normalize(luz.pos − P)` | `L = normalize(V·luz.pos − P)` |
| Dirección vista | `V = normalize(camPos − P)` | `V = normalize(−P)` (cámara en origen) |
| Normal | `n = normalMat(M)·n_local` | `n = normalMat(V·M)·n_local` |

**En vista:** no hace falta pasar la posición de cámara, pero **la posición de
luz debe transformarse por `V` cada frame**.
**En mundo:** la luz se define en coordenadas intuitivas y no cambia al mover
la cámara, pero el especular **necesita `camPos`** (`vista = P − C`).

**Decisión: espacio mundo.** Por eso en [gs.c](plantilla/src/gs.c): existe
`gs_camPos` + `gs_SetCameraPos`; la luz **no** se multiplica por `V`;
`gs_normalMat = mat4_normalMat(M)` (no de `V·M`), calculado una vez por draw
en `gs_DrawElemsLit`.
