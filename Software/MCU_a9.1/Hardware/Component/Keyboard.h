#ifndef __KEYBOARD_H
#define	__KEYBOARD_H

#include "ELoad.h"

// 键码映射
// 为了与 HX1838 的遥控器兼容，此处给出键码映射表
typedef enum {
	KB_KEY1 = 0x45,
	KB_KEY2 = 0x46,
	KB_KEY3 = 0x47,
	KB_KEY4 = 0x44,
	KB_KEY5 = 0x40,
	KB_KEY6 = 0x43,
	KB_KEY7 = 0x07,
	KB_KEY8 = 0x15,
	KB_KEY9 = 0x09,
	
	KB_KEYstar = 0x16,
	KB_KEY0 = 0x19,
	KB_KEYsharp = 0x0D,
	
	KB_KEYup = 0x18,
	KB_KEYleft = 0x08,
	KB_KEYok = 0x1C,
	KB_KEYright = 0x5A,
	KB_KEYdown = 0x52,
	
	KB_KEYA = 0x7A,
	KB_KEYB = 0x7B,
	KB_KEYC = 0x7C,
	KB_KEYD = 0x7D,
	
	KB_NONE = 0x00
} KB_KEY;

#ifdef KB_IT_ENABLE
// 键盘状态
typedef enum {
    KEY_STATE_IDLE,       // 空闲状态
    KEY_STATE_DEBOUNCE,   // 消抖状态
    KEY_STATE_SCAN,       // 扫描状态
    KEY_STATE_PRESSED,    // 按键已按下
    KEY_STATE_RELEASED    // 按键已释放
} Key_State;

void Keyboard_Init(void);
void Keyboard_Process(void);
KB_KEY Keyboard_GetKey(void);
void Keyboard_ClearKey(void);
#else
void Keyboard_Init( void );
KB_KEY Keyboard_Scan( void );
#endif

#endif
