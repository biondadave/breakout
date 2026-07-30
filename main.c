/*
 * Breakout — a simple SDL2 game in C
 *
 * Controls:
 *   Left/Right arrows or A/D  - move paddle
 *   Space                     - launch ball / restart after game over
 *   Esc                       - quit
 *
 * Build:
 *   make
 * Run:
 *   ./breakout
 */

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------- Config ---------- */
#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600
#define FPS            60
#define FRAME_DELAY_MS (1000 / FPS)

#define PADDLE_WIDTH   100
#define PADDLE_HEIGHT  16
#define PADDLE_SPEED   480.0f   /* pixels per second */
#define PADDLE_Y_OFFSET 40

#define BALL_SIZE      14
#define BALL_SPEED     360.0f   /* pixels per second */

#define BRICK_ROWS     6
#define BRICK_COLS     10
#define BRICK_HEIGHT   24
#define BRICK_PADDING  6
#define BRICK_TOP_OFFSET 60
#define BRICK_SIDE_OFFSET 35

#define STARTING_LIVES 3

/* ---------- Types ---------- */
typedef struct {
    SDL_Rect rect;
    float x, y;   /* precise position (rect uses ints) */
} Paddle;

typedef struct {
    SDL_Rect rect;
    float x, y;
    float vx, vy;
    bool stuck_to_paddle; /* true before launch */
} Ball;

typedef struct {
    SDL_Rect rect;
    bool alive;
    SDL_Color color;
} Brick;

typedef enum {
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

/* ---------- Globals ---------- */
static Brick bricks[BRICK_ROWS][BRICK_COLS];

/* Row colors, top to bottom (classic Breakout palette) */
static const SDL_Color row_colors[BRICK_ROWS] = {
    {214, 40, 40, 255},   /* red */
    {247, 127, 0, 255},   /* orange */
    {252, 191, 73, 255},  /* yellow */
    {124, 181, 24, 255},  /* green */
    {0, 143, 179, 255},   /* blue */
    {106, 76, 156, 255},  /* purple */
};

static void init_bricks(void) {
    int brick_width = (WINDOW_WIDTH - 2 * BRICK_SIDE_OFFSET
                        - (BRICK_COLS - 1) * BRICK_PADDING) / BRICK_COLS;

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &bricks[r][c];
            b->rect.w = brick_width;
            b->rect.h = BRICK_HEIGHT;
            b->rect.x = BRICK_SIDE_OFFSET + c * (brick_width + BRICK_PADDING);
            b->rect.y = BRICK_TOP_OFFSET + r * (BRICK_HEIGHT + BRICK_PADDING);
            b->alive = true;
            b->color = row_colors[r];
        }
    }
}

static int bricks_remaining(void) {
    int count = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
        for (int c = 0; c < BRICK_COLS; c++)
            if (bricks[r][c].alive) count++;
    return count;
}

static void reset_ball_and_paddle(Paddle *paddle, Ball *ball) {
    paddle->x = (WINDOW_WIDTH - PADDLE_WIDTH) / 2.0f;
    paddle->y = WINDOW_HEIGHT - PADDLE_Y_OFFSET;
    paddle->rect.w = PADDLE_WIDTH;
    paddle->rect.h = PADDLE_HEIGHT;

    ball->stuck_to_paddle = true;
    ball->x = paddle->x + PADDLE_WIDTH / 2.0f - BALL_SIZE / 2.0f;
    ball->y = paddle->y - BALL_SIZE;
    ball->vx = 0;
    ball->vy = 0;
    ball->rect.w = BALL_SIZE;
    ball->rect.h = BALL_SIZE;
}

static void sync_rect(SDL_Rect *rect, float x, float y) {
    rect->x = (int)x;
    rect->y = (int)y;
}

