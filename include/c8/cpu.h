#ifndef C8_CPU_H
#define C8_CPU_H

#include <stdbool.h>

#define C8_CPU_HZ 500

typedef struct c8_cpu C8Cpu;
typedef struct c8_memory C8Memory;
typedef struct c8_keyboard C8Keyboard;

C8Cpu *c8_cpu_new(C8Memory *memory, C8Keyboard *keyboard);
C8Cpu *c8_cpu_free(C8Cpu *cpu);
void c8_cpu_execute_instruction(C8Cpu *cpu);

bool c8_display_updated(C8Cpu *cpu);
void c8_delay_timer_tick(C8Cpu *cpu);
void c8_sound_timer_tick(C8Cpu *cpu);

#endif
