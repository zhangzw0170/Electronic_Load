#include "stm32f10x.h"                  // Device header
#include "../ELoad_config.h"

void LED_Init(void) {
	RCC_APB2PeriphClockCmd( LED_RCC, ENABLE );
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_KBIN_PIN | LED_CCOUT_PIN | LED_OVERU_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init( LED_GPIO_PORT, &GPIO_InitStructure );
	
	GPIO_SetBits( LED_GPIO_PORT, LED_KBIN_PIN | LED_CCOUT_PIN | LED_OVERU_PIN );
}
