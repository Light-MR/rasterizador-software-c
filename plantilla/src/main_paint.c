/*
 * main_paint.c — MiniPaint
 *
 * Aplicación de dibujo pixel-art sobre libosw.
 * Paleta de 64 colores, herramienta lápiz (Bresenham), guardado PPM.
 *
 * Compilar:  plantilla\build_paint.bat  →  paint.exe
 * Controles:
 *   Clic izquierdo en lienzo : dibujar
 *   Clic en paleta           : seleccionar color
 *   Clic en cajas de color   : intercambiar color actual / anterior
 *   S                        : guardar dibujo como dibujo.ppm
 */

#include <libosw/osw.h>
#include <stdio.h>

#define FB_H 240
#define FB_W 320
#define PALETA_W 80

u32 framebuffer[FB_W * FB_H];

u32 current_color = 0xFFEED6DC;
u32 prev_Color    = 0xFFC9B7E8;

#define BOX_H 24
#define BOX_W (PALETA_W / 2)
#define PAL_COLS 8
#define PAL_ROWS 8
#define PAL_START_Y BOX_H
#define CELL_W (PALETA_W / PAL_COLS)
#define CELL_H 10
#define PAL_GRID_H (PAL_ROWS * CELL_H)

static const u32 palette[PAL_ROWS * PAL_COLS] = {
    0xFFFDEBD0, 0xFFF5CBA7, 0xFFE8B88A, 0xFFD4956B,
    0xFFC07850, 0xFFA0522D, 0xFF6B3A2A, 0xFF3B1F12,
    0xFFFF0000, 0xFFCC0000, 0xFF880000, 0xFFFF4444,
    0xFFFF8800, 0xFFFF6600, 0xFFCC4400, 0xFFFF2266,
    0xFFFFFF00, 0xFFFFDD00, 0xFFFFAA00, 0xFFFFCC44,
    0xFFF0E68C, 0xFFDAA520, 0xFFB8860B, 0xFFFFF8DC,
    0xFF00FF00, 0xFF00CC00, 0xFF008800, 0xFF004400,
    0xFF44FF44, 0xFF88CC44, 0xFF228B22, 0xFF2E8B57,
    0xFF0088FF, 0xFF0044CC, 0xFF002288, 0xFF87CEEB,
    0xFF4488FF, 0xFF1E90FF, 0xFF000066, 0xFF00CCFF,
    0xFFFF00FF, 0xFFAA00AA, 0xFF660088, 0xFFCC44FF,
    0xFFFF88CC, 0xFFFF4488, 0xFFEE82EE, 0xFF9966CC,
    0xFF8B4513, 0xFFA0522D, 0xFFD2691E, 0xFFDEB887,
    0xFF3E1C00, 0xFF5C3317, 0xFF8B7355, 0xFFD2B48C,
    0xFF000000, 0xFF1C1C1C, 0xFF444444, 0xFF888888,
    0xFFBBBBBB, 0xFFDDDDDD, 0xFFFFFFFF, 0xFFFF0088,
};

#define KEYCODE_S 0x1F

void fillArea(u32 color, u32 x, u32 y, u32 w, u32 h){
    for(u32 j = y; j < y + h; j++)
        for(u32 i = x; i < x + w; i++)
            framebuffer[j*FB_W+i] = color;
}

void set_pixel(s32 x, s32 y, u32 color){
    if(x < 0 || y < 0 || x>=FB_W || y >= FB_H) return;
    framebuffer[y*FB_W+x] = color;
}

u32 esLienzo(s32 x, s32 y){
    if (x >(s32)PALETA_W) return 1;
    return 0;
}

void dibujaLinea(s32 x0, s32 y0, s32 x1, s32 y1){
    s32 dx = x1-x0; if(dx<0) dx=-dx;
    s32 dy = y1-y0; if(dy<0) dy=-dy;
    s32 sx = (x0<x1)?1:-1;
    s32 sy = (y0<y1)?1:-1;
    s32 err = dx-dy;
    while(1){
        if(esLienzo(x0,y0)) set_pixel(x0,y0,current_color);
        if(x0==x1 && y0==y1) break;
        s32 e2 = err*2;
        if(e2>-dy){err-=dy; x0+=sx;}
        if(e2< dx){err+=dx; y0+=sy;}
    }
}

