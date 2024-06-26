#ifndef C8_MEMORY_H
#define C8_MEMORY_H

#include <stdint.h>
#include <stddef.h>

typedef struct c8_memory C8Memory;

C8Memory *c8_memory_new(const void *program, uint16_t size);

int c8_memory_program_read(C8Memory *memory, uint16_t addr, uint16_t *value);
uint16_t c8_memory_program_begin(void);

int c8_memory_stack_read(C8Memory *memory, uint8_t sp, uint16_t *value);
int c8_memory_stack_write(C8Memory *memory, uint8_t sp, uint16_t value);

void c8_memory_display_clear(C8Memory *memory);
void c8_memory_display_read(C8Memory *memory, uint8_t *buf);
uint8_t c8_memory_display_write(C8Memory *memory, uint8_t x, uint8_t y,
                                uint8_t *buf, uint8_t n);

int c8_memory_read(C8Memory *memory, uint16_t addr, void *buf, uint16_t len);
int c8_memory_write(C8Memory *memory, uint16_t addr, void *buf, uint16_t len);
int c8_memory_write_i8(C8Memory *memory, uint16_t addr, uint8_t value);

uint8_t *c8_rom_new(const char *path, size_t *size);

#endif
