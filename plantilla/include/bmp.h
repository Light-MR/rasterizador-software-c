/*
 * bmp.h: Cargador de texturas BMP (24 bpp, sin compresión)
 * v1.0 - public domain
 *
 * Lee un BMP de 24 bits (BI_RGB) y rellena un GsTexture: los 3 bytes por
 * píxel se dejan tal cual vienen en el archivo, bottom-up (fila 0 abajo);
 * el orden de canales lo interpreta el muestreador (tex_texel en gs.c).
 * Genérico: carga cualquier BMP 24bpp válido, no solo el de bob esponja.
 */

#ifndef __PLANTILLA_BMP_H__
#define __PLANTILLA_BMP_H__

#include <gs.h>   /* GsTexture */

/* Carga 'path' en 'out'. Retorna 0 en éxito, !=0 en error.
   No fija out->filter/out->wrap (los pone el llamador). */
int  bmp_Load(const char *path, GsTexture *out);

/* Libera out->data y deja la struct a cero. */
void bmp_Free(GsTexture *out);

#endif /*__PLANTILLA_BMP_H__*/
