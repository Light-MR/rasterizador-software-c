# MiniPaint — Documentacion Tecnica

## Descripcion general

MiniPaint es una aplicacion de dibujo de pixel art construida sobre la libreria **libosw**, que proporciona una ventana con un framebuffer de acceso directo. Todo el renderizado se hace escribiendo valores de color en un arreglo de `u32` — no se usa ninguna API grafica de alto nivel.

La pantalla de 320x240 pixeles se divide en dos zonas:

```
 0                 80  81                          319
 +------------------+--+-----------------------------+
 | Caja actual |ant |  |                             |
 |   (40x24)   |    |  |                             |
 +------------------+  |                             |
 |                  |  |        LIENZO / CANVAS       |
 |  Paleta 8x8     |  |        (239 x 240 px)        |
 |  (80x80 px)     |  |                             |
 |                  |  |                             |
 +------------------+  |                             |
 |  Area sobrante   |  |                             |
 |  (no pintable)   |  |                             |
 +------------------+--+-----------------------------+
       PALETA_W=80  sep(1px)
```

### Controles

| Accion | Control |
|---|---|
| Dibujar en el lienzo | Click izquierdo sostenido en el canvas |
| Seleccionar color | Click izquierdo en una celda de la paleta |
| Intercambiar color actual/anterior | Click izquierdo en las cajas superiores |
| Guardar imagen | Tecla S (genera `dibujo.ppm`) |

---

## Constantes y configuracion

```c
#define FB_W      320          // Ancho total del framebuffer en pixeles
#define FB_H      240          // Alto total del framebuffer en pixeles
#define PALETA_W   80          // Ancho del panel lateral izquierdo

#define BOX_H      24          // Alto de las cajas de color seleccionado
#define BOX_W     (PALETA_W/2) // Ancho de cada caja (40px)

#define PAL_COLS    8          // Columnas en la grilla de colores
#define PAL_ROWS    8          // Filas en la grilla de colores
#define PAL_START_Y BOX_H      // Y donde empieza la paleta (debajo de las cajas)
#define CELL_W    (PALETA_W/PAL_COLS)  // Ancho de cada celda: 10px
#define CELL_H     10          // Alto de cada celda: 10px
#define PAL_GRID_H (PAL_ROWS * CELL_H) // Alto total de la grilla: 80px

#define KEYCODE_S  0x1F        // Scan code PS/2 de la tecla 'S'
```

### Formato de color: ARGB

libosw usa pixeles de 32 bits en formato `0xAARRGGBB`:
- Bits 31-24: Alpha (siempre `0xFF` = opaco)
- Bits 23-16: Rojo
- Bits 15-8: Verde
- Bits 7-0: Azul

Ejemplo: `0xFFFF0000` = rojo puro, `0xFF00FF00` = verde puro.

### Variables globales

```c
u32 framebuffer[FB_W * FB_H];  // Arreglo 1D que representa la imagen 2D
u32 current_color = 0xFFEED6DC; // Color activo para dibujar (rosa por defecto)
u32 prev_Color    = 0xFFC9B7E8; // Color anterior (morado por defecto)
```

El framebuffer es un arreglo lineal. Para acceder al pixel en la posicion `(x, y)` se usa el indice `y * FB_W + x`, porque los pixeles estan almacenados fila por fila de izquierda a derecha, de arriba hacia abajo.

---

## Funciones de dibujo

### `fillArea(color, x, y, w, h)`

**Que hace:** Rellena un rectangulo solido de un solo color dentro del framebuffer.

**Por que existe:** Es la operacion mas basica y frecuente del programa. Se usa para:
- Pintar el fondo del lienzo al inicio
- Dibujar cada celda de la paleta de colores
- Dibujar las cajas de color seleccionado/anterior
- Dibujar el separador vertical
- Rellenar el area sobrante debajo de la paleta

Sin esta funcion, habria que escribir dos bucles anidados cada vez que se necesite pintar un area rectangular, duplicando codigo y aumentando la probabilidad de errores.

**Como funciona:**
1. Recibe la esquina superior izquierda `(x, y)` y las dimensiones `w` (ancho) y `h` (alto).
2. Itera fila por fila: `j` va desde `y` hasta `y + h - 1`.
3. Dentro de cada fila, itera columna por columna: `i` va desde `x` hasta `x + w - 1`.
4. Para cada punto `(i, j)`, calcula su posicion lineal en el framebuffer como `j * FB_W + i` y le asigna el color.

