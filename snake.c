#pragma comment(lib, "tcanvas")

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define DYNARR_IMPLEMENTATION
#include "dynarr.h"

#include "tcanvas.h"


#define START_SNAKE_SIZE 4
#define SNAKE_X_VEL .3f
#define SNAKE_X_VEL_IADD .05f

#define SNAKE_Y_VEL .15f
#define SNAKE_Y_VEL_IADD .025f

#define SNAKE_MAX_VEL .95f

// inclusives
s32vec2 get_random_point(s32 min_x, s32 max_x, s32 min_y, s32 max_y) {
    return (s32vec2) {
        min_x + (rand() % (max_x - min_x*2)),
        min_y + (rand() % (max_y - min_y*2))
    };
}

int main(void)
{
    s8 borders[][5] = {"═", "║", "╔", "╗", "╚", "╝"};
    s8 snake_body[][5] = {"◴", "◵", "◶", "◷"};
    s8 *thing_char = "₢";
    s8 *wall_char = "X";
    u8vec4 fore_snake = get_color(0x1ffff00);
    u8vec4 back_snake = {0};
    u8vec4 fore_thing = get_color(0x100ff00);
    u8vec4 back_thing = {0};
    u8vec4 fore_wall = get_color(0x1ff0000);
    u8vec4 back_wall = get_color(0x1909000);

    tc_setup(60);
    srand((u32)time(NULL)); rand();

    DynArr snake;
    DynArr wall;
    bool quit = false;
    while (!quit) {

        snake = dynarr_init(sizeof(s32vec2));
        wall = dynarr_init(sizeof(s32vec2));

        s32 width, height;
        get_terminal_size(&width, &height);

        draw_lasting_char(borders[2], (s32vec2){1,1});
        draw_lasting_char(borders[3], (s32vec2){width,1});
        draw_lasting_char(borders[4], (s32vec2){1,height});
        draw_lasting_char(borders[5], (s32vec2){width,height});
        for (s32 i = 2; i < width; ++i) {
            draw_lasting_char(borders[0], (s32vec2){ i, 1});
            draw_lasting_char(borders[0], (s32vec2){ i, height});
        }
        for (s32 i = 2; i < height; ++i) {
            draw_lasting_char(borders[1], (s32vec2){ 1, i});
            draw_lasting_char(borders[1], (s32vec2){ width, i});
        }

        s32 mid = width/2;
        s32vec2 pos;
        for (int i = 0; i < START_SNAKE_SIZE - 1; ++i) {
            pos = (s32vec2){mid, height/2};
            dynarr_append(&snake, &pos);
            mid++;
        }

        f32vec2 head = {(f32)mid, (f32)height/2};
        pos = vecround(head);
        dynarr_append(&snake, &pos);

        f32vec2 right = {SNAKE_X_VEL};
        f32vec2 left  = {-SNAKE_X_VEL};
        f32vec2 down  = {0, SNAKE_Y_VEL};
        f32vec2 up    = {0, -SNAKE_Y_VEL};

        f32vec2 *dir = &right;

        s32vec2 thing = get_random_point(2, width - 1, 2, height - 1);

        s32 score = 0;
        while (true) {

            char buf[100];
            s32vec2 hod = {4, 2};
#define printf_hod(fmt, ...) sprintf(buf, fmt, __VA_ARGS__); draw_text(buf, hod); hod.y++

            printf_hod("Score: %i", score);

            if (kb_key_down('E') && !dir->y) dir = &up;
            if (kb_key_down('S') && !dir->x) dir = &left;
            if (kb_key_down('D') && !dir->y) dir = &down;
            if (kb_key_down('F') && !dir->x) dir = &right;

            head.x += dir->x;
            head.y += dir->y;
            pos = vecround(head);

            if (pos.x < 2 || pos.x > width - 1 || pos.y < 2 || pos.y > height - 1) break;

            bool hit_wall = false;
            for (int i = 0; i < wall.count; ++i) {
                s32vec2 *p = dynarr_at(&wall, i);
                if (pos.x == p->x && pos.y == p->y) hit_wall = true;
            }
            if (hit_wall) break;

            s32vec2 *prev = dynarr_at(&snake, snake.count - 1);

            if (pos.x == thing.x && pos.y == thing.y) {
                score += 15;
                dynarr_append(&snake, &pos);
                if (right.x < SNAKE_MAX_VEL) {
                    left.x  -= SNAKE_X_VEL_IADD;
                    right.x += SNAKE_X_VEL_IADD;
                    up.y    -= SNAKE_Y_VEL_IADD;
                    down.y  += SNAKE_Y_VEL_IADD;
                }
                thing = get_random_point(2, width - 1, 2, height - 1);
                s32vec2 new_point = get_random_point(2, width - 1, 2, height - 1);
                dynarr_append(&wall, &new_point);

            } else if (prev->x != pos.x || prev->y != pos.y) {
                dynarr_shift_append(&snake, &pos);
            }

            for (int i = 0; i < snake.count; ++i) {
                s32vec2 *p = dynarr_at(&snake, i);
                draw_char_color(snake_body[rand()%4], *p, fore_snake, back_snake);
            }

            for (int i = 0; i < wall.count; ++i) {
                s32vec2 *p = dynarr_at(&wall, i);
                draw_char_color(wall_char, *p, fore_wall, back_wall);
            }

            draw_char_color(thing_char, thing, fore_thing, back_thing);

            tc_render();
        }

        while (true) {

            s8 hod_lines_on_screen = 4;
            s8 idx = 0;
            s32 len;
            s8 buf[100];
            s32vec2 hod = {0, height/2 - hod_lines_on_screen/2 - idx};

            len = sprintf(buf, "Game Over"); hod.y++; idx++;
            hod.x = width/2 - len/2;
            draw_text(buf, hod);

            hod.y++;

            len = sprintf(buf, "Score: %i.", score); hod.y++; idx++;
            hod.x = width/2 - len/2;
            draw_text(buf, hod);

            len = sprintf(buf, "Press 'q' to quit."); hod.y++; idx++;
            hod.x = width/2 - len/2;
            draw_text(buf, hod);

            len = sprintf(buf, "Press 'r' to restart."); hod.y++; idx++;
            hod.x = width/2 - len/2;
            draw_text(buf, hod);

            if (kb_key_down('Q')) { quit = true;         break; }
            if (kb_key_down('R')) { dynarr_free(&snake); break; }

            tc_render();
        }
    }

    flush_input_buffer();
    reset_term();
}
