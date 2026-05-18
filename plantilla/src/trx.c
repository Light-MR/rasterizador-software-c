#include <trx.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* *
 * Adds two 3D vectors.
 * dest: The destination vector where the result will be stored.
 * v: The first input vector.
 * u: The second input vector.
 * Returns: The resulting vector after addition.
 * */
vec3  vec3_add(vec3 dest, const vec3 v, const vec3 u){
    dest.x = v.x + u.x;
    dest.y = v.y + u.y;
    dest.z = v.z + u.z;
    return dest;
}

/* *
    * Subtracts one 3D vector from another.
    * dest: The destination vector where the result will be stored.
    * v: The first input vector (minuend).
    * u: The second input vector (subtrahend).
    * Returns: The resulting vector after subtraction.
    *
* */
vec3  vec3_sub(vec3 dest, const vec3 v, const vec3 u){
    dest.x = v.x - u.x;
    dest.y = v.y - u.y;
    dest.z = v.z - u.z;
    return dest;
}
/**
 * Multiplies two 3D vectors component-wise.
 * dest: The destination vector where the result will be stored.
 * v: The first input vector.
 * u: The second input vector.
 * Returns: The resulting vector after component-wise multiplication.
 */
vec3  vec3_mul(vec3 dest, const vec3 v, const vec3 u)
{
    dest.x = v.x * u.x;
    dest.y = v.y * u.y;
    dest.z = v.z * u.z;
    return dest;
}

/**
 * Multiplies a 3D vector by a scalar.
 * dest: The destination vector where the result will be stored.
 * k: The scalar value to multiply the vector by.
 * v: The input vector to be scaled.
 * Returns: The resulting vector after scaling.
 */
vec3  vec3_smul(vec3 dest, float k, const vec3 v){
    dest.x = v.x * k;
    dest.y = v.y * k;
    dest.z = v.z * k;
    return dest;
}

/**
 * Checks if two 3D vectors are equal.
 * u: The first input vector.
 * v: The second input vector.
 * Returns: 1 if the vectors are equal, 0 otherwise.
 */
int   vec3_eq(const vec3 u, const vec3 v){
    return u.x == v.x && u.y == v.y && u.z == v.z;
}

/**
 * Clamps the components of a 3D vector to a specified range.
 * v: The input vector to be clamped.
 * a: The minimum value for clamping.
 * b: The maximum value for clamping.
 * Returns: The resulting vector after clamping.
 */
vec3  vec3_clamp(vec3 v, float a, float b){
    if(v.x < a) v.x = a;
    else if(v.x > b) v.x = b;

    if(v.y < a) v.y = a;
    else if(v.y > b) v.y = b;

    if(v.z < a) v.z = a;
    else if(v.z > b) v.z = b;

    return v;
}

/**
 * Performs linear interpolation between two 3D vectors.
 * dest: The destination vector where the result will be stored.
 * v: The first input vector (start).
 * u: The second input vector (end).
 * a: The interpolation factor (0.0 to 1.0).
 * Returns: The resulting vector after linear interpolation.
 */
vec3  vec3_lerp(vec3 dest, const vec3 v, const vec3 u, f32 a){
    dest.x = v.x + a * (u.x - v.x);
    dest.y = v.y + a * (u.y - v.y);
    dest.z = v.z + a * (u.z - v.z);
    return dest;

}

/**
 * Computes the dot product of two 3D vectors.
 * v: The first input vector.
 * u: The second input vector.
 * Returns: The dot product of the two vectors.
 */
float vec3_dot(const vec3 v, const vec3 u){
    return v.x * u.x + v.y * u.y + v.z * u.z;
}

/**
 * Computes the cross product of two 3D vectors.
 * dest: The destination vector where the result will be stored.
 * v: The first input vector.
 * u: The second input vector.
 * Returns: The resulting vector after computing the cross product.
 */
vec3  vec3_cross(vec3 dest, const vec3 v, const vec3 u){
    dest.x = v.y * u.z - v.z * u.y;
    dest.y = v.z * u.x - v.x * u.z;
    dest.z = v.x * u.y - v.y * u.x;
    return dest;
}