**Por que no tiene bounds check:** Solo se invoca desde el codigo de UI con coordenadas calculadas a partir de constantes conocidas en tiempo de compilacion (`PALETA_W`, `BOX_H`, etc.), por lo que nunca se salen del framebuffer.

---

### `set_pixel(x, y, color)`

**Que hace:** Escribe un solo pixel en el framebuffer, verificando primero que las coordenadas esten dentro de los limites.

**Por que existe:** A diferencia de `fillArea`, esta funcion se usa con coordenadas que vienen del mouse o del algoritmo de Bresenham, las cuales pueden ser negativas o exceder los bordes del framebuffer. Sin esta verificacion, escribir fuera de los limites del arreglo causaria corrupcion de memoria o un crash.

**Como funciona:**
1. Verifica que `x >= 0`, `y >= 0`, `x < FB_W`, `y < FB_H`. Si alguna condicion falla, retorna sin hacer nada.
2. Si pasa la verificacion, escribe el color en `framebuffer[y * FB_W + x]`.

**Por que usa `s32` y no `u32`:** El algoritmo de Bresenham trabaja con numeros con signo (`s32`) porque las coordenadas pueden ser temporalmente negativas durante el calculo. Si se usara `u32` (sin signo), un valor como `-1` se interpretaria como `4294967295`, pasando la verificacion de limites y corrompiendo memoria.

---

### `esLienzo(x, y)`

**Que hace:** Determina si una coordenada `(x, y)` esta dentro de la zona pintable (el lienzo/canvas).

**Por que existe:** La pantalla tiene dos zonas: la paleta (panel izquierdo, 0 a 80px) y el lienzo (81 a 319px). Cuando el usuario dibuja o cuando Bresenham genera pixeles, necesitamos asegurarnos de no pintar sobre los elementos de la UI. Esta funcion centraliza esa decision.

**Como funciona:**
- Si `x > PALETA_W` (es decir, `x > 80`), retorna `1` (es lienzo).
- En cualquier otro caso, retorna `0` (es zona de UI).

El cast `(s32)PALETA_W` es necesario porque `x` es `s32` (con signo) y `PALETA_W` es un `#define` que el compilador trata como `unsigned`. Sin el cast, la comparacion entre signed y unsigned puede dar resultados incorrectos (el compilador incluso lanza un warning).

**Relacion con el clipping:** Esta funcion es la primera capa de clipping del programa. `set_pixel` verifica los bordes del framebuffer; `esLienzo` verifica que no se pinte sobre la UI. Juntas, garantizan que el dibujo solo aparezca en el canvas.

---

## Algoritmo de Bresenham

### El problema

Cuando el usuario mueve el mouse rapido mientras dibuja, el sistema operativo reporta la posicion del cursor solo cada ciertos milisegundos. Esto significa que entre un frame y el siguiente, el mouse puede haber saltado varios pixeles. Si solo pintaramos un pixel por frame, quedarian huecos visibles en el trazo.

La solucion es trazar una **linea recta** entre la posicion anterior del cursor y la posicion actual, rellenando todos los pixeles intermedios.

### La matematica

Una linea recta entre dos puntos `(x0, y0)` y `(x1, y1)` se define por la ecuacion:

```
y - y0     y1 - y0
────── = ─────────
x - x0     x1 - x0
```

Reorganizando: `(y - y0)(x1 - x0) = (x - x0)(y1 - y0)`

El problema es que calcular la pendiente `dy/dx` requiere division y numeros de punto flotante, lo cual es lento y puede acumular errores de redondeo. Bresenham resuelve esto usando **solo sumas, restas y multiplicaciones por 2** (que son shifts de bits).

### La idea clave: variable de error

En lugar de calcular la posicion exacta `y` para cada `x`, Bresenham mantiene una variable de **error acumulado** que indica que tan lejos esta el pixel actual de la linea ideal:

```
err = dx - dy
```

Donde `dx = |x1 - x0|` y `dy = |y1 - y0|`.

En cada paso:
1. Se calcula `e2 = err * 2`
2. Si `e2 > -dy`: el error indica que debemos avanzar en X → `err -= dy`, `x += sx`
3. Si `e2 < dx`: el error indica que debemos avanzar en Y → `err += dx`, `y += sy`

`sx` y `sy` son las direcciones de paso (+1 o -1), determinadas por si el punto final esta a la derecha/izquierda o arriba/abajo del punto inicial.

### Ejemplo visual

