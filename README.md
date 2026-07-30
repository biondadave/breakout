# Breakout (SDL2 / C)

A classic Breakout clone written in C using SDL2. Single source file, no external assets required.

## Controls
- **Left / Right arrows** or **A / D** — move paddle
- **Space** — launch the ball, and restart after game over / win
- **Esc** — quit

## Requirements
- GCC (or any C compiler)
- SDL2 development headers
- `pkg-config` (used by the Makefile to find SDL2's flags)

### Install SDL2 dev headers

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libsdl2-dev
```

**macOS (Homebrew):**
```bash
brew install sdl2
```

**Windows:**
Easiest path is via [MSYS2](https://www.msys2.org/):
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-gcc
```

## Build & Run

```bash
make        # compiles ./breakout
make run    # compiles and runs
make clean  # removes the binary
```

Or manually:
```bash
gcc -Wall -Wextra -O2 $(pkg-config --cflags sdl2) -o breakout main.c $(pkg-config --libs sdl2) -lm
./breakout
```

## How it works (for extending the code)

- **Game loop**: fixed ~60 FPS loop using `SDL_GetTicks()` for delta time, capped to avoid huge jumps if the window is dragged/paused.
- **Paddle & ball**: stored as float positions (`x, y`) synced into `SDL_Rect` each frame for precise, frame-rate-independent movement.
- **Collisions**: simple AABB (`SDL_HasIntersection`) checks against the paddle, walls, and bricks. The paddle bounce angle depends on *where* the ball hits the paddle (edges = sharper angle).
- **Bricks**: a 2D array (`BRICK_ROWS x BRICK_COLS`), each row tinted a different color, classic-Breakout style.
- **Score/lives**: shown in the window title bar (no font rendering library needed — keeps the project dependency-free). If you want on-screen text, add `SDL2_ttf` and load a `.ttf` font.

## Ideas to extend it
- On-screen HUD text via `SDL2_ttf`
- Power-ups (multi-ball, wider paddle, slow ball) dropping from bricks
- Multiple levels with different brick layouts
- Sound effects via `SDL2_mixer`
- A start menu / pause screen
