/*
 * bmp.c: Cargador de texturas BMP de 24 bpp (BI_RGB, sin compresión).
 *        Deja los datos como en el archivo: BGR, fila 0 = abajo.
 */

#include <bmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Lee un u32 little-endian de un buffer de bytes. */
static u32 rd_u32(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

int bmp_Load(const char *path, GsTexture *out) {
    if (!out || !path) return 1;
    out->w = out->h = 0;
    out->data = NULL;

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[bmp] no se pudo abrir '%s'\n", path);
        return 2;
    }

    unsigned char hdr[54];   /* FILEHEADER(14) + INFOHEADER(40) */
    if (fread(hdr, 1, 54, f) != 54) {
        fprintf(stderr, "[bmp] cabecera incompleta en '%s'\n", path);
        fclose(f);
        return 3;
    }
    if (hdr[0] != 'B' || hdr[1] != 'M') {
        fprintf(stderr, "[bmp] '%s' no es BMP\n", path);
        fclose(f);
        return 4;
    }

    u32 data_off = rd_u32(hdr + 10);
    int width    = (int)rd_u32(hdr + 18);
    int height   = (int)rd_u32(hdr + 22);
    u32 bpp      = (u32)(hdr[28] | (hdr[29] << 8));
    u32 comp     = rd_u32(hdr + 30);

    if (bpp != 24 || comp != 0) {
        fprintf(stderr, "[bmp] solo 24bpp sin compresión (bpp=%u comp=%u) en '%s'\n",
                bpp, comp, path);
        fclose(f);
        return 5;
    }

    int topdown = (height < 0);          /* height<0 → filas de arriba a abajo */
    u32 W = (u32)(width  < 0 ? -width  : width);
    u32 H = (u32)(height < 0 ? -height : height);
    if (W == 0 || H == 0) { fclose(f); return 6; }

    u32 fstride = ((W * 3u) + 3u) & ~3u;  /* stride del archivo (alineado a 4) */
    u32 ostride = W * 3u;                  /* stride de salida (empaquetado)   */

    unsigned char *data   = (unsigned char *)malloc((size_t)ostride * H);
    unsigned char *rowbuf = (unsigned char *)malloc(fstride);
    if (!data || !rowbuf) {
        free(data); free(rowbuf); fclose(f);
        fprintf(stderr, "[bmp] sin memoria para '%s'\n", path);
        return 7;
    }

    if (fseek(f, (long)data_off, SEEK_SET) != 0) {
        free(data); free(rowbuf); fclose(f);
        return 8;
    }

    /* Queremos out->data con fila 0 = ABAJO.
       BMP bottom-up: la fila 0 del archivo ya es la inferior → destino r.
       BMP top-down : la fila 0 del archivo es la superior     → destino H-1-r. */
    for (u32 r = 0; r < H; r++) {
        if (fread(rowbuf, 1, fstride, f) != fstride) {
            free(data); free(rowbuf); fclose(f);
            fprintf(stderr, "[bmp] datos truncados en '%s'\n", path);
            return 9;
        }
        u32 dst = topdown ? (H - 1u - r) : r;
        memcpy(data + (size_t)dst * ostride, rowbuf, ostride);
    }

    free(rowbuf);
    fclose(f);
    out->w = W;
    out->h = H;
    out->data = data;
    printf("[bmp] '%s' cargado: %ux%u 24bpp\n", path, W, H);
    return 0;
}

void bmp_Free(GsTexture *out) {
    if (!out) return;
    free(out->data);
    out->data = NULL;
    out->w = out->h = 0;
}
