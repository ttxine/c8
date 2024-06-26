#include "c8/c8.h"
#include "c8/cpu.h"
#include "c8/keyboard.h"
#include "c8/memory.h"

#include <SDL2/SDL.h>

#include <stdbool.h>
#include <stdlib.h>

typedef enum c8_state {
    C8_STOPPED = 0,
    C8_RUNNING,
    C8_EXITED
} C8State;

typedef struct c8_emulator {
    /* Device */
    C8Memory *memory;
    C8Keyboard *keyboard;
    C8Cpu *cpu;

    /* Render */
    SDL_Window *window;
    bool window_resized;
    SDL_Renderer *renderer;
    SDL_Surface *surface;

    /* Timing */
    uint64_t cpu_ticks;
    uint64_t timers_ticks;

    /* State */
    C8State state;
} C8Emulator;

static int c8_emulator_new_render(C8Emulator *emulator)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        return -1;
    }

    emulator->window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        C8_DISPLAY_WIDTH * 10, C8_DISPLAY_HEIGHT * 10,
        0);
    if (emulator->window == NULL) {
        SDL_Quit();
        return -1;
    }

    emulator->renderer = SDL_CreateRenderer(emulator->window, -1, 0);
    if (emulator->renderer == NULL) {
        SDL_DestroyWindow(emulator->window);
        SDL_Quit();
        return -1;
    }
    SDL_RenderSetLogicalSize(emulator->renderer,
        C8_DISPLAY_WIDTH, C8_DISPLAY_HEIGHT);

    emulator->surface = SDL_CreateRGBSurfaceWithFormat(
        0, C8_DISPLAY_WIDTH, C8_DISPLAY_HEIGHT, 1, SDL_PIXELFORMAT_INDEX1LSB);
    if (emulator->surface == NULL) {
        SDL_DestroyRenderer(emulator->renderer);
        SDL_DestroyWindow(emulator->window);
        SDL_Quit();
        return -1;
    }

    SDL_Color colors[] = {
        {46, 48, 55, 255},
        {235, 229, 206, 255}
    };
    if (SDL_SetPaletteColors(emulator->surface->format->palette, colors,
                              0, 2) < 0) {
        SDL_FreeSurface(emulator->surface);
        SDL_DestroyRenderer(emulator->renderer);
        SDL_DestroyWindow(emulator->window);
        SDL_Quit();
        return -1;
    }

    SDL_ShowCursor(SDL_DISABLE);
}

static void c8_emulator_free_render(C8Emulator *emulator)
{
    if (emulator->surface != NULL) {
        SDL_FreeSurface(emulator->surface);
    }
    if (emulator->renderer != NULL) {
        SDL_DestroyRenderer(emulator->renderer);
    }
    if (emulator->window != NULL) {
        SDL_DestroyWindow(emulator->window);
    }

    SDL_Quit();
}

static int c8_emulator_new_device(C8Emulator *emulator, const uint8_t *program,
                                  size_t size)
{
    emulator->memory = c8_memory_new(program, size);
    if (emulator->memory == NULL) {
        return -1;
    }

    emulator->keyboard = c8_keyboard_new();
    if (emulator->keyboard == NULL) {
        free(emulator->memory);
        return -1;
    }

    emulator->cpu = c8_cpu_new(emulator->memory, emulator->keyboard);
    if (emulator->cpu == NULL) {
        free(emulator->keyboard);
        free(emulator->memory);
        return -1;
    }

    return 0;
}

static void c8_emulator_free_device(C8Emulator *emulator)
{
    if (emulator->cpu != NULL) {
        free(emulator->cpu);
    }
    if (emulator->keyboard != NULL) {
        free(emulator->keyboard);
    }
    if (emulator->memory != NULL) {
        free(emulator->memory);
    }
}

static C8Emulator *c8_emulator_new(const uint8_t *program, size_t size)
{
    C8Emulator *emulator= calloc(sizeof(C8Emulator), 1);
    if (emulator == NULL) {
        fprintf(stderr, "emulator: can't allocate emulator\n");
        return NULL;
    }

    if (c8_emulator_new_render(emulator) < 0) {
        fprintf(stderr, "render: %s\n", SDL_GetError());
        free(emulator);
        return NULL;
    }

    if (c8_emulator_new_device(emulator, program, size) < 0) {
        c8_emulator_free_render(emulator);
        free(emulator);
        return NULL;
    }

    emulator->state = C8_RUNNING;
    return emulator;
}

