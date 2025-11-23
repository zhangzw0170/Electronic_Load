// sys_tick.c
#include "systick.h"

volatile uint32_t sys_tick = 0;

void SysTick_Init(uint32_t freq_hz) {
    uint32_t reload = (SystemCoreClock / freq_hz) - 1;
    if(reload > 0x00FFFFFF) {
        // ´íÎó´¦Àí
        while(1);
    }
    
    SysTick->LOAD = reload;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_TICKINT_Msk |
                   SysTick_CTRL_ENABLE_Msk;
    
    NVIC_SetPriority(SysTick_IRQn, 0);
}

uint32_t SysTick_GetTick(void) {
    return sys_tick;
}

void SysTick_Delay(uint32_t ms) {
    uint32_t start = SysTick_GetTick();
    while ((SysTick_GetTick() - start) < ms) { __NOP(); }
}
