# Plan — Sistema de plantas acuáticas (estanque)

Vegetación procedural en C puro para la escena del estanque (koi + bote + nenúfares).
No se usa geometría descargada: las plantas se generan en código para tener control
total de variación, escala, color y animación, y porque los modelos gratuitos no se
encontraron.

**Estado:** Aprobado, pendiente de implementar.

---

## Plantas a generar

| Tipo | Función generadora | Polígonos aprox. |
|---|---|---|
| Nenúfar (lily pad) | `mesh_make_lilypad` | ~70 tris |
| Flor de loto | `mesh_make_lotus` | ~50 tris |
| Capullo (bud) | `mesh_make_bud` | ~30 tris |
| Trébol acuático (Marsilea) | `mesh_make_clover_patch` | ~80 tris/parche |
| Totora / espadaña (cattail) | `mesh_make_cattail` | ~200 tris |
| Tallo/raíz largo | `mesh_make_stem` | ~30 tris |

---

## Archivos a crear

- **`plantilla/include/plants.h`** — API pública
- **`plantilla/src/plants.c`** — constructores de mallas + siembra + dibujo

## Archivos a modificar

- **`plantilla/build_win.bat`** — añadir `src/plants.c` a la línea `cl`
- **`plantilla/src/main.c`** — llamar `plants_init` una vez y `plants_draw` por frame

---

## API pública (`plants.h`)

```c
void plants_init(PlantField *f, float bottom_y, u32 seed);
void plants_draw(PlantField *f, float time, vec3 cam_pos);
void plants_free(PlantField *f);
```

---

## Enganche con el pipeline existente

- `Vert { vec3 pos; vec3 color; }` — color RGB 0..1 (gs.h:16)
- Dibujo con luz: `gs_DrawElemsLit(GS_TYPE_TRIANGLES, verts, nv, idx, ni, normals, NULL)`
  El `t_arr` va NULL — las plantas usan color de vértice, sin textura.
- Backface cull a 0 (`gs_SetBackfaceCull(0)`) — reverso recibe solo ambiente
  → silueta oscura vista desde abajo (efecto correcto para nenúfares sumergidos).
- Matrices — orden obligatorio: `identity → scale → rotate → translate`
  `mat4_rotate(m, eje, angulo)` usa **grados**.

---

## Constructores de geometría

### A. Nenúfar — `mesh_make_lilypad(Mesh*, float R, u32 seed)`

Disco horizontal en XZ, anclaje en centro (y≈0).
- N=24 segmentos, 2 anillos (r=0.55R y r=R).
- **Muesca en V:** ángulo aleatorio, ancho ~14°. Segmentos en la muesca no generan triángulos.
- **Borde ondulado:** `r_out(θ) = R·(1 + 0.06·sin(7θ+seed) + 0.03·sin(13θ))`
- **Altura (ahuecado):** centro y=−0.05R, medio y=−0.02R, borde y=+0.04R·(1+0.5·sin(5θ))
- **Color por anillo:** centro verde-amarillo → borde verde oscuro, variación ±10% por seed.
- Normales suaves acumuladas.

### B. Flor de loto — `mesh_make_lotus(Mesh*, u32 seed, int variante)`

3 anillos de pétalos + centro amarillo.
- Externo: 8 pétalos, inclinación 72°, L=0.40, W=0.18
- Medio:   6 pétalos, inclinación 45°, L=0.38, W=0.16, azimut desfasado
- Interno: 5 pétalos, inclinación 18° (acopados), L=0.30, W=0.13
- Pétalo = 4 vértices (base, punta, lados), 2 triángulos CCW.
- Colores: rosa / blanco / durazno (variante aleatoria por seed).

### C. Capullo — `mesh_make_bud(Mesh*, u32 seed)`

Torno (lathe) 8 lados: perfil base→panza→punta. Color verde→rosa por altura. ~32 verts.

### D. Parche de trébol — `mesh_make_clover_patch(Mesh*, u32 count, u32 seed)`

Un mesh = un parche (1 draw call). count=6–10 tréboles en offsets aleatorios.
Cada trébol: tallito prisma 4 lados + 4 lóbulos en cruz (abanico pequeño).

### E. Totora — `mesh_make_cattail(Mesh*, u32 seed)`

Hojas en arco-S, tallo cilíndrico, cabeza café, plumero en punta. ~200 verts.

### F. Tallo/raíz — `mesh_make_stem(Mesh*, float len, u32 seed)`

Cilindro 6 lados, r≈0.02, curva suave, verde oscuro.
Se hornea dentro del mismo mesh del nenúfar/loto → planta + tallo se balancean como unidad.
Pivote en superficie → extremo del fondo ondea suavemente.

---

## Siembra (`plants_init`)

Estanque: superficie y=0, fondo `bottom_y`=−3.0, radio XZ ~7.
Carril central despejado (radio ~1.5) para el bote.

| Tipo | Cantidad | Distribución |
|---|---|---|
| Nenúfar | 16–22 | Dispersos, evitando centro |
| Loto | 6–9 | Encima de algunos nenúfares |
| Capullo | 5–8 | Junto a las flores |
| Parche trébol | 5–8 | Huecos y orillas |
| Totora | 6–10 | Perímetro exterior |

Por instancia: forma+color aleatorios horneados en vértices (yaw y escala aplicados),
más `pos`, `phase`, `sway_amp`, `bob_amp`, `sway_axis` para animación.

---

## Animación (`plants_draw`)

Solo matriz de modelo por frame (geometría fija → rápido).

```
θ = sway_amp · (sin(time·1.1 + phase) + 0.4·sin(time·2.3 + phase))
bob = bob_amp · sin(time·0.8 + phase·1.3)

mat4 M; mat4_identity(M);
mat4_rotate(M, sway_axis, θ_grados);
mat4_translate(M, {pos.x, pos.y + bob, pos.z});
gs_SetModelMatrix(M);
```

---

## Estado de render en `plants_draw`

```c
gs_SetLighting(1);
gs_SetTexture(NULL);
gs_SetAlpha(1.0f);
gs_SetBackfaceCull(0);
gs_SetMaterial((GsMaterial){{0.10f,0.10f,0.10f}, 8.0f});
// por instancia:
gs_DrawElemsLit(GS_TYPE_TRIANGLES, p->mesh.v, p->mesh.nv,
                p->mesh.idx, p->mesh.ni, p->mesh.n, NULL);
```

---

## Verificación al implementar

1. Build Windows: añadir `src/plants.c` a `build_win.bat` y compilar.
2. Arnés temporal en `main.c`: quad plano color agua en y=0 + `plants_init` + `plants_draw`,
   cámara picada `eye≈{0,5,4}`. Verificar nenúfares/flores/capullos/parches/totoras.
3. Vista submarina: cámara bajo y=0 → tallos deben colgar hasta el fondo,
   reversos como siluetas oscuras.
4. Animación: balanceo/flote suave y desincronizado.
5. Rendimiento: ~50 plantas → debe ir fluido a 640×480 con SSAA 2×.
