# Pipeline Gráfico por Software


Rasterizador 3D en C sin GPU: proyección perspectiva, interpolación perspectiva-correcta,
iluminación Phong por fragmento (difuso, especular, ambiente, atenuación) y
muestreo de texturas bilineal. Incluye MiniPaint, una aplicación de dibujo pixel-art.

---

## Estructura del proyecto

```
.
├── libosw/               ← librería de ventanas (dep. externa, no versionada)
├── plantilla/            ← pipeline 3D + MiniPaint
│   ├── include/          ← gs.h, trx.h, tdm.h, bmp.h
│   ├── src/
│   │   ├── main.c        ← pipeline 3D (cubo + modelos .tdm)
│   │   ├── main_paint.c  ← MiniPaint
│   │   ├── gs.c          ← rasterizador por software
│   │   ├── trx.c         ← álgebra vectorial/matricial
│   │   ├── tdm.c         ← lector de modelos .tdm
│   │   └── bmp.c         ← cargador de texturas BMP 24bpp
│   ├── build_win.bat     ← compila el pipeline 3D → prog.exe
│   └── build_paint.bat   ← compila MiniPaint       → paint.exe
├── bob_esponja/
│   ├── bob_esponja.tdm
│   └── bob_esponja_tex.bmp
└── BITACORA.md           ← registro pizarrón → implementación
```

---

## Compilar y ejecutar (Windows, MSVC x64)

**Requisito previo:** tener `libosw/` en la raíz y compilarla una sola vez:
```
cd libosw
build_win.bat
cd ..
```

### Pipeline 3D

```bat
cd plantilla
build_win.bat

rem Cubo interactivo:
prog.exe

rem Modelo con textura y Phong:
prog.exe ..\bob_esponja\bob_esponja.tdm
```

**Controles del pipeline 3D:**

| Tecla | Acción |
|-------|--------|
| Flechas ← → | Rotar en Y |
| Flechas ↑ ↓ | Rotar en X |
| Q / E | Rotar en Z |
| W / S | Acercar / alejar cámara |
| O | Alternar perspectiva / ortogonal |

### MiniPaint

```bat
cd plantilla
build_paint.bat
paint.exe
```

**Controles de MiniPaint:**

| Acción | Descripción |
|--------|-------------|
| Clic en lienzo | Dibujar con lápiz (Bresenham) |
| Clic en paleta | Seleccionar color |
| Clic en cajas superiores | Intercambiar color actual / anterior |
| S | Guardar como `dibujo.ppm` |
