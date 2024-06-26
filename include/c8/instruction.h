#ifndef C8_INSTRUCTION_H
#define C8_INSTRUCTION_H

#include <stdint.h>

#define C8_INSTRUCTION_SIZE 2

static inline uint8_t c8_instruction_get_x(uint16_t instruction)
{
    return (instruction & 0xf00) >> 8;
}

static inline uint8_t c8_instruction_get_y(uint16_t instruction)
{
    return (instruction & 0x0f0) >> 4;
}

static inline uint16_t c8_instruction_get_n(uint16_t instruction)
{
    return instruction & 0x00f;
}

static inline uint8_t c8_instruction_get_kk(uint16_t instruction)
{
    return instruction & 0x0ff;
}

static inline uint16_t c8_instruction_get_nnn(uint16_t instruction)
{
    return instruction & 0xfff;
}

#endif