/**
 * Reflects a vector across a plane defined by a normal vector.
 * dest: The destination vector where the result will be stored.
 * m: The normal vector of the plane (should be normalized).
 * p: The input vector to be reflected.
 * Returns: The resulting vector after reflection.
 */
vec3  vec3_reflect(vec3 dest, const vec3 m, const vec3 p){
    float dot = vec3_dot(m, p);
    dest.x = m.x * 2.0f * dot - p.x;
    dest.y = m.y * 2.0f * dot - p.y;
    dest.z = m.z * 2.0f * dot - p.z;
    return dest;
}

/**
 * Normalizes a 3D vector to have a length of 1.
 * v: The input vector to be normalized.
 * Returns: The resulting normalized vector.
 * Note: If the input vector has a length of 0, it will remain unchanged.
 */
vec3  vec3_normalize(vec3 v){
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        v.x *= invLen;
        v.y *= invLen;
        v.z *= invLen;
    }
    return v;
}


// matriz 3x3
/**
 * Multiplies a 3D vector by a 3x3 matrix.
 * dest: The destination vector where the result will be stored.
 * m: The 3x3 matrix to multiply with.
 * p: The input vector to be transformed.
 * Returns: The resulting vector after multiplication.
 * Note: The matrix is expected to be in column-major order.
 *       The input vector is treated as a column vector.
 *       The resulting vector is computed as dest = m * p.
 *       The w component is assumed to be 1 for homogeneous coordinates.
 *       The resulting vector is not normalized.
 *       This function is typically used for transforming points in 3D space.
 *       For transforming directions, use vec3_mat3Mul instead.
 */
vec3  vec3_matMul(vec3 dest, const mat3 m, vec3 p){
    dest.x = m[0][0] * p.x + m[1][0] * p.y + m[2][0] * p.z;
    dest.y = m[0][1] * p.x + m[1][1] * p.y + m[2][1] * p.z;
    dest.z = m[0][2] * p.x + m[1][2] * p.y + m[2][2] * p.z;
    return dest;
}
//  4 x 4 con w = 0
/**
 * Multiplies a 3D vector by a 4x4 matrix, treating the vector as a direction (w = 0).
 * dest: The destination vector where the result will be stored.
 * m: The 4x4 matrix to multiply with.
 * p: The input vector to be transformed.
 * Returns: The resulting vector after multiplication.
 * Note: The matrix is expected to be in column-major order.
 *       The input vector is treated as a column vector with w = 0.
 *       The resulting vector is computed as dest = m * p.
 *       The w component is ignored in the transformation.
 *       This function is typically used for transforming directions in 3D space.
 *       For transforming points, use vec3_mat4Mul instead.
 */
vec3  vec3_mat3Mul(vec3 dest, const mat4 m, vec3 p){
    dest.x = m[0][0] * p.x + m[1][0] * p.y + m[2][0] * p.z;
    dest.y = m[0][1] * p.x + m[1][1] * p.y + m[2][1] * p.z;
    dest.z = m[0][2] * p.x + m[1][2] * p.y + m[2][2] * p.z;
    return dest;
}
// 4x4 con w = 1
/**
 * Multiplies a 3D vector by a 4x4 matrix, treating the vector as a point (w = 1).
 * dest: The destination vector where the result will be stored.
 * m: The 4x4 matrix to multiply with.
 * p: The input vector to be transformed.
 * Returns: The resulting vector after multiplication.
 * Note: The matrix is expected to be in column-major order.
 *       The input vector is treated as a column vector with w = 1.
 *       The resulting vector is computed as dest = m * p.
 *       The w component is used in the transformation and is returned as a float.
 *       This function is typically used for transforming points in 3D space.
 *       For transforming directions, use vec3_mat3Mul instead.
 *       The resulting vector is not normalized.
 *       The caller is responsible for homogenizing the resulting vector if needed.
 */