Trazar linea de `(2, 1)` a `(8, 4)`:
- `dx = 6`, `dy = 3`, `err = 6 - 3 = 3`
- `sx = +1`, `sy = +1`

```
Paso  x  y  err  e2   avanza_X?  avanza_Y?
  0   2  1   3    6    si(6>-3)   no(6>6?)   → err=0, x=3
  1   3  1   0    0    si(0>-3)   si(0<6)    → err=-3,x=4; err=3,y=2
  2   4  2   3    6    si(6>-3)   no(6>6?)   → err=0, x=5
  3   5  2   0    0    si(0>-3)   si(0<6)    → err=-3,x=6; err=3,y=3
  4   6  3   3    6    si(6>-3)   no         → err=0, x=7
  5   7  3   0    0    si(0>-3)   si(0<6)    → err=-3,x=8; err=3,y=4
  6   8  4   —    fin
```

Resultado: pixeles en `(2,1) (3,1) (4,2) (5,2) (6,3) (7,3) (8,4)` — una linea sin huecos.

### Implementacion en `dibujaLinea`

```c
void dibujaLinea(s32 x0, s32 y0, s32 x1, s32 y1){
    s32 dx = x1-x0; if (dx < 0) dx = -dx;   // |x1 - x0|
    s32 dy = y1-y0; if (dy < 0) dy = -dy;   // |y1 - y0|
    s32 sx = (x0 < x1) ? 1 : -1;            // direccion horizontal
    s32 sy = (y0 < y1) ? 1 : -1;            // direccion vertical
    s32 err = dx - dy;                        // error inicial

    while (1) {
        if (esLienzo(x0, y0))                // clipping: solo pinta en canvas
            set_pixel(x0, y0, current_color);
        if (x0 == x1 && y0 == y1) break;     // llegamos al destino

        s32 e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }  // paso horizontal
        if (e2 <  dx) { err += dx; y0 += sy; }  // paso vertical
    }
}
```

**Clipping en Bresenham:** Cada pixel generado pasa por dos filtros:
1. `esLienzo(x0, y0)` — descarta pixeles en la zona de UI
2. `set_pixel` — descarta pixeles fuera del framebuffer

Esto significa que si una linea cruza del canvas hacia la paleta, los pixeles que caerian sobre la UI simplemente se ignoran. El algoritmo sigue calculando la linea completa, pero solo se pintan los pixeles validos.

---

## Funciones de UI

### `dibujaPaleta()`

**Que hace:** Dibuja la grilla de 8x8 colores (64 en total) en el panel izquierdo.

**Por que existe:** La paleta es la interfaz para que el usuario seleccione colores. Se dibuja una sola vez al inicio del programa porque los colores nunca cambian.

**Como funciona:**
1. Itera por filas (`row`: 0-7) y columnas (`col`: 0-7).
2. Para cada celda, calcula su posicion:
   - X: `col * CELL_W + 1` (el `+1` deja 1px de margen izquierdo)
   - Y: `PAL_START_Y + row * CELL_H + 1` (el `+1` deja 1px de margen superior)
3. Dibuja la celda con ancho `CELL_W - 1` y alto `CELL_H - 1`.

**Tecnica del borde de 1px:** Cada celda se dibuja 1 pixel mas pequena que su espacio asignado y desplazada 1 pixel. El fondo oscuro que queda visible entre las celdas actua como borde/separador natural, sin necesidad de dibujar lineas de borde explicitamente.

```
Sin inset (celdas se tocan):     Con inset de 1px:
+--------+--------+             +--------+--------+
|AAAAAAAA|BBBBBBBB|             | AAAAAAA| BBBBBBB|
|AAAAAAAA|BBBBBBBB|             | AAAAAAA| BBBBBBB|
+--------+--------+             +--------+--------+
|CCCCCCCC|DDDDDDDD|             | CCCCCCC| DDDDDDD|
+--------+--------+             +--------+--------+
                                 ^ fondo visible = borde
```

### `dibujaCajas()`

**Que hace:** Dibuja dos rectangulos de color en la esquina superior izquierda: el color actual (izquierda) y el color anterior (derecha).

**Por que existe:** Permite al usuario ver que color tiene seleccionado y cual tenia antes. Al hacer click en esta zona, los colores se intercambian (util para alternar rapidamente entre dos colores).

**Como funciona:**
- Caja izquierda: `fillArea(current_color, 1, 1, BOX_W-2, BOX_H-2)`
- Caja derecha: `fillArea(prev_Color, BOX_W+1, 1, BOX_W-2, BOX_H-2)`

