#include "stm32f10x.h"
#include "Delay.h"
#include "ELoad.h"
#include "Keyboard.h"

void Keyboard_Init( void ) {
	// 启用时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 行线 PA0-PA3 配置为推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; // 根据需要修改
	
	GPIO_Init( GPIOA, &GPIO_InitStructure );
	
	// 列线 PA4-PA7 配置为上拉输入
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	
	GPIO_Init( GPIOA, &GPIO_InitStructure );
}

KB_KEY Keyboard_Scan(void) {
    KB_KEY key_val = KB_NONE;
	// 行掩码需要根据使用的引脚做出相应修改
    const uint16_t row_masks[] = { 0xFFFE, 0xFFFD, 0xFFFB, 0xFFF7 }; // 行掩码：R1-R4 依次拉低
    
	// 原理：利用循环轮询各行
    for (uint8_t row = 0; row < 4; row++) {
        // 设置当前行为低，其他行为高
        GPIO_Write(GPIOA, row_masks[row]);
        Delay_us(10); // 短暂延时稳定信号
        
        // 读取列线状态（高四位）
        uint16_t col_state = GPIO_ReadInputData(GPIOA) & 0x00F0;
        if (col_state != 0x00F0) { // 检测到列线有低电平
            // 消除按键抖动防止误触
			Delay_ms(20); 
            col_state = GPIO_ReadInputData(GPIOA) & 0x00F0;
			
            if (col_state != 0x00F0) {
                // 确定具体列
				GPIO_SetBits( GPIOC, GPIO_Pin_13 ); // PC13 用于指示是否有按键输入
				/*
				// 第 2 行第 3 列：23
                switch (col_state) {								// 0x00F0 = 0000 0000 1111 0000
                    case 0x00E0: key_val = row * 10 + 11; break; 	// 0x00E0 = 0000 0000 1110 0000
                    case 0x00D0: key_val = row * 10 + 12; break; 	// 0x00D0 = 0000 0000 1101 0000
                    case 0x00B0: key_val = row * 10 + 13; break; 	// 0x00B0 = 0000 0000 1011 0000
                    case 0x0070: key_val = row * 10 + 14; break; 	// 0x0070 = 0000 0000 0111 0000
                    default: break;
                }*/
				// 为与 HX1838 兼容，重新分配键码
				if ( row == 0 ) {
					if (col_state == 0x00E0) key_val = KB_KEY1;
					if (col_state == 0x00D0) key_val = KB_KEY2;
					if (col_state == 0x00B0) key_val = KB_KEY3;
					if (col_state == 0x0070) key_val = KB_KEYA;
				} else if ( row == 1 ) {
					if (col_state == 0x00E0) key_val = KB_KEY4;
					if (col_state == 0x00D0) key_val = KB_KEY5;
					if (col_state == 0x00B0) key_val = KB_KEY6;
					if (col_state == 0x0070) key_val = KB_KEYB;
				} else if ( row == 2 ) {
					if (col_state == 0x00E0) key_val = KB_KEY7;
					if (col_state == 0x00D0) key_val = KB_KEY8;
					if (col_state == 0x00B0) key_val = KB_KEY9;
					if (col_state == 0x0070) key_val = KB_KEYC;
				} else if ( row == 3 ) {
					if (col_state == 0x00E0) key_val = KB_KEYstar;
					if (col_state == 0x00D0) key_val = KB_KEY0;
					if (col_state == 0x00B0) key_val = KB_KEYsharp;
					if (col_state == 0x0070) key_val = KB_KEYD;
				} else {
					;
				}
				
                // 等待按键释放
                while ( (GPIO_ReadInputData(GPIOA) & 0x00F0) != 0x00F0 );
				GPIO_ResetBits( GPIOC, GPIO_Pin_13 );
            }
        }
        // 恢复所有行为高（避免干扰后续扫描）
        GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);
    }
    return key_val;
}