void dibujaPaleta(void){
    for(u32 row=0;row<PAL_ROWS;row++)
        for(u32 col=0;col<PAL_COLS;col++)
            fillArea(palette[row*PAL_COLS+col],col*CELL_W+1,PAL_START_Y+row*CELL_H+1,CELL_W-1,CELL_H-1);
}

void dibujaCajas(void){
    fillArea(current_color,1,1,BOX_W-2,BOX_H-2);
    fillArea(prev_Color,BOX_W+1,1,BOX_W-2,BOX_H-2);
}

void dibujaSeparador(void){
    fillArea(0xFF1C1C1C,PALETA_W,0,1,FB_H);
}

void GuardarDibujo(const char* nombre){
    FILE* f=fopen(nombre,"wb");
    if(!f){printf("Error al guardar\n");return;}
    fprintf(f,"P6\n%d %d\n255\n",FB_W,FB_H);
    for(u32 i=0;i<FB_W*FB_H;i++){
        u32 px=framebuffer[i];
        u8 r=(px>>16)&0xFF, g=(px>>8)&0xFF, b=px&0xFF;
        fwrite(&r,1,1,f); fwrite(&g,1,1,f); fwrite(&b,1,1,f);
    }
    fclose(f);
    printf("Guardado: %s\n",nombre);
}

int main(){
    u32 err=OSW_Init("MiniPaint",FB_W,FB_H,0);
    if(err!=OSW_OK) return err;
    fillArea(0xFF1C1C1C,PALETA_W+1,0,FB_W-PALETA_W-1,FB_H);
    fillArea(0xFF1C1317,0,BOX_H+PAL_GRID_H,PALETA_W,FB_H-BOX_H-PAL_GRID_H);
    dibujaPaleta(); dibujaCajas(); dibujaSeparador();
    OSW_MouseSetPolling(1);
    OSW_KeyboardSetPolling(1);
    OSWMouse mouse;
    u32 btn_prev=0;
    s32 prev_x=-1,prev_y=-1;
    while(1){
        OSW_Poll();
        OSWKeyEvent kev;
        while(OSW_KeyboardGetEvent(&kev))
            if(kev.type==OSW_KEYEV_TYPE_PRESSED && kev.keycode==KEYCODE_S)
                GuardarDibujo("dibujo.ppm");
        OSW_MouseGetState(&mouse);
        u32 btn=mouse.btn&OSW_MOUSE_BTN0;
        if(btn && !btn_prev){
            u32 mx=(u32)mouse.x, my=(u32)mouse.y;
            if(mx<PALETA_W){
                if(my<BOX_H){ u32 t=current_color; current_color=prev_Color; prev_Color=t; dibujaCajas(); }
                else if(my>=PAL_START_Y && my<PAL_START_Y+PAL_GRID_H){
                    u32 col=mx/CELL_W, row=(my-PAL_START_Y)/CELL_H;
                    prev_Color=current_color;
                    current_color=palette[row*PAL_COLS+col];
                    dibujaCajas();
                }
            }
        }
        btn_prev=btn;
        if(btn){
            s32 mx=(s32)mouse.x, my=(s32)mouse.y;
            if(esLienzo(mx,my)){
                if(prev_x>=0) dibujaLinea(prev_x,prev_y,mx,my);
                else set_pixel(mx,my,current_color);
                prev_x=mx; prev_y=my;
            } else { prev_x=-1; prev_y=-1; }
        } else { prev_x=-1; prev_y=-1; }
        OSW_VideoDrawBuffer(framebuffer,FB_W,FB_H);
        OSW_VideoSwapBuffers();
    }
    return 0;
}