float vec3_mat4Mul(vec3* dest, const mat4 m, vec3 p){
    float w = m[0][3] * p.x + m[1][3] * p.y + m[2][3] * p.z + m[3][3];
    dest->x = m[0][0] * p.x + m[1][0] * p.y + m[2][0] * p.z + m[3][0];
    dest->y = m[0][1] * p.x + m[1][1] * p.y + m[2][1] * p.z + m[3][1];
    dest->z = m[0][2] * p.x + m[1][2] * p.y + m[2][2] * p.z + m[3][2];
    return w;
}
/**
 * Converts a 3D vector to homogeneous coordinates by adding a w component.
 * v: The input vector to be homogenized.
 * w: The w component to be added to the vector (typically 1.0 for points).
 * Returns: The resulting vector in homogeneous coordinates.
 * Note: The resulting vector will have the same x, y, z components as the input vector, and the w component will be set to the specified value.
 *       This function is typically used to convert a 3D point to homogeneous coordinates for transformation with a 4x4 matrix.
 *       The caller is responsible for normalizing the resulting vector if needed after transformation.
 *       For directions, use w = 0.0 when homogenizing.
 *       The resulting vector is not normalized.
 *       The caller is responsible for homogenizing the resulting vector if needed after transformation.    
 */
vec3  vec3_homogenize(vec3 v, float w){
    if (w != 0.0f && w != 1.0f) {
        float invW = 1.0f / w;
        v.x *= invW;
        v.y *= invW;
        v.z *= invW;
    }
    return v;
}

/**
 * Normalizes a 4D vector to have a length of 1.
 * v: The input vector to be normalized.
 * Returns: The resulting normalized vector.
 * Note: If the input vector has a length of 0, it will remain unchanged.
 *       The w component is included in the normalization process.
 *       This function is typically used for normalizing homogeneous coordinates or other 4D vectors in 3D graphics applications.
 *       The resulting vector will have the same direction as the input vector but with a length of 1.
 *       The caller is responsible for homogenizing the resulting vector if needed after normalization.
 */
vec4  vec4_normalize(vec4 v){
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        v.x *= invLen;
        v.y *= invLen;
        v.z *= invLen;
        v.w *= invLen;
    }
    return v;
}
/**
 * Multiplies a 4D vector by a 4x4 matrix.
 * dest: The destination vector where the result will be stored.
 * m: The 4x4 matrix to multiply with.
 * p: The input vector to be transformed.
 * Returns: The resulting vector after multiplication.
 * Note: The matrix is expected to be in column-major order.
 *       The input vector is treated as a column vector.
 *       The resulting vector is computed as dest = m * p.
 *       The resulting vector is not normalized.
 *       This function is typically used for transforming points or directions in 3D space using homogeneous coordinates.
 *       The caller is responsible for homogenizing the resulting vector if needed after transformation.
 *       The w component is included in the transformation and is returned as part of the resulting vector
 */
vec4  vec4_mat4Mul(vec4 dest, const mat4 m, vec4 p){
    dest.x = m[0][0] * p.x + m[1][0] * p.y + m[2][0] * p.z + m[3][0] * p.w;
    dest.y = m[0][1] * p.x + m[1][1] * p.y + m[2][1] * p.z + m[3][1] * p.w;
    dest.z = m[0][2] * p.x + m[1][2] * p.y + m[2][2] * p.z + m[3][2] * p.w;
    dest.w = m[0][3] * p.x + m[1][3] * p.y + m[2][3] * p.z + m[3][3] * p.w;
    return dest;
}
/**
 * Converts a 4D vector from homogeneous coordinates to 3D coordinates by dividing by the w component.
 * v: The input vector in homogeneous coordinates to be homogenized.
 * Returns: The resulting vector in 3D coordinates.
 * Note: If the w component of the input vector is 0, the function will return the original vector without modification to avoid division by zero.
 *       This function is typically used to convert a homogeneous coordinate back to 3D space after transformation with a 4x4 matrix.
 *       The resulting vector will have its x, y, z components divided by the w component of the input vector, and the w component will be set to 1.0 for points.
 *       For directions, the w component is typically set to 0.0 and remains unchanged after homogenization.
 *       The caller is responsible for normalizing the resulting vector if needed after homogenization.    
 */
vec4  vec4_homogenize(vec4 v){
    if (v.w != 0.0f) {
        float invW = 1.0f / v.w;
        v.x *= invW;
        v.y *= invW;
        v.z *= invW;
        v.w = 1.0f; // After homogenization, w is typically set to 1 for points.
    }
    return v;   
}

