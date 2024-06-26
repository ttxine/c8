#include "c8/cpu.h"

#include "c8/audio.h"
#include "c8/instruction.h"
#include "c8/keyboard.h"
#include "c8/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct c8_cpu {
    uint8_t v[16];
    uint16_t i;
    uint8_t dt;
    uint8_t st;
    uint16_t pc;
    uint8_t sp;

    C8Memory *memory;
    C8Keyboard *keyboard;
    C8Audio *audio;

    uint16_t instruction;
};

C8Cpu *c8_cpu_new(C8Memory *memory, C8Keyboard *keyboard)
{
    C8Cpu *cpu = calloc(1, sizeof(C8Cpu));

    if (cpu == NULL) {
        fprintf(stderr, "cpu: can't allocate cpu\n");
        return NULL;
    }

    cpu->memory = memory;
    cpu->keyboard = keyboard;
    cpu->audio = c8_audio_new();
    srand(time(0));

    cpu->pc = c8_memory_program_begin();
    return cpu;
}

C8Cpu *c8_cpu_free(C8Cpu *cpu)
{
    if (cpu != NULL) {
        if (cpu->audio) {
            c8_audio_free(cpu->audio);
        }
        if (cpu->memory != NULL) {
            free(cpu->memory);
        }
        if (cpu->keyboard != NULL) {
            free(cpu->keyboard);
        }
        free(cpu);
    }
}

static int c8_cpu_cls(C8Cpu *cpu)
{
    c8_memory_display_clear(cpu->memory);
    return 1;
}

static int c8_cpu_ret(C8Cpu *cpu)
{
    uint16_t addr = 0;

    if (c8_memory_stack_read(cpu->memory, cpu->sp, &addr) < 0) {
        return -1;
    }

    cpu->pc = addr;
    cpu->sp--;

    return 1;
}

static int c8_cpu_jp_i12(C8Cpu *cpu)
{
    cpu->pc = c8_instruction_get_nnn(cpu->instruction);
    return 0;
}

static int c8_cpu_jp_reg_i12(C8Cpu *cpu)
{
    cpu->pc = cpu->v[0] + c8_instruction_get_nnn(cpu->instruction);
    return 0;
}

static int c8_cpu_call(C8Cpu *cpu)
{
    if (c8_memory_stack_write(cpu->memory, cpu->sp + 1, cpu->pc) < 0) {
        return -1;
    }

    cpu->sp++;
    cpu->pc = c8_instruction_get_nnn(cpu->instruction);

    return 0;
}

static int c8_cpu_se_i8(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    return (cpu->v[x] == c8_instruction_get_kk(cpu->instruction)) ? 2 : 1;
}

static int c8_cpu_se_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);
    return (cpu->v[x] == cpu->v[y]) ? 2 : 1;
}

static int c8_cpu_sne_i8(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    return (cpu->v[x] != c8_instruction_get_kk(cpu->instruction)) ? 2 : 1;
}

static int c8_cpu_sne_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);
    return (cpu->v[x] != cpu->v[y]) ? 2 : 1;
}

static int c8_cpu_ld_reg_i8(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->v[x] = c8_instruction_get_kk(cpu->instruction);
    return 1;
}

static int c8_cpu_ld_reg_i12(C8Cpu *cpu)
{
    uint16_t addr = c8_instruction_get_nnn(cpu->instruction);
    cpu->i = addr;
    return 1;
}

static int c8_cpu_ld_reg_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[x] = cpu->v[y];
    return 1;
}

static int c8_cpu_ld_reg_dt(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->v[x] = cpu->dt;
    return 1;
}

static int c8_cpu_ld_dt_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->dt = cpu->v[x];
    return 1;
}

static int c8_cpu_ld_st_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->st = cpu->v[x];
    return 1;
}