/* Axis-aligned bounding box collision */
static bool aabb_intersect(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Breakout",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        fprintf(stderr, "CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Paddle paddle;
    Ball ball;
    init_bricks();
    reset_ball_and_paddle(&paddle, &ball);

    int score = 0;
    int lives = STARTING_LIVES;
    GameState state = STATE_PLAYING;

    bool running = true;
    Uint32 last_ticks = SDL_GetTicks();

    while (running) {
        Uint32 frame_start = SDL_GetTicks();
        float dt = (frame_start - last_ticks) / 1000.0f;
        last_ticks = frame_start;
        if (dt > 0.05f) dt = 0.05f; /* clamp to avoid huge jumps (e.g. window drag) */

        /* ---- Input handling ---- */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_SPACE) {
                    if (state == STATE_PLAYING && ball.stuck_to_paddle) {
                        ball.stuck_to_paddle = false;
                        ball.vx = BALL_SPEED * 0.5f;
                        ball.vy = -BALL_SPEED;
                    } else if (state == STATE_GAME_OVER || state == STATE_WIN) {
                        /* restart */
                        init_bricks();
                        reset_ball_and_paddle(&paddle, &ball);
                        score = 0;
                        lives = STARTING_LIVES;
                        state = STATE_PLAYING;
                    }
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (state == STATE_PLAYING) {
            /* ---- Paddle movement ---- */
            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
                paddle.x -= PADDLE_SPEED * dt;
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
                paddle.x += PADDLE_SPEED * dt;

            if (paddle.x < 0) paddle.x = 0;
            if (paddle.x + PADDLE_WIDTH > WINDOW_WIDTH)
                paddle.x = WINDOW_WIDTH - PADDLE_WIDTH;
            sync_rect(&paddle.rect, paddle.x, paddle.y);

            /* ---- Ball movement ---- */
            if (ball.stuck_to_paddle) {
                ball.x = paddle.x + PADDLE_WIDTH / 2.0f - BALL_SIZE / 2.0f;
                ball.y = paddle.y - BALL_SIZE;
            } else {
                ball.x += ball.vx * dt;
                ball.y += ball.vy * dt;

                /* Wall collisions */
                if (ball.x <= 0) {
                    ball.x = 0;
                    ball.vx = -ball.vx;
                } else if (ball.x + BALL_SIZE >= WINDOW_WIDTH) {
                    ball.x = WINDOW_WIDTH - BALL_SIZE;
                    ball.vx = -ball.vx;
                }
                if (ball.y <= 0) {
                    ball.y = 0;
                    ball.vy = -ball.vy;
                }

                /* Fell below paddle -> lose a life */
                if (ball.y > WINDOW_HEIGHT) {
                    lives--;
                    if (lives <= 0) {
                        state = STATE_GAME_OVER;
                    } else {
                        reset_ball_and_paddle(&paddle, &ball);
                    }
                }
            }
            sync_rect(&ball.rect, ball.x, ball.y);

            /* ---- Paddle collision ---- */
            if (!ball.stuck_to_paddle && aabb_intersect(ball.rect, paddle.rect) && ball.vy > 0) {
                ball.y = paddle.y - BALL_SIZE;
                /* Reflect angle based on where it hit the paddle */
                float hit_pos = (ball.x + BALL_SIZE / 2.0f - paddle.x) / (float)PADDLE_WIDTH; /* 0..1 */
                float angle = (hit_pos - 0.5f) * 2.0f; /* -1..1 */
                ball.vx = BALL_SPEED * angle;
                ball.vy = -fabsf(ball.vy);
                sync_rect(&ball.rect, ball.x, ball.y);
            }

            /* ---- Brick collisions ---- */
            for (int r = 0; r < BRICK_ROWS && !ball.stuck_to_paddle; r++) {
                for (int c = 0; c < BRICK_COLS; c++) {
                    Brick *b = &bricks[r][c];
                    if (!b->alive) continue;
                    if (aabb_intersect(ball.rect, b->rect)) {
                        b->alive = false;
                        score += 10;

                        /* Determine bounce direction by comparing overlap depth */
                        float ball_cx = ball.x + BALL_SIZE / 2.0f;
                        float ball_cy = ball.y + BALL_SIZE / 2.0f;
                        float brick_cx = b->rect.x + b->rect.w / 2.0f;
                        float brick_cy = b->rect.y + b->rect.h / 2.0f;
                        float dx = ball_cx - brick_cx;
                        float dy = ball_cy - brick_cy;
                        float overlap_x = (BALL_SIZE / 2.0f + b->rect.w / 2.0f) - fabsf(dx);
                        float overlap_y = (BALL_SIZE / 2.0f + b->rect.h / 2.0f) - fabsf(dy);

                        if (overlap_x < overlap_y) {
                            ball.vx = -ball.vx;
                        } else {
                            ball.vy = -ball.vy;
                        }
                        goto brick_done; /* only resolve one brick per frame */
                    }
                }
            }
            brick_done:

            if (bricks_remaining() == 0) {
                state = STATE_WIN;
            }

            /* Update window title with score / lives */
            char title[128];
            snprintf(title, sizeof(title), "Breakout — Score: %d  Lives: %d", score, lives);
            SDL_SetWindowTitle(window, title);
        } else {
            char title[160];
            const char *msg = (state == STATE_WIN) ? "You Win!" : "Game Over";
            snprintf(title, sizeof(title), "Breakout — %s — Score: %d — Press SPACE to restart", msg, score);
            SDL_SetWindowTitle(window, title);
        }

        /* ---- Rendering ---- */
        SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
        SDL_RenderClear(renderer);

        /* Bricks */
        for (int r = 0; r < BRICK_ROWS; r++) {
            for (int c = 0; c < BRICK_COLS; c++) {
                Brick *b = &bricks[r][c];
                if (!b->alive) continue;
                SDL_SetRenderDrawColor(renderer, b->color.r, b->color.g, b->color.b, 255);
                SDL_RenderFillRect(renderer, &b->rect);
                SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
                SDL_RenderDrawRect(renderer, &b->rect);
            }
        }

        /* Paddle */
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderFillRect(renderer, &paddle.rect);

        /* Ball */
        SDL_SetRenderDrawColor(renderer, 255, 214, 10, 255);
        SDL_RenderFillRect(renderer, &ball.rect);

        SDL_RenderPresent(renderer);

        /* ---- Frame limiting ---- */
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY_MS) {
            SDL_Delay(FRAME_DELAY_MS - frame_time);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