static void c8_emulator_free(C8Emulator *emulator)
{
    if (emulator != NULL) {
        c8_emulator_free_device(emulator);
        c8_emulator_free_render(emulator);
        free(emulator);
    }
}

static C8Key c8_key_from_sdl(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_1:
        return C8_KEY_1;

    case SDLK_2:
        return C8_KEY_2;

    case SDLK_3:
        return C8_KEY_3;

    case SDLK_q:
        return C8_KEY_4;

    case SDLK_w:
        return C8_KEY_5;

    case SDLK_e:
        return C8_KEY_6;

    case SDLK_a:
        return C8_KEY_7;

    case SDLK_s:
        return C8_KEY_8;

    case SDLK_d:
        return C8_KEY_9;

    case SDLK_z:
        return C8_KEY_A;

    case SDLK_x:
        return C8_KEY_0;

    case SDLK_c:
        return C8_KEY_B;

    case SDLK_4:
        return C8_KEY_C;

    case SDLK_r:
        return C8_KEY_D;

    case SDLK_f:
        return C8_KEY_E;

    case SDLK_v:
        return C8_KEY_F;

    default:
        return C8_KEY_NUM;
    }
}

static void c8_handle_event(C8Emulator *emulator, SDL_Event *event)
{
    switch (event->type){
    case SDL_QUIT:
        emulator->state = C8_STOPPED;
        break;

    case SDL_KEYDOWN:
        c8_keyboard_press_key(emulator->keyboard,
                              c8_key_from_sdl(event->key.keysym.sym));
        break;

    case SDL_KEYUP:
        c8_keyboard_release_key(emulator->keyboard,
                                c8_key_from_sdl(event->key.keysym.sym));
        break;

    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            emulator->window_resized = true;
            break;
        
        default:
            break;
        }

    default:
        break;
    }
}

static void c8_handle_events(C8Emulator *emulator)
{
    SDL_Event event = {};
    while (SDL_PollEvent(&event) > 0) {
        c8_handle_event(emulator, &event);
    }
}

static int c8_handle_cpu(C8Emulator *emulator, uint64_t elapsed_ticks)
{
    const uint64_t target = 1000 / C8_CPU_HZ;
    emulator->cpu_ticks += elapsed_ticks;

    if (emulator->cpu_ticks >= target) {
        emulator->cpu_ticks -= target;
        c8_cpu_execute_instruction(emulator->cpu);
    }

    return 0;
}

static void c8_handle_timers(C8Emulator *emulator, uint64_t elapsed_ticks)
{
    const uint64_t target = 1000 / C8_TIMERS_HZ;
    emulator->timers_ticks += elapsed_ticks;

    if (emulator->timers_ticks >= target) {
        emulator->timers_ticks -= target;
        c8_delay_timer_tick(emulator->cpu);
        c8_sound_timer_tick(emulator->cpu);
    }
}

static void c8_handle_render(C8Emulator *emulator)
{
    if (!c8_display_updated(emulator->cpu) && !emulator->window_resized) {
        return;
    }

    c8_memory_display_read(emulator->memory, emulator->surface->pixels);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(
        emulator->renderer, emulator->surface);
    if (texture == NULL) {
        fprintf(stderr, "render: %s\n", SDL_GetError());
        return;
    }

    SDL_RenderClear(emulator->renderer);
    SDL_RenderCopy(emulator->renderer, texture, NULL, NULL);
    SDL_RenderPresent(emulator->renderer);

    SDL_DestroyTexture(texture);

    emulator->window_resized = false;
}

static void c8_main_loop(C8Emulator *emulator)
{
    uint64_t prev_ticks = SDL_GetTicks64();

    while (emulator->state == C8_RUNNING) {
        uint64_t ticks = SDL_GetTicks64();
        uint64_t elapsed_ticks = ticks - prev_ticks;
        prev_ticks = ticks;

        c8_handle_events(emulator);
        c8_handle_cpu(emulator, elapsed_ticks);
        c8_handle_timers(emulator, elapsed_ticks);
        c8_handle_render(emulator);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s [program]\n", argv[0]);
        return 1;
    }

    size_t size = 0;
    uint8_t *rom = c8_rom_new(argv[1], &size);
    if (rom == NULL) {
        return 1;
    }

    C8Emulator *emulator = c8_emulator_new(rom, size);
    if (emulator == NULL) {
        free(rom);
        return 1;
    }

    c8_main_loop(emulator);

    c8_emulator_free(emulator);
    return 0;
}