static int c8_cpu_ld_reg_key(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    C8Key key = c8_keyboard_wait_for_press(cpu->keyboard);

    if (key == C8_KEY_NUM) {
        return 0;
    }

    cpu->v[x] = key;
    return 1;
}

static int c8_cpu_ld_reg_sprite(C8Cpu *cpu)
{
    const uint8_t sprite_size = 5;
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->i = sprite_size * cpu->v[x];
    return 1;
}

static int c8_cpu_ld_mem_bcd(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t value = cpu->v[x];

    if (c8_memory_write_i8(cpu->memory, cpu->i, value / 100) < 0) {
        return -1;
    }

    value %= 100;
    if (c8_memory_write_i8(cpu->memory, cpu->i + 1, value / 10) < 0) {
        return -1;
    }

    value %= 10;
    if (c8_memory_write_i8(cpu->memory, cpu->i + 2, value) < 0) {
        return -1;
    }

    return 1;
}

static int c8_cpu_ld_mem_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);

    if (c8_memory_write(cpu->memory, cpu->i, cpu->v, x + 1) < 0) {
        return -1;
    }

    return 1;
}

static int c8_cpu_ld_reg_mem(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);

    if (c8_memory_read(cpu->memory, cpu->i, cpu->v, x + 1) < 0) {
        return -1;
    }

    return 1;
}

static int c8_cpu_add_i8(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->v[x] += c8_instruction_get_kk(cpu->instruction);
    return 1;
}

static int c8_cpu_add_i12(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->i += cpu->v[x];
    return 1;
}

static int c8_cpu_add_reg(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[0xf] = cpu->v[x] > (UINT8_MAX - cpu->v[y]);
    cpu->v[x] += cpu->v[y];

    return 1;
}

static int c8_cpu_sub(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[0xf] = cpu->v[x] > cpu->v[y];
    cpu->v[x] -= cpu->v[y];

    return 1;
}

static int c8_cpu_subn(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[0xf] = cpu->v[y] > cpu->v[x];
    cpu->v[y] -= cpu->v[x];

    return 1;
}

static int c8_cpu_or(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[x] |= cpu->v[y];
    return 1;
}

static int c8_cpu_and(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[x] &= cpu->v[y];
    return 1;
}

static int c8_cpu_xor(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);

    cpu->v[x] ^= cpu->v[y];
    return 1;
}

static int c8_cpu_shr(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);

    cpu->v[0xf] = cpu->v[x] & 0x01;
    cpu->v[x] >>= 1;

    return 1;
}

static int c8_cpu_shl(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);

    cpu->v[0xf] = cpu->v[x] & 0x80;
    cpu->v[x] <<= 1;

    return 1;
}

static int c8_cpu_rnd(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    cpu->v[x] <<= rand() & c8_instruction_get_kk(cpu->instruction);
    return 1;
}

static int c8_cpu_drw(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    uint8_t y = c8_instruction_get_y(cpu->instruction);
    uint8_t n = c8_instruction_get_n(cpu->instruction);
    uint8_t buf[n];

    if (c8_memory_read(cpu->memory, cpu->i, buf, n) < 0) {
        return -1;
    }

    cpu->v[0xf] = c8_memory_display_write(
        cpu->memory, cpu->v[x], cpu->v[y], buf, n);
    return 1;
}

static int c8_cpu_skp(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    return (c8_keyboard_is_key_pressed(cpu->keyboard, cpu->v[x])) ? 2 : 1;
}

static int c8_cpu_sknp(C8Cpu *cpu)
{
    uint8_t x = c8_instruction_get_x(cpu->instruction);
    return (c8_keyboard_is_key_pressed(cpu->keyboard, cpu->v[x])) ? 1 : 2;
}

