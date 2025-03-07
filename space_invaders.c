#pragma comment(lib, "tcanvas")

#define _CRT_SECURE_NO_WARNINGS  // sprintf

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "tcanvas.h"


#define PLAYER_VEL .4f
#define BULLET_VEL .4f
#define INIT_BULLET_QTT 3
#define INIT_BULLET_COOLDOWN_MS 500

#define INIT_ENEMIES_QTT 1
#define INIT_ENEMY_VEL_X 0.2f
#define INIT_ENEMY_VEL_Y 0.005f

#define BUF_SIZE 40

typedef struct {
    PosVel posvel;
    bool active;
} Bullet;

typedef struct {
    PosVel posvel;
    int hitpoints;
} Enemy;


void draw_player(s32vec2 pos) {
    pos.x--;
    draw_text("_⍹_", pos);
    pos.y++;
    draw_text("⌾-⌾", pos);
}
void draw_enemy(s32vec2 pos, bool hit) {
    u8vec4 color = hit ? get_color(0x1ff0000) : (u8vec4){0};
    pos.x--;
    draw_text_color("XXX", pos, color, (u8vec4){0});
    pos.y++;
    draw_text_color("XXX", pos, color, (u8vec4){0});
}


int main(void)
{
    tc_setup(60);

    s32 width, height;
    get_terminal_size(&width, &height);

    PosVel player = {
        .pos = { (f32)width/2, (f32)height - 1 },
        .vel = { PLAYER_VEL, .0f }
    };
    DynArr bullets = dynarr_init(sizeof(Bullet));
    for (int i = 0; i < INIT_BULLET_QTT; ++i) {
        Bullet b = { .posvel = { .vel = { .y = BULLET_VEL } } };
        dynarr_append(&bullets, &b);
    }
    DynArr enemies = dynarr_init(sizeof(Enemy));
    Enemy e = {
        .posvel = {
            .pos = {3, 1},
            .vel = {
                .x = INIT_ENEMY_VEL_X,
                .y = INIT_ENEMY_VEL_Y
            }
        },
        .hitpoints = 3
    };
    dynarr_append(&enemies, &e);

    u8vec4 red = get_color(0x1ff0000);
    u64 credits = 0;

    u64 wave_count = 0;
    f32 bullet_time = 0;
    f32 bulllet_cooldown_ms = INIT_BULLET_COOLDOWN_MS;
    bool bulllet_cooldown = false;
    while (true) {

        s32vec2 hod = {2, 1};
        s8 buf[BUF_SIZE];
        sprintf(buf, "Credits: %zu", credits);
        draw_text(buf, hod);
        int len = sprintf(buf, "Wave: %zu", wave_count); assert(len > 0 && len < BUF_SIZE);
        hod.x = width / 2 - len / 2;
        draw_text(buf, hod);

        s8 *limit_hod = "- - -";
        s32 limit = height * 2 / 3;
        draw_text_color(limit_hod, (s32vec2){1, limit}, red, (u8vec4){0});
        draw_text_color(limit_hod, (s32vec2){width - (s32)strlen(limit_hod), limit}, red, (u8vec4){0});

        f32vec2 npos = player.pos;
        if      (kb_key_down('S')) npos.x -= player.vel.x;
        else if (kb_key_down('F')) npos.x += player.vel.x;

        s32vec2 rnpos = vecround(npos);
        if (rnpos.x > 1 && rnpos.x < width) player.pos = npos;

        draw_player(vecround(player.pos));

        if (kb_key_down('E') && !bulllet_cooldown) {
            for (u64 i = 0; i < bullets.count; ++i) {
                Bullet *b = dynarr_at(&bullets, i);
                if (b->active) continue;
                bulllet_cooldown = true;
                b->active = true;
                b->posvel.pos.x = player.pos.x;
                b->posvel.pos.y = player.pos.y - 1;
                break;
            }
        }

        // draw bullet
        for (u64 i = 0; i < bullets.count; ++i) {
            Bullet *b = dynarr_at(&bullets, i);
            if (!b->active) continue;
            b->posvel.pos.y -= b->posvel.vel.y;
            if (b->posvel.pos.y < 1) b->active = false;
            draw_char_color("|", vecround(b->posvel.pos), red, (u8vec4){0});
        }

        // draw enemy
        bool limit_reached = false;
        bool wave_active = false;
        for (u64 i = 0; i < enemies.count; ++i) {
            Enemy *e = dynarr_at(&enemies, i);
            if (e->hitpoints <= 0) continue;
            wave_active = true;
            f32vec2 npos = e->posvel.pos;
            npos.x += e->posvel.vel.x;
            npos.y += e->posvel.vel.y;
            if (npos.x >= width || npos.x <= 1) e->posvel.vel.x *= -1;
            e->posvel.pos.x += e->posvel.vel.x;
            e->posvel.pos.y += e->posvel.vel.y;

            if (lroundf(e->posvel.pos.y) == limit) limit_reached = true;

            // collision bullet x enemy
            bool hit = false;
            for (u64 i = 0; i < bullets.count; ++i) {
                Bullet *b = dynarr_at(&bullets, i);
                if (!b->active) continue;
                f32 ex = e->posvel.pos.x;
                f32 ey = e->posvel.pos.y;
                f32 bx = b->posvel.pos.x;
                f32 by = b->posvel.pos.y;
                if (bx >= ex - 1 && bx <= ex + 1 && by >= ey && by <= ey + 1) {
                    hit = true;
                    b->active = false;
                    e->hitpoints--;
                    if (!e->hitpoints) credits += 5;
                }
            }
            draw_enemy(vecround(e->posvel.pos), hit);
        }
        if (limit_reached) break;

        if (!wave_active) {
            wave_count++;
            if (wave_count % 10) {
                dynarr_append(&enemies, &e);
                for (u64 i = 0; i < enemies.count; ++i) {
                    Enemy *en = dynarr_at(&enemies, i);
                    f32 y = (f32)((i / 2) * 3 + 1);

                    bool right = i % 2;
                    f32vec2 pos = { (f32)(rand()%(width-1) + 1), y};
                    en->posvel.pos = pos;
                    en->hitpoints = 3;
                }
            } else {
                enemies.count = 1;
                Enemy *en = dynarr_at(&enemies, 0);
                en->posvel.pos = (f32vec2){2, 1};
                en->hitpoints = 3;
            }
        }

        f32 frame_delta = tc_render();
        if (bulllet_cooldown) bullet_time += frame_delta;

        if (bullet_time > bulllet_cooldown_ms) {
            bulllet_cooldown = false;
            bullet_time = 0;
        }
    }

    flush_input_buffer();
    reset_term();
}