/* Matrix related functions */
/**
 * Sets a 4x4 matrix to the identity matrix.
 * m: The matrix to be set to the identity matrix.
 * Note: The identity matrix is a special matrix that does not change a vector when multiplied by it. It has 1s on the diagonal and 0s elsewhere.
 *       After calling this function, the matrix m will be set to the identity matrix, which can be used as a starting point for building transformation matrices or for resetting transformations.
 *       The resulting matrix will have the following form:
 *       [1 0 0 0]
 *       [0 1 0 0
 *       [0 0 1 0]
 *       [0 0 0 1]
 *       This function is typically used in graphics applications to initialize transformation matrices before applying translations, rotations, or scaling transformations.
 *       The caller can then modify the matrix m by applying additional transformations as needed.
 *       The resulting matrix is not normalized, as the identity matrix is already in its standard form
 */
void mat4_identity(mat4 m){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

/**
 * Multiplies two 4x4 matrices and stores the result in a destination matrix.
 * dest: The destination matrix where the result will be stored.
 * m1: The first input matrix.
 * m2: The second input matrix.
 * Note: The matrices are expected to be in column-major order.
 *       The resulting matrix is computed as dest = m1 * m2.
 *       The resulting matrix is not normalized.
 *       This function is typically used in graphics applications to combine multiple transformations into a single matrix by multiplying them together.
 *       The caller can use the resulting matrix to transform vectors or points in 3D space by multiplying the resulting matrix with the vectors or points.
 *       The order of multiplication matters, as matrix multiplication is not commutative. The transformation represented by m2 will be applied first, followed by the transformation represented by m1.
 *       The resulting matrix will contain the combined effect of both transformations represented by m1 and m2.
 */
void mat4_mul(mat4 dest, const mat4 m1, const mat4 m2){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dest[i][j] = m1[0][j] * m2[i][0] + m1[1][j] * m2[i][1] + m1[2][j] * m2[i][2] + m1[3][j] * m2[i][3];
        }
    }

}

/**
 * Extracts the normal matrix from a 4x4 transformation matrix and stores it in a 3x3 matrix.
 * dest: The destination 3x3 matrix where the normal matrix will be stored.
 * m: The input 4x4 transformation matrix.
 * Note: The normal matrix is used to transform normal vectors correctly when applying transformations that include non-uniform scaling. It is typically the inverse transpose of the upper-left 3x3 portion of the 4x4 transformation matrix.
 *       This function assumes that the input matrix m is a valid transformation matrix and that the upper-left 3x3 portion of m contains the rotation and scaling components of the transformation.
 *       The resulting normal matrix will be stored in the dest matrix, which can then be used to transform normal vectors in 3D space. The caller is responsible for normalizing the resulting normal matrix if needed after extraction.
 *       The resulting normal matrix is not normalized, as it is typically used in conjunction with the original transformation matrix to ensure correct transformation of normal vectors.      
 */
void mat4_normalMat(mat3 dest, const mat4 m){
    // Normal matrix = transpose(inverse(upper-left 3x3))
    // Using cofactors to compute the general inverse (handles non-uniform scaling)
    float a00 = m[0][0], a01 = m[1][0], a02 = m[2][0];
    float a10 = m[0][1], a11 = m[1][1], a12 = m[2][1];
    float a20 = m[0][2], a21 = m[1][2], a22 = m[2][2];

    float c00 =  (a11 * a22 - a12 * a21);
    float c01 = -(a10 * a22 - a12 * a20);
    float c02 =  (a10 * a21 - a11 * a20);
    float c10 = -(a01 * a22 - a02 * a21);
    float c11 =  (a00 * a22 - a02 * a20);
    float c12 = -(a00 * a21 - a01 * a20);
    float c20 =  (a01 * a12 - a02 * a11);
    float c21 = -(a00 * a12 - a02 * a10);
    float c22 =  (a00 * a11 - a01 * a10);

    float det = a00 * c00 + a01 * c01 + a02 * c02;
    if (det == 0.0f) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                dest[i][j] = (i == j) ? 1.0f : 0.0f;
        return;
    }

    float invDet = 1.0f / det;
    // normal_mat[row][col] = C[row][col] / det  →  dest[col][row] = C[row][col] / det
    dest[0][0] = c00 * invDet;  dest[0][1] = c10 * invDet;  dest[0][2] = c20 * invDet;
    dest[1][0] = c01 * invDet;  dest[1][1] = c11 * invDet;  dest[1][2] = c21 * invDet;
    dest[2][0] = c02 * invDet;  dest[2][1] = c12 * invDet;  dest[2][2] = c22 * invDet;
}