Los margenes de `+1` y `-2` dejan un borde de 1px alrededor de cada caja (mismo principio que la paleta).

Se llama cada vez que el usuario cambia de color, para actualizar visualmente las cajas.

### `dibujaSeparador()`

**Que hace:** Dibuja una linea vertical de 1 pixel de ancho en `x = PALETA_W` (x=80), de arriba a abajo.

**Por que existe:** Separa visualmente el panel de la paleta del area de dibujo. Sin el separador, el borde entre ambas zonas seria invisible cuando el color del lienzo coincide con el fondo de la paleta.

---

## Guardar imagen: formato PPM

### `GuardarDibujo(nombre)`

**Que hace:** Exporta el contenido completo del framebuffer como un archivo de imagen en formato PPM.

**Por que existe:** Permite al usuario guardar su dibujo. Se eligio PPM porque es el formato de imagen mas simple que existe: no requiere compresion, no necesita librerias externas, y se puede implementar en ~15 lineas de codigo.

**Como funciona:**

1. **Abre el archivo** en modo escritura binaria (`"wb"`).
2. **Escribe el header PPM** como texto:
   ```
   P6\n
   320 240\n
   255\n
   ```
   - `P6` indica formato PPM binario (vs `P3` que seria texto)
   - `320 240` son ancho y alto en pixeles
   - `255` es el valor maximo por canal (8 bits)

3. **Escribe los pixeles** uno por uno, convirtiendo de ARGB a RGB:
   ```c
   u8 r = (px >> 16) & 0xFF;  // extrae bits 23-16 (rojo)
   u8 g = (px >> 8)  & 0xFF;  // extrae bits 15-8  (verde)
   u8 b =  px        & 0xFF;  // extrae bits 7-0   (azul)
   ```
   El canal alpha (bits 31-24) se descarta porque PPM no soporta transparencia.

4. **Cierra el archivo** e imprime confirmacion en consola.

### Conversion ARGB a RGB (detalle)

El framebuffer almacena cada pixel como un entero de 32 bits en formato ARGB:

```
Bit:  31..24  23..16  15..8   7..0
       Alpha   Rojo   Verde   Azul

Ejemplo: 0xFFEED6DC
  Alpha = 0xFF (255) → opaco
  Rojo  = 0xEE (238)
  Verde = 0xD6 (214)
  Azul  = 0xDC (220)
```

Para extraer cada canal se usan operaciones de bits:
- `>> 16` desplaza el valor 16 bits a la derecha, poniendo el canal rojo en los bits mas bajos
- `& 0xFF` mascara todo excepto los 8 bits mas bajos, aislando el canal

### Como abrir archivos PPM

- **Windows:** IrfanView, GIMP, o Paint.NET (con plugin)
- **Linux:** `eog`, `feh`, `gimp`, o cualquier visor de imagenes
- **Conversion:** `ffmpeg -i dibujo.ppm dibujo.png` para convertir a PNG

---

## Loop principal (`main`)

### Inicializacion

```c
OSW_Init("MiniPaint", FB_W, FB_H, 0);  // Crea ventana 320x240
```

Luego se pintan las areas iniciales:
1. **Lienzo** (canvas): rectangulo gris oscuro `#1C1C1C` desde `x=81` hasta `x=319`
2. **Area sobrante** debajo de la paleta: color `#1C1317` para diferenciarlo del lienzo
3. **Paleta**, **cajas** y **separador**: se dibujan con sus funciones dedicadas

Se activa el polling de mouse y teclado con `OSW_MouseSetPolling(1)` y `OSW_KeyboardSetPolling(1)`.

### Variables de estado del mouse

```c
u32 btn_prev = 0;              // Estado del boton en el frame anterior
s32 prev_x = -1, prev_y = -1;  // Posicion anterior del cursor (-1 = sin posicion)
```

- `btn_prev` se compara con `btn` para detectar el **instante** del click (no el estado sostenido). Sin esto, seleccionar un color de la paleta se ejecutaria 60 veces por segundo mientras el boton este presionado.
- `prev_x`/`prev_y` almacenan donde estaba el cursor en el frame anterior. Si es `-1`, significa que el usuario acaba de empezar a dibujar (primer punto del trazo), y se pinta un solo pixel en vez de trazar una linea.

### Deteccion de teclado

