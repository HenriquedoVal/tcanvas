#pragma once

#include <stdbool.h>

// data defs
typedef signed long long     s64;
typedef signed int           s32;
typedef signed short         s16;
typedef char                 s8;

typedef unsigned long long   u64;
typedef unsigned             u32;
typedef unsigned short       u16;
typedef unsigned char        u8;

typedef float                f32;
typedef double               f64;

typedef struct {
    u8 a;
    u8 r;
    u8 g;
    u8 b;
} u8vec4;

typedef struct {
    f32 x;
    f32 y;
} f32vec2;

typedef struct {
    f64 x;
    f64 y;
} f64vec2;

typedef struct {
    s32 x;
    s32 y;
} s32vec2;


// most used defs
typedef struct {
    f32vec2 pos;
    f32vec2 vel;
} PosVel;

#ifdef BUILD_ENGINE
#define DYNARR_EXTERN_FUNCS
#define DYNARR_IMPLEMENTATION
#endif
#include "dynarr.h"

#define DllExport   __declspec( dllexport )

void    DllExport tc_setup(f32 fps);
f32     DllExport tc_render(void);
void    DllExport draw_char(s8 *mbc, s32vec2 p);
void    DllExport draw_char_color(s8 *mbc, s32vec2 p, u8vec4 fore, u8vec4 back);
void    DllExport draw_text(s8 *str, s32vec2 p);
void    DllExport draw_text_color(s8 *str, s32vec2 p, u8vec4 fore, u8vec4 back);
void    DllExport draw_rectangle(s32 x, s32 y, s32 w, s32 h, s8 borders[6][5]);
void    DllExport draw_lasting_char(s8 *mbc, s32vec2 p);
u8vec4  DllExport get_color(u32 argb);
void    DllExport reset_term(void);
s32vec2 DllExport vecround(f32vec2 x);

bool DllExport get_terminal_size(s32 *width, s32 *height);
bool DllExport kb_key_down(s32 key);
void DllExport flush_input_buffer(void);