/**
 * Applies a translation transformation to a 4x4 matrix using a 3D vector.
 * m: The input matrix to be translated. This matrix will be modified in place.
 * v: The translation vector that specifies the amount of translation along the x, y, and z axes.
 * Note: The translation is applied by modifying the last column of the matrix m. The resulting matrix will represent the original transformation combined with the specified translation. 
 * The caller can use the resulting matrix to transform points or vectors in 3D space by multiplying the resulting matrix with the points or vectors. The resulting matrix is not normalized, as it represents a combination of transformations including translation. 
 * This function is typically used in graphics applications to apply a translation transformation to an existing transformation matrix, allowing for the movement of objects in 3D space. The caller can apply additional transformations such as rotation or scaling to the resulting matrix as needed.
 */
void mat4_translate(mat4 m, vec3 v){
    m[3][0] += v.x; //  0,3 , 1,3 , 2,3
    m[3][1] += v.y;
    m[3][2] += v.z;
}

/**
 * Applies a rotation transformation to a 4x4 matrix using an axis-angle representation.
 * m: The input matrix to be rotated. This matrix will be modified in place.
 * v: The rotation axis vector that specifies the axis of rotation. This vector should be normalized for correct results.
 * angle: The angle of rotation in degrees. Positive angles represent counter-clockwise rotation when looking along the rotation axis towards the origin.
 * Note: The rotation is applied by creating a rotation matrix based on the specified axis and angle, and then multiplying the input matrix m by the rotation matrix. The resulting matrix will represent the original transformation combined with the specified rotation. 
 * The caller can use the resulting matrix to transform points or vectors in 3D space by multiplying the resulting matrix with the points or vectors. The resulting matrix is not normalized, as it represents a combination of transformations including rotation. 
 * This function is typically used in graphics applications to apply a rotation transformation to an existing transformation matrix, allowing for the rotation of objects in 3D space. The caller can apply additional transformations such as translation or scaling to the resulting matrix as needed.    
 */
void mat4_rotate(mat4 m, vec3 v, float angle){
    float rad = angle * (M_PI / 180.0f);
    float c = cosf(rad);
    float s = sinf(rad);
    mat4 rot;
    mat4_identity(rot);

    rot[0][0] = c + v.x * v.x * (1 - c);
    rot[0][1] = v.x * v.y * (1 - c) + v.z * s;
    rot[0][2] = v.x * v.z * (1 - c) - v.y * s;

    rot[1][0] = v.y * v.x * (1 - c) - v.z * s;
    rot[1][1] = c + v.y * v.y * (1 - c);
    rot[1][2] = v.y * v.z * (1 - c) + v.x * s;

    rot[2][0] = v.z * v.x * (1 - c) + v.y * s;
    rot[2][1] = v.z * v.y * (1 - c) - v.x * s;
    rot[2][2] = c + v.z * v.z * (1 - c);

    mat4 tmp;
    mat4_mul(tmp, m, rot);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[i][j] = tmp[i][j];
}


/**
 * Applies a scaling transformation to a 4x4 matrix using a 3D vector.
 * m: The input matrix to be scaled. This matrix will be modified in place.
 * v: The scaling vector that specifies the scaling factors along the x, y, and z axes. A value of 1.0 means no scaling, values greater than 1.0 will enlarge the object, and values between 0.0 and 1.0 will shrink the object.
 * Note: The scaling is applied by modifying the diagonal elements of the matrix m. The resulting matrix will represent the original transformation combined with the specified scaling. 
 * The caller can use the resulting matrix to transform points or vectors in 3D space by multiplying the resulting matrix with the points or vectors. The resulting matrix is not normalized, as it represents a combination of transformations including scaling. 
 * This function is typically used in graphics applications to apply a scaling transformation to an existing transformation matrix, allowing for the resizing of objects in 3D space. The caller can apply additional transformations such
 */
void mat4_scale(mat4 m, vec3 v){
    m[0][0] *= v.x;
    m[1][1] *= v.y;
    m[2][2] *= v.z;
}

