/*
 * tdm.h: Lector de modelos binarios .tdm
 * v1.0 - public domain
 *
 * Formato .tdm (little-endian):
 *   Cabecera (16 B): char magic[4]="TDMD", u32 version, u32 n_verts, u32 n_index
 *   Por vértice (36 B): f32 pos[3], f32 norm[3], f32 tex[2], u8 color[4] (RGBA)
 *   Índices: n_index * u32  (triangle list, múltiplo de 3)
 *
 * Genérico: carga cualquier .tdm válido (p.ej. mallas exportadas de Blender).
 */

#ifndef __PLANTILLA_TDM_H__
#define __PLANTILLA_TDM_H__

#include <gs.h>   /* Vert, u32 */
#include <trx.h>  /* vec3      */

typedef struct TdmModel_t {
    u32   n_verts;
    u32   n_index;
    Vert *verts;     /* pos + color, listo para gs_DrawElems        */
    u32  *index;     /* índices de triángulos                        */
    vec3 *normal;    /* normales parseadas (uso futuro: iluminación) */
    vec3 *texcoord;  /* (s, t, 0) parseado (uso futuro: texturas)    */
} TdmModel;

/* Carga 'path' en 'm'. Retorna 0 en éxito, !=0 en error.
   Normaliza la geometría (centrada en el origen, dimensión mayor ~1.0)
   para que se trate igual que el cubo unitario del pipeline. */
int  tdm_Load(const char *path, TdmModel *m);

/* Libera los buffers de 'm' y deja la struct a cero. */
void tdm_Free(TdmModel *m);

#endif /*__PLANTILLA_TDM_H__*/
