#include "stm32f10x.h"                  // Device header
#include "../ELoad_config.h"

// 高电平时恒流电路关闭

void OverU(void) {
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// 禁止 TIM1 占用 PA8
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, DISABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, DISABLE);
	
	RCC_APB2PeriphClockCmd( OVERU_RCC, ENABLE );
	
	// PA15, PB3, PB4 用作 JTAG 调试接口，需要重定义
	// 这个 bug 找了一个早上
	// GPIO_PinRemapConfig( GPIO_Remap_SWJ_NoJTRST, ENABLE );
	
    GPIO_InitStruct.GPIO_Pin = OVERU_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( OVERU_GPIO_PORT, &GPIO_InitStruct );

	GPIO_ResetBits( OVERU_GPIO_PORT, OVERU_GPIO_PIN );
	GPIO_ResetBits( LED_GPIO_PORT, LED_OVERU_PIN );
}