/*
 * mat4_ortho — Proyección ortogonal (sin perspectiva)
 *
 * En modo ortogonal los objetos NO se ven más pequeños al alejarse.
 * Se usa para planos técnicos, vistas isométricas o interfaces 2D.
 *
 * El "volumen visible" es una caja rectangular (ortoedro) definida por
 * seis planos de recorte. Todo lo que cae dentro se proyecta
 * perpendicularmente sobre la pantalla — no hay punto de fuga.
 *
 * Parámetros:
 *   m      — matriz resultado (se sobreescribe)
 *   left   — límite izquierdo del volumen visible (en unidades de mundo)
 *   right  — límite derecho
 *   bottom — límite inferior
 *   top    — límite superior
 *   near   — distancia al plano cercano (lo más cerca que ve la cámara)
 *   far    — distancia al plano lejano  (lo más lejos que ve la cámara)
 *
 * Qué hace la matriz:
 *   Mapea la caja [left,right]×[bottom,top]×[near,far]
 *   al cubo NDC  [-1,+1]×[-1,+1]×[-1,+1].
 *
 *   m[0][0] = 2/(right-left)   ← escala X para que el ancho quepa en [-1,+1]
 *   m[1][1] = 2/(top-bottom)   ← escala Y
 *   m[2][2] = -2/(far-near)    ← escala Z (negativo: Z mundo → -Z NDC)
 *   m[3][0] = -(right+left)/(right-left)  ← desplaza al centro en X
 *   m[3][1] = -(top+bottom)/(top-bottom)  ← desplaza al centro en Y
 *   m[3][2] = -(far+near)/(far-near)      ← desplaza al centro en Z
 *
 * Uso en main.c:
 *   float h = cam_dist * 0.268f;          // tamaño del volumen
 *   mat4_ortho(P, -h*aspect, h*aspect, -h, h, 0.1f, 100.0f);
 *   El factor 0.268 = tan(15°) hace que a la misma distancia se vea
 *   igual que la perspectiva de 30° de campo visual.
 */
void mat4_ortho(mat4 m, float left, float right, float bottom, float top, float near, float far){
    mat4_identity(m);
    m[0][0] = 2.0f / (right - left);           /* escala X */
    m[1][1] = 2.0f / (top - bottom);           /* escala Y */
    m[2][2] = -2.0f / (far - near);            /* escala Z */
    m[3][0] = -(right + left) / (right - left);/* centra X */
    m[3][1] = -(top + bottom) / (top - bottom);/* centra Y */
    m[3][2] = -(far + near)   / (far - near);  /* centra Z */
}

/*
 * mat4_perspective — Proyección en perspectiva (objetos lejanos se ven más pequeños)
 *
 * Simula como ve el ojo humano o una cámara real: hay un punto de fuga,
 * las líneas paralelas convergen en la distancia.
 *
 * El "volumen visible" es un frustum (pirámide truncada). Todo lo que
 * cae dentro del frustum aparece en pantalla; lo demás se descarta.
 *
 *        cámara
 *          /\
 *         /  \   ← near plane (znear)
 *        /    \
 *       /      \
 *      /________\ ← far plane (zfar)
 *
 * Parámetros:
 *   m      — matriz resultado (se sobreescribe)
 *   fovy   — campo de visión vertical en GRADOS
 *              30° = vista telescópica (objetos grandes, poco ángulo)
 *              90° = vista amplia tipo FPS
 *              En main.c usamos 30°
 *   aspect — relación ancho/alto de la pantalla (FB_W / FB_H = 640/480 ≈ 1.33)
 *              Sin esto la imagen se deformaría (círculos se verían ovalados)
 *   znear  — distancia mínima visible (0.1 en main.c)
 *              Vértices más cerca de esto se descartan (w <= 0 en project())
 *   zfar   — distancia máxima visible (100.0 en main.c)
 *              Vértices más lejos de esto también se descartan
 *
 * Cómo se construye:
 *   f = 1/tan(fovy/2)  ← "factor de zoom": a mayor fovy, menor f, más se ve
 *
 *   m[0][0] = f/aspect ← escala X (corrige la deformación del aspecto)
 *   m[1][1] = f        ← escala Y (controla el campo visual vertical)
 *   m[2][2] = (zfar+znear)/(znear-zfar)  ← mapea Z al rango NDC [-1,+1]
 *   m[2][3] = -1       ← hace que w = -z_camara (así project() puede detectar
 *                         vértices detrás de la cámara cuando w <= 0)
 *   m[3][2] = (2*zfar*znear)/(znear-zfar) ← parte del mapeo de profundidad
 *   m[3][3] = 0        ← necesario para que w no sea 1 fijo
 *
 * Al multiplicar un vértice (x,y,z,1) por esta matriz y dividir entre w,
 * se obtiene la división por z que produce la perspectiva:
 *   x_NDC = x*f/aspect / z,   y_NDC = y*f / z
 *
 * Uso en main.c:
 *   mat4_perspective(P, 30.0f, (float)FB_W/FB_H, 0.1f, 100.0f);
 */
