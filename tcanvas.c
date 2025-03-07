#define _CRT_SECURE_NO_WARNINGS
#include <math.h>

#define BUILD_ENGINE
#include "tcanvas.h"

#define TERM_IMPLEMENTATION
#include "term.h"
#include "win_term.c"


// inner stuff
typedef struct {
    s32vec2 pos;
    u64 scratch_offset;
    u64 size;
} QueueItem;

static struct {
    DynArr drawing_queue[2];
    DynArr terminal_instructions;
    DynArr scratch_da;
    bool queue_selector;
    s64 counts_per_ms;
    f32 frame_time;
    s32 width;
    s32 height;
} TCANVAS;


static u64 utf8len(const s8 *str)
{
    u64 count = 0;
    u8 c;

    // shitty thing, may overrun buffer on malformed utf8
    while ((c = (u8)*str)) {
        s8 add = 0;

        c >>= 4;
        if      (!(c ^ 0b1111)) add = 4;
        else if (!(c ^ 0b1110)) add = 3;
        else if (!(c ^ 0b1100)) add = 2;
        else                    add = 1;

        count++;
        str += add;
    }

    return count;
}


static s8 *utf8_at(s8 *str, u64 idx, s8 *size)
{
    u64 count = 0;
    u8 c;

    // shitty thing too
    while ((c = (u8)*str)) {
        s8 add;

        c >>= 4;
        if      (!(c ^ 0b1111)) add = 4;
        else if (!(c ^ 0b1110)) add = 3;
        else if (!(c ^ 0b1100)) add = 2;
        else                    add = 1;

        *size = add;
        if (count == idx) return str;
        str += add;
        count++;
    }

    return NULL;
}


static f32 _sleep_frame_delta(void)
{
    static s64 prev = 0;
    LARGE_INTEGER li;

    if (prev == 0) {
        BOOL res = QueryPerformanceCounter(&li);
        assert(res);
        prev = li.QuadPart;
        Sleep((DWORD)TCANVAS.frame_time);
        return TCANVAS.frame_time;
    }

    BOOL res = QueryPerformanceCounter(&li);
    assert(res);

    s64 end = li.QuadPart;
    f32 delta = (f32)(end - prev) / TCANVAS.counts_per_ms;
    prev = end;

    if (delta < TCANVAS.frame_time) {
        Sleep((DWORD)(TCANVAS.frame_time - delta));
        delta = TCANVAS.frame_time;
    } 

    return delta;
}


s32vec2 vecround(f32vec2 x)
{
    return (s32vec2){lroundf(x.x), lroundf(x.y)};
}


void reset_term(void)
{
    term_show_cursor();
    term_main_sc_buf();
}


u8vec4 get_color(u32 argb)
{
    return (u8vec4) {
        (argb & 0xff000000) >> 24,
        (argb & 0x00ff0000) >> 16,
        (argb & 0x0000ff00) >> 8,
        (argb & 0x000000ff) 
    };
}


void draw_lasting_char(s8 *mbc, s32vec2 p)
{
    term_set_cursor_position(p.x, p.y);
    printf("%s", mbc);
}


void draw_char_color(s8 *mbc, s32vec2 p, u8vec4 fore, u8vec4 back)
{
    if (p.x < 1 || p.x > TCANVAS.width || p.y < 1 || p.y > TCANVAS.height) return;

    s8 buf[40];
    s32 wrote;
    s32 written = 0;
    DynArr *scratch_da = &TCANVAS.scratch_da;
    u64 offset = scratch_da->count;

    if (fore.a) {
        wrote = term_buf_sgr_ex_color(buf, true, fore.r, fore.g, fore.b);
        dynarr_append_str(scratch_da, buf, wrote);
        written += wrote;
    }
    if (back.a) {
        wrote = term_buf_sgr_ex_color(buf, false, back.r, back.g, back.b);
        dynarr_append_str(scratch_da, buf, wrote);
        written += wrote;
    }

    wrote = term_buf_set_cursor_position(buf, p.x, p.y);
    dynarr_append_str(scratch_da, buf, wrote);
    written += wrote;

    size_t len = strlen(mbc);
    dynarr_append_str(scratch_da, mbc, (s32)len);
    written += (s32)len;

    wrote = term_buf_sgr(buf, SGR_DEFAULT);
    dynarr_append_str(scratch_da, buf, wrote);
    written += wrote;

    DynArr *queue = dynarr_at(&TCANVAS.drawing_queue[TCANVAS.queue_selector], p.y - 1);
    QueueItem item = { p, offset, written};
    dynarr_append(queue, &item);
}


void draw_char(s8 *mbc, s32vec2 p) { draw_char_color(mbc, p, (u8vec4){0}, (u8vec4){0}); }


void draw_text_color(s8 *str, s32vec2 p, u8vec4 fore, u8vec4 back)
{
    DynArr *queue = dynarr_at(&TCANVAS.drawing_queue[TCANVAS.queue_selector], p.y - 1);

    u64 len = utf8len(str);
    for (u64 i = 0; i < len; i++) {
        s8 size, buf[5];
        s8 *mbc = utf8_at(str, i, &size);

        memcpy(buf, mbc, size);
        buf[size] = '\0';

        draw_char_color(buf, p, fore, back);

        // TODO: deal with newlines
        p.x++;
    }
}