```c
while(OSW_KeyboardGetEvent(&kev)){
    if(kev.type == OSW_KEYEV_TYPE_PRESSED && kev.keycode == KEYCODE_S)
        GuardarDibujo("dibujo.ppm");
}
```

Se procesan **todos** los eventos de teclado acumulados. Se usa un `while` porque pueden haber multiples eventos por frame (tecla presionada y soltada en el mismo frame, por ejemplo).

**Scan code vs Virtual key:** libosw usa scan codes PS/2, no codigos virtuales de Windows. La tecla 'S' tiene scan code `0x1F`, no `0x53` (que seria el virtual key de Windows). Esto se descubrio experimentalmente con un `printf` de depuracion.

### Interaccion con mouse: seleccion de color

```c
u32 btn = mouse.btn & OSW_MOUSE_BTN0;  // aisla el boton izquierdo
if(btn && !btn_prev){                    // flanco de subida: click unico
```

Cuando se detecta un click (no sostenido) en el panel izquierdo (`mx < PALETA_W`):

**Click en las cajas** (`my < BOX_H`):
- Intercambia `current_color` y `prev_Color` usando una variable temporal
- Redibuja las cajas para reflejar el cambio

**Click en la paleta** (`my >= PAL_START_Y && my < PAL_START_Y + PAL_GRID_H`):
- Calcula que celda fue clickeada:
  - `col = mx / CELL_W` → columna (0-7)
  - `row = (my - PAL_START_Y) / CELL_H` → fila (0-7)
- El color actual se mueve a `prev_Color`, y el color de la celda se vuelve `current_color`
- Redibuja las cajas

### Interaccion con mouse: dibujo

```c
if(btn){  // boton sostenido (no solo click)
    if(esLienzo(mx, my)){
        if (prev_x >= 0)
            dibujaLinea(prev_x, prev_y, mx, my);  // linea continua
        else
            set_pixel(mx, my, current_color);      // primer punto del trazo
        prev_x = mx;
        prev_y = my;
    }else{
        prev_x = -1; prev_y = -1;  // salio del canvas → resetear
    }
}else{
    prev_x = -1; prev_y = -1;      // solto el boton → resetear
}
```

Cuando el usuario suelta el boton o mueve el cursor fuera del canvas, `prev_x` se resetea a `-1`. Asi, la proxima vez que empiece a dibujar, no se traza una linea desde el ultimo punto del trazo anterior.

### Render

```c
OSW_VideoDrawBuffer(framebuffer, FB_W, FB_H);  // sube framebuffer a GPU
OSW_VideoSwapBuffers();                         // presenta en pantalla
```

Esto sucede cada frame. `DrawBuffer` copia el arreglo de memoria a una textura de OpenGL, y `SwapBuffers` intercambia el buffer frontal y trasero para mostrar la imagen sin parpadeo (doble buffer).

---

## Sistema de clipping (resumen)

El clipping es lo que previene que el dibujo se salga de donde debe estar. MiniPaint implementa clipping en dos niveles:

| Nivel | Funcion | Que protege |
|---|---|---|
| 1 | `esLienzo(x, y)` | Impide dibujar sobre la paleta, cajas, separador y area sobrante. Solo permite pixeles donde `x > PALETA_W`. |
| 2 | `set_pixel(x, y, color)` | Impide escribir fuera del framebuffer. Descarta pixeles con `x < 0`, `y < 0`, `x >= 320`, o `y >= 240`. |

Ambos filtros se aplican a cada pixel generado por `dibujaLinea`. El algoritmo de Bresenham calcula la linea completa matematicamente, pero los pixeles invalidos se descartan silenciosamente.

Esto es mas simple (y suficiente para este programa) que el clipping por recorte (Cohen-Sutherland), que modificaria los extremos de la linea para que solo se calcule la porcion visible.

---

## Paleta de colores

64 colores organizados en 8 filas tematicas:

| Fila | Tema | Colores |
|---|---|---|
| 0 | Piel / cuerpo | Tonos beige claro a marron oscuro |
| 1 | Rojos / naranjas | Rojo puro, granate, naranja, coral |
| 2 | Amarillos / dorados | Amarillo, dorado, khaki, crema |
| 3 | Verdes | Verde puro, bosque, oliva, esmeralda |
| 4 | Azules | Celeste, azul rey, marino, cian |
| 5 | Morados / rosas | Magenta, purpura, lila, rosa |
| 6 | Cafes / tierra | Siena, chocolate, arena, tan |
| 7 | Grises + utilidad | Negro, grises, blanco, rosa fuerte |