void mat4_perspective(mat4 m, float fovy, float aspect, float znear, float zfar){
    float rad = fovy * (M_PI / 180.0f);        /* grados → radianes          */
    float f   = 1.0f / tanf(rad / 2.0f);       /* cotangente del semi-ángulo  */
    mat4_identity(m);
    m[0][0] = f / aspect;                       /* escala X, corrige aspecto   */
    m[1][1] = f;                                /* escala Y (campo visual)     */
    m[2][2] = (zfar + znear) / (znear - zfar); /* mapeo de profundidad a NDC  */
    m[2][3] = -1.0f;                            /* w ← -z (activa perspectiva) */
    m[3][2] = (2.0f * zfar * znear) / (znear - zfar); /* offset de profundidad */
    m[3][3] = 0.0f;                             /* anula el w=1 de los vértices*/
}

/*
 * Construye una View matrix tipo "look-at" (sistema dextrógiro, cámara mira hacia -Z).
 * eye:    posición de la cámara en el mundo
 * center: punto al que apunta la cámara
 * up:     vector "arriba" de referencia, típicamente {0,1,0}
 *
 * Produce la matriz que transforma coordenadas del mundo al espacio cámara.
 * Equivalente a: V = R * T(-eye), donde R alinea los ejes de la cámara con los del mundo.
 */
void mat4_lookAt(mat4 m, vec3 eye, vec3 center, vec3 up){
    /* forward = normalize(center - eye) */
    vec3 f;
    f.x = center.x - eye.x;
    f.y = center.y - eye.y;
    f.z = center.z - eye.z;
    f = vec3_normalize(f);

    /* right = normalize(forward × up) */
    vec3 r = {0,0,0};
    r = vec3_cross(r, f, up);
    r = vec3_normalize(r);

    /* up recalculado = right × forward (ortogonal) */
    vec3 u = {0,0,0};
    u = vec3_cross(u, r, f);

    /* Matriz en columna-mayor: m[col][row]
     * Fila 0 → eje X de cámara (right)
     * Fila 1 → eje Y de cámara (up)
     * Fila 2 → eje Z de cámara (-forward, cámara mira hacia -Z)
     * Columna 3 → traslación: -dot(eje, eye)                      */
    mat4_identity(m);
    m[0][0] =  r.x;  m[1][0] =  r.y;  m[2][0] =  r.z;
    m[0][1] =  u.x;  m[1][1] =  u.y;  m[2][1] =  u.z;
    m[0][2] = -f.x;  m[1][2] = -f.y;  m[2][2] = -f.z;
    m[3][0] = -(r.x*eye.x + r.y*eye.y + r.z*eye.z);
    m[3][1] = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
    m[3][2] =  (f.x*eye.x + f.y*eye.y + f.z*eye.z);
}

/* Mezcla dos colores ARGB empaquetados (0xAARRGGBB).
   a=1.0 → src puro,  a=0.0 → dst puro.              */
u32 color_lerp(u32 src, u32 dst, float a) {
    float ia = 1.0f - a;
    u8 r = (u8)(a * ((src >> 16) & 0xFF) + ia * ((dst >> 16) & 0xFF));
    u8 g = (u8)(a * ((src >>  8) & 0xFF) + ia * ((dst >>  8) & 0xFF));
    u8 b = (u8)(a * ( src        & 0xFF) + ia * ( dst        & 0xFF));
    return 0xFF000000 | ((u32)r << 16) | ((u32)g << 8) | b;
}