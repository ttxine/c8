#include "c8/memory.h"

#include "c8/c8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C8_MEMORY_INTERPRETER_SIZE 0x200
#define C8_MEMORY_PROGRAM_SIZE 0x4a0
#define C8_MEMORY_STACK_SIZE 16

#define C8_MEMORY_INTERPRETER_BEGIN 0x000
#define C8_MEMORY_INTERPRETER_END \
    (C8_MEMORY_INTERPRETER_BEGIN + C8_MEMORY_INTERPRETER_SIZE)
#define C8_MEMORY_PROGRAM_BEGIN 0x200
#define C8_MEMORY_PROGRAM_END \
    (C8_MEMORY_PROGRAM_BEGIN + C8_MEMORY_PROGRAM_SIZE)

#define C8_DISPLAY_WIDTH_BYTES (C8_DISPLAY_WIDTH / 8)

static const uint8_t c8_font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80,
};

struct c8_memory {
    uint8_t interpreter[C8_MEMORY_INTERPRETER_SIZE];
    uint8_t program[C8_MEMORY_PROGRAM_SIZE];
    uint16_t stack[C8_MEMORY_STACK_SIZE];
    uint8_t display[C8_DISPLAY_HEIGHT][C8_DISPLAY_WIDTH_BYTES];
};

C8Memory *c8_memory_new(const void *program, uint16_t size)
{
    if (size > C8_MEMORY_PROGRAM_SIZE) {
        fprintf(stderr, "memory: program is too big\n");
        return NULL;
    }

    C8Memory *memory = calloc(1, sizeof(C8Memory));

    if (memory == NULL) {
        fprintf(stderr, "memory: can't allocate memory\n");
        return NULL;
    }

    memcpy(memory->interpreter, c8_font, sizeof(c8_font));
    memcpy(memory->program, program, size);

    return memory;
}

int c8_memory_program_read(C8Memory *memory, uint16_t pc, uint16_t *value)
{
    pc -= C8_MEMORY_PROGRAM_BEGIN;

    if (pc + 1 >= C8_MEMORY_PROGRAM_SIZE) {
        fprintf(stderr, "memory: trying to read from invalid address\n");
        return -1;
    }

    *value = (memory->program[pc] << 8) | memory->program[pc + 1];
    return 0;
}

uint16_t c8_memory_program_begin(void)
{
    return C8_MEMORY_PROGRAM_BEGIN;
}

int c8_memory_stack_read(C8Memory *memory, uint8_t sp, uint16_t *value)
{
    if (sp >= C8_MEMORY_STACK_SIZE) {
        fprintf(stderr, "memory: invalid stack pointer\n");
        return -1;
    }

    *value = memory->stack[sp];
    return 0;
}

int c8_memory_stack_write(C8Memory *memory, uint8_t sp, uint16_t value)
{
    if (sp >= C8_MEMORY_STACK_SIZE) {
        fprintf(stderr,
                "memory: maximum level of nested subroutines is exceeded\n");
        return -1;
    }

    memory->stack[sp] = value;
    return 0;
}

void c8_memory_display_clear(C8Memory *memory)
{
    memset(memory->display, 0, sizeof(memory->display));
}

void c8_memory_display_read(C8Memory *memory, uint8_t *buf)
{
    memcpy(buf, memory->display, sizeof(memory->display));
}

uint8_t c8_memory_display_write(C8Memory *memory, uint8_t x, uint8_t y,
                                uint8_t *buf, uint8_t n)
{
    uint8_t ret = 0;
    uint8_t low_byte_offset = x / 8 % C8_DISPLAY_WIDTH_BYTES;
    uint8_t high_byte_offset = (low_byte_offset + 1) % C8_DISPLAY_WIDTH_BYTES;
    uint8_t in_byte_offset = x % 8;

    for (uint8_t i = 0; i < n; i++) {
        uint8_t *row = memory->display[(y + i) % C8_DISPLAY_HEIGHT];
        uint8_t low_part = buf[i] >> in_byte_offset;
        uint8_t high_part = buf[i] << (8 - in_byte_offset);

        if (ret == 0 &&
            ((row[low_byte_offset] & low_part) > 0 ||
             (row[high_byte_offset] & high_part) > 0)) {
            ret = 1;
        }

        row[low_byte_offset] ^= low_part;
        row[high_byte_offset] ^= high_part;
    }

    return ret;
}

int c8_memory_read(C8Memory *memory, uint16_t addr, void *buf, uint16_t len)
{
    if (addr >= C8_MEMORY_INTERPRETER_BEGIN &&
        addr + len <= C8_MEMORY_INTERPRETER_END) {
        memcpy(buf, memory->interpreter + addr - C8_MEMORY_INTERPRETER_BEGIN,
               len);
        return 0;
    } else if (addr >= C8_MEMORY_PROGRAM_BEGIN &&
               addr + len <= C8_MEMORY_PROGRAM_END) {
        memcpy(buf, memory->program + addr - C8_MEMORY_PROGRAM_BEGIN, len);
        return 0;
    } else {
        fprintf(stderr, "memory: trying to read from invalid address\n");
        return -1;
    }
}

int c8_memory_write(C8Memory *memory, uint16_t addr, void *buf, uint16_t len)
{
    if (addr >= C8_MEMORY_PROGRAM_BEGIN &&
        addr + len <= C8_MEMORY_PROGRAM_END) {
        memcpy(memory->program + addr - C8_MEMORY_PROGRAM_BEGIN, buf, len);
        return 0;
    } else {
        fprintf(stderr, "memory: trying to write to invalid address\n");
        return -1;
    }
}

int c8_memory_write_i8(C8Memory *memory, uint16_t addr, uint8_t value)
{
    return c8_memory_write(memory, addr, &value, sizeof(value));
}

static long c8_rom_get_size(FILE *file)
{
    if (fseek(file, 0, SEEK_END) < 0) {
        return -1;
    }

    long file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) < 0) {
        return -1;
    }

    return file_size;
}

uint8_t *c8_rom_new(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "rom: can't open %s\n", path);
        return NULL;
    }

    long file_size = c8_rom_get_size(file);
    if (file_size < 0) {
        fprintf(stderr, "rom: can't get size of %s\n", path);
        fclose(file);
        return NULL;
    }

    uint8_t *rom = malloc(file_size);
    if (fread(rom, 1, file_size, file) != file_size) {
        fprintf(stderr, "rom: can't read from %s\n", path);
        fclose(file);
        return NULL;
    }

    *size = file_size;

    fclose(file);
    return rom;
}
