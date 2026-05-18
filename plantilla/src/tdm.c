/*
 * tdm.c: Lector de modelos binarios .tdm  (ver tdm.h para el formato)
 */

#include <tdm.h>
#include <stdio.h>
#include <stdlib.h>

#define TDM_VERT_BYTES   36u  /* 3+3+2 floats + 4 bytes color */
#define TDM_HEADER_BYTES 16u

/* Libera todo lo asignado y pone la struct a cero. */
void tdm_Free(TdmModel *m) {
    if (!m) return;
    free(m->verts);
    free(m->index);
    free(m->normal);
    free(m->texcoord);
    m->verts = NULL;  m->index = NULL;
    m->normal = NULL; m->texcoord = NULL;
    m->n_verts = 0;   m->n_index = 0;
}

int tdm_Load(const char *path, TdmModel *m) {
    if (!m || !path) return 1;
    m->verts = NULL;  m->index = NULL;
    m->normal = NULL; m->texcoord = NULL;
    m->n_verts = 0;   m->n_index = 0;

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[tdm] no se pudo abrir '%s'\n", path);
        return 2;
    }

    /* --- Cabecera --- */
    char magic[4];
    u32  version, n_verts, n_index;
    if (fread(magic,    1, 4, f) != 4 ||
        fread(&version, 4, 1, f) != 1 ||
        fread(&n_verts, 4, 1, f) != 1 ||
        fread(&n_index, 4, 1, f) != 1) {
        fprintf(stderr, "[tdm] cabecera incompleta en '%s'\n", path);
        fclose(f);
        return 3;
    }
    if (magic[0] != 'T' || magic[1] != 'D' ||
        magic[2] != 'M' || magic[3] != 'D') {
        fprintf(stderr, "[tdm] magic invalido (se esperaba 'TDMD') en '%s'\n",
                path);
        fclose(f);
        return 4;
    }
    if (n_verts == 0 || n_index == 0 || (n_index % 3u) != 0u) {
        fprintf(stderr, "[tdm] conteos invalidos (v=%u i=%u) en '%s'\n",
                n_verts, n_index, path);
        fclose(f);
        return 5;
    }

    /* Invariante de tamaño para este formato */
    if (fseek(f, 0, SEEK_END) == 0) {
        long sz = ftell(f);
        long expected = (long)TDM_HEADER_BYTES +
                        (long)n_verts * (long)TDM_VERT_BYTES +
                        (long)n_index * 4L;
        if (sz != expected) {
            fprintf(stderr,
                    "[tdm] tamaño %ld != esperado %ld en '%s'\n",
                    sz, expected, path);
            fclose(f);
            return 6;
        }
        fseek(f, (long)TDM_HEADER_BYTES, SEEK_SET);
    }

    (void)version;

    /* --- Reservar buffers --- */
    m->verts    = (Vert *)malloc(sizeof(Vert) * n_verts);
    m->normal   = (vec3 *)malloc(sizeof(vec3) * n_verts);
    m->texcoord = (vec3 *)malloc(sizeof(vec3) * n_verts);
    m->index    = (u32  *)malloc(sizeof(u32)  * n_index);
    if (!m->verts || !m->normal || !m->texcoord || !m->index) {
        fprintf(stderr, "[tdm] sin memoria para '%s'\n", path);
        fclose(f);
        tdm_Free(m);
        return 7;
    }

    /* --- Vértices: pos(3f) norm(3f) tex(2f) color(4 bytes) --- */
    float bbmin[3] = { 1e30f,  1e30f,  1e30f};
    float bbmax[3] = {-1e30f, -1e30f, -1e30f};
    for (u32 i = 0; i < n_verts; i++) {
        float pos[3], nrm[3], tex[2];
        unsigned char col[4];
        if (fread(pos, 4, 3, f) != 3 ||
            fread(nrm, 4, 3, f) != 3 ||
            fread(tex, 4, 2, f) != 2 ||
            fread(col, 1, 4, f) != 4) {
            fprintf(stderr, "[tdm] vértice %u truncado en '%s'\n", i, path);
            fclose(f);
            tdm_Free(m);
            return 8;
        }
        m->verts[i].pos.x = pos[0];
        m->verts[i].pos.y = pos[1];
        m->verts[i].pos.z = pos[2];
        /* color RGBA del archivo → vec3 0..1 (alpha ignorado por el pipeline) */
        m->verts[i].color.x = (float)col[0] / 255.0f;
        m->verts[i].color.y = (float)col[1] / 255.0f;
        m->verts[i].color.z = (float)col[2] / 255.0f;
        m->normal[i].x = nrm[0];
        m->normal[i].y = nrm[1];
        m->normal[i].z = nrm[2];
        m->texcoord[i].x = tex[0];
        m->texcoord[i].y = tex[1];
        m->texcoord[i].z = 0.0f;

        for (int k = 0; k < 3; k++) {
            float v = pos[k];
            if (v < bbmin[k]) bbmin[k] = v;
            if (v > bbmax[k]) bbmax[k] = v;
        }
    }

    /* --- Índices --- */
    if (fread(m->index, 4, n_index, f) != n_index) {
        fprintf(stderr, "[tdm] índices truncados en '%s'\n", path);
        fclose(f);
        tdm_Free(m);
        return 9;
    }
    fclose(f);

    /* --- Normalizar: centrar en origen y escalar a dimensión mayor ~1.0
           (mismo tamaño que el cubo unitario del pipeline) --- */
    float ctr[3], ext = 0.0f;
    for (int k = 0; k < 3; k++) {
        ctr[k] = 0.5f * (bbmin[k] + bbmax[k]);
        float d = bbmax[k] - bbmin[k];
        if (d > ext) ext = d;
    }
    float scale = (ext > 1e-6f) ? (1.0f / ext) : 1.0f;
    for (u32 i = 0; i < n_verts; i++) {
        m->verts[i].pos.x = (m->verts[i].pos.x - ctr[0]) * scale;
        m->verts[i].pos.y = (m->verts[i].pos.y - ctr[1]) * scale;
        m->verts[i].pos.z = (m->verts[i].pos.z - ctr[2]) * scale;
    }

    m->n_verts = n_verts;
    m->n_index = n_index;
    printf("[tdm] '%s' cargado: %u vertices, %u indices (%u triangulos)\n",
           path, n_verts, n_index, n_index / 3u);
    return 0;
}