void draw_text(s8 *str, s32vec2 p) { draw_text_color(str, p, (u8vec4){0}, (u8vec4){0}); }

void draw_rectangle(s32 x, s32 y, s32 w, s32 h, s8 borders[6][5])
{
    s8 _borders[][5] = {"─", "│", "┌", "┐", "└", "┘"};
    if (borders == NULL) borders = _borders;

    draw_char(borders[2], (s32vec2){ x,     y });
    draw_char(borders[3], (s32vec2){ x+w-1, y });
    draw_char(borders[4], (s32vec2){ x,     y+h-1 });
    draw_char(borders[5], (s32vec2){ x+w-1, y+h-1 });

    for (s32 i = x + 1; i < x + w - 1; ++i) {
        draw_char(borders[0], (s32vec2){ i, y });
        draw_char(borders[0], (s32vec2){ i, y+h-1 });
    }
    for (s32 i = y + 1; i < y + h - 1; ++i) {
        draw_char(borders[1], (s32vec2){ x,     i });
        draw_char(borders[1], (s32vec2){ x+w-1, i });
    }
}


f32 tc_render(void)
{
    s8 buf[40];
    DynArr *scratch_da = &TCANVAS.scratch_da;
    DynArr *ti = &TCANVAS.terminal_instructions;
    u64 rows = TCANVAS.drawing_queue[0].count;
    assert(TCANVAS.drawing_queue[1].count == rows);

    // check terminal resize
    s32 width, height;
    get_terminal_size(&width, &height);
    if (height > TCANVAS.height) {
        for (s8 i = 0; i < 2; ++i) {
            for (s32 j = 0; j < height - TCANVAS.height; ++j) {
                DynArr queue = dynarr_init(sizeof(QueueItem));
                dynarr_append(&TCANVAS.drawing_queue[i], &queue);
            }
        }
    }
    TCANVAS.width  = width;
    TCANVAS.height = height;

    // Remove chars from screen.
    // I don't want to clear all screen and draw again to avoid flickering
    for (s32 row = 0; row < rows; ++row) {
        DynArr *queue = dynarr_at(&TCANVAS.drawing_queue[TCANVAS.queue_selector], row);
        DynArr *prev_queue = dynarr_at(&TCANVAS.drawing_queue[!TCANVAS.queue_selector], row);

        while (prev_queue->count) {
            QueueItem *prev = dynarr_pop(prev_queue); assert(prev);
            bool on_screen = false;

            // Since we're popping above, makes sense to do a reverse scan
            for (s32 j = (s32)queue->count - 1; j >= 0; --j) {
                QueueItem *current = dynarr_at(queue, (u64)j); assert(current);

                if (prev->pos.x == current->pos.x && prev->pos.y == current->pos.y) {
                    on_screen = true;
                    break;
                }
            }

            if (!on_screen) {
                s32 wrote = term_buf_set_cursor_position(buf, prev->pos.x, prev->pos.y);
                dynarr_append_str(ti, buf, wrote);
                wrote = term_buf_mod(buf, MOD_ERASE_CHAR, 1);
                dynarr_append_str(ti, buf, wrote);
            }
        }
    }

    // Only draw the last call to draw_char for each (x, y) to avoid flickering
    for (s32 row = 0; row < rows; ++row) {
        DynArr *queue = dynarr_at(&TCANVAS.drawing_queue[TCANVAS.queue_selector], row);

        for (s32 i = 0; i < queue->count; ++i) {
            QueueItem *current = dynarr_at(queue, (u64)i); assert(current);

            bool under = false;
            for (s32 j = i + 1; j < queue->count; ++j) {
                QueueItem *next = dynarr_at(queue, (u64)j); assert(next);
                assert(current->pos.y == next->pos.y);
                if (current->pos.x == next->pos.x) {
                    under = true;
                    break;
                }
            }
            if (under) continue;

            dynarr_append_str(ti, dynarr_at(scratch_da, current->scratch_offset), (s32)current->size);
        }
    }

    s8 null = '\0';
    dynarr_append(ti, &null);
    printf("%s", (s8 *)ti->_data);

    dynarr_reset(ti);
    dynarr_reset(scratch_da);
    TCANVAS.queue_selector = !TCANVAS.queue_selector;

    return _sleep_frame_delta();
}


void tc_setup(f32 fps)
{
    get_terminal_size(&TCANVAS.width, &TCANVAS.height);

    TCANVAS.terminal_instructions = dynarr_init(sizeof(s8));
    TCANVAS.scratch_da = dynarr_init(sizeof(s8));

    for (s8 i = 0; i < 2; ++i) {
        TCANVAS.drawing_queue[i] = dynarr_init(sizeof(DynArr));
        for (s32 j = 0; j < TCANVAS.height; ++j) {
            DynArr queue = dynarr_init(sizeof(QueueItem));
            dynarr_append(&TCANVAS.drawing_queue[i], &queue);
        }
    }

    LARGE_INTEGER li;
    BOOL res = QueryPerformanceFrequency(&li); assert(res);
    TCANVAS.counts_per_ms = li.QuadPart / 1000;
    TCANVAS.frame_time = 1000/fps;

    register_ctrlc_handler(reset_term);

    term_alt_sc_buf();
    term_hide_cursor();
}
