/*
 * trx.h: Matrix and vector functions
 * v1.0 - public domain
 */

#ifndef __PLANTILLA_TRX_H__
#define __PLANTILLA_TRX_H__

#include <libosw/types.h>

typedef struct Vec3_t{
    float x;
    float y;
    float z;
} vec3;

typedef struct Vec4_t{
    float x;
    float y;
    float z;
    float w;
} vec4;

// Columnas se recorren primero
typedef float mat3[3][3];  //[y][x]
typedef float mat4[4][4];

/* Vector related functions */
vec3  vec3_add(vec3 dest, const vec3 v, const vec3 u);
vec3  vec3_sub(vec3 dest, const vec3 v, const vec3 u);
vec3  vec3_mul(vec3 dest, const vec3 v, const vec3 u);
vec3  vec3_smul(vec3 dest, float k, const vec3 v);
int   vec3_eq(const vec3 u, const vec3 v);

vec3  vec3_clamp(vec3 v, float a, float b);

vec3  vec3_lerp(vec3 dest, const vec3 v, const vec3 u, f32 a);
float vec3_dot(const vec3 v, const vec3 u);
vec3  vec3_cross(vec3 dest, const vec3 v, const vec3 u);
vec3  vec3_reflect(vec3 dest, const vec3 m, const vec3 p);
vec3  vec3_normalize(vec3 v);
// matriz 3x3
vec3  vec3_matMul(vec3 dest, const mat3 m, vec3 p);
//  4 x 4 con w = 0
vec3  vec3_mat3Mul(vec3 dest, const mat4 m, vec3 p);
// 4x4 con w = 1
float vec3_mat4Mul(vec3* dest, const mat4 m, vec3 p);
vec3  vec3_homogenize(vec3 v, float w);

vec4  vec4_normalize(vec4 v);
vec4  vec4_mat4Mul(vec4 dest, const mat4 m, vec4 p);
vec4  vec4_homogenize(vec4 v);

/* Matrix related functions */
void mat4_identity(mat4 m);
void mat4_mul(mat4 dest, const mat4 m1, const mat4 m2);
void mat4_normalMat(mat3 dest, const mat4 m);

void mat4_translate(mat4 m, vec3 v);
void mat4_rotate(mat4 m, vec3 v, float angle);
void mat4_scale(mat4 m, vec3 v);

void mat4_ortho(mat4 m, float left, float right, float bottom, float top, float near, float far);
void mat4_perspective(mat4 m, float fovy, float aspect, float znear, float zfar);

/* Construye una View matrix tipo look-at.
   eye: posición de la cámara, center: punto objetivo, up: vector arriba ({0,1,0}).
   CONVENCIÓN de mat4_scale / mat4_translate: aplicar siempre en orden
   identity → scale → rotate → translate. No componen correctamente en otro orden. */
void mat4_lookAt(mat4 m, vec3 eye, vec3 center, vec3 up);

/* Mezcla dos colores ARGB empaquetados (0xAARRGGBB).
   a=1.0 → src puro,  a=0.0 → dst puro.              */
u32 color_lerp(u32 src, u32 dst, float a);

#endif /*__PLANTILLA_TRX_H__*/
