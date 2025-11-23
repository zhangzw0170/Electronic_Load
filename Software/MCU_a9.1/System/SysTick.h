// sys_tick.h
#ifndef __SYS_TICK_H
#define __SYS_TICK_H
#include "stm32f10x.h"
#include <stdint.h>

extern volatile uint32_t sys_tick;

void SysTick_Init(uint32_t freq_hz);
uint32_t SysTick_GetTick(void);
void SysTick_Delay(uint32_t ms);

#endif