static int c8_cpu_execute_instruction_internal(C8Cpu *cpu)
{
    switch (cpu->instruction >> 12) {
    case 0x0:
        if ((cpu->instruction >> 8) != 0) {
            /* Ignore SYS instruction */
            return 0;
        }

        switch (cpu->instruction & 0x0ff) {
        case 0xe0:
            return c8_cpu_cls(cpu);

        case 0xee:
            return c8_cpu_ret(cpu);

        default:
            return -1;
        }

    case 0x1:
        return c8_cpu_jp_i12(cpu);

    case 0x2:
        return c8_cpu_call(cpu);

    case 0x3:
        return c8_cpu_se_i8(cpu);

    case 0x4:
        return c8_cpu_sne_i8(cpu);

    case 0x5:
        if ((cpu->instruction & 0x00f) != 0) {
            return -1;
        }
        return c8_cpu_se_reg(cpu);

    case 0x6:
        return c8_cpu_ld_reg_i8(cpu);

    case 0x7:
        return c8_cpu_add_i8(cpu);

    case 0x8:
        switch (cpu->instruction & 0x00f) {
        case 0x0:
            return c8_cpu_ld_reg_reg(cpu);

        case 0x1:
            return c8_cpu_or(cpu);

        case 0x2:
            return c8_cpu_and(cpu);

        case 0x3:
            return c8_cpu_xor(cpu);

        case 0x4:
            return c8_cpu_add_reg(cpu);

        case 0x5:
            return c8_cpu_sub(cpu);

        case 0x6:
            return c8_cpu_shr(cpu);

        case 0x7:
            return c8_cpu_subn(cpu);

        case 0xe:
            return c8_cpu_shl(cpu);

        default:
            return -1;
        }

    case 0x9:
        return c8_cpu_sne_reg(cpu);

    case 0xa:
        return c8_cpu_ld_reg_i12(cpu);

    case 0xb:
        return c8_cpu_jp_reg_i12(cpu);

    case 0xc:
        return c8_cpu_rnd(cpu);

    case 0xd:
        return c8_cpu_drw(cpu);

    case 0xe:
        switch (cpu->instruction & 0x0ff) {
        case 0x9e:
            return c8_cpu_skp(cpu);

        case 0xa1:
            return c8_cpu_sknp(cpu);

        default:
            return -1;
        }

    case 0xf:
        switch (cpu->instruction & 0x0ff) {
        case 0x07:
            return c8_cpu_ld_reg_dt(cpu);

        case 0x0a:
            return c8_cpu_ld_reg_key(cpu);

        case 0x15:
            return c8_cpu_ld_dt_reg(cpu);

        case 0x18:
            return c8_cpu_ld_st_reg(cpu);

        case 0x1e:
            return c8_cpu_add_i12(cpu);

        case 0x29:
            return c8_cpu_ld_reg_sprite(cpu);

        case 0x33:
            return c8_cpu_ld_mem_bcd(cpu);

        case 0x55:
            return c8_cpu_ld_mem_reg(cpu);

        case 0x65:
            return c8_cpu_ld_reg_mem(cpu);
        }

    default:
        return -1;
    }
}

void c8_cpu_execute_instruction(C8Cpu *cpu)
{
    if (c8_memory_program_read(cpu->memory, cpu->pc, &cpu->instruction) < 0) {
        return;
    }

    int ret = c8_cpu_execute_instruction_internal(cpu);
    if (ret < 0) {
        fprintf(stderr, "cpu: bad instruction: 0x%04x\n", cpu->instruction);
        cpu->pc += C8_INSTRUCTION_SIZE;
    } else {
        cpu->pc += C8_INSTRUCTION_SIZE * ret;
    }
}

bool c8_display_updated(C8Cpu *cpu)
{
    return (cpu->instruction >> 12) == 0xd;
}

void c8_delay_timer_tick(C8Cpu *cpu)
{
    if (cpu->dt > 0) {
        cpu->dt--;
    }
}

void c8_sound_timer_tick(C8Cpu *cpu)
{
    if (cpu->st > 0) {
        c8_audio_play(cpu->audio);
        cpu->st--;
    }
}
