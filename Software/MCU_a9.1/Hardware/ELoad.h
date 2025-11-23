#ifndef __ELOAD_H
#define __ELOAD_H

#include "stm32f10x.h"                  // Device header
#include "ELoad_config.h"
#include "string.h"
#include "Delay.h"
#include "Systick.h"

#include "Component/OLED.h"
#include "Component/INA226.h"
#include "Component/MCP4725.h"
#include "Component/Keyboard.h"
#include "Periph/LED.h"
#include "Periph/OverU.h"
#include "Periph/Serial.h"

// 调试模式启用标志
#define DEBUG

// 最大输入位数
#define INPUT_VAL_MAX 	3

// 项目基本信息（将在信息页输出）
#ifdef DEBUG
#define SOFTWARE_VER 			"Alpha 9.1d"
#else
#define SOFTWARE_VER 			"Alpha 9.1 "
#endif
#define SOFTWARE_DESIGNER 		"Z.W. Zhang"
#define HARDWARE_DESIGNER 		"Y.Q. Chen "
#define LAST_UPDATE 			"2025-06-13"
// 字符标尺：					 123456789ABCDEF+
#define CTL_NAME				"  STM32F103C8T6 "
// 字符标尺：					 789ABCDEF+
#define ADC_NAME				"INA226 "
#define DAC_NAME				"MCP4725   "
#define MOS_NAME				"IRFP260N  "
															  
// 中文输出索引表
static uint8_t menu0_U[3] 							= {1, 5, OLED_16x16_EMPTY};
static uint8_t menu0_I[3] 							= {1, 6, OLED_16x16_EMPTY};
static uint8_t menu0_Set[3] 						= {10, 11, OLED_16x16_EMPTY};
static uint8_t menu0_L4[6] 							= {13, 14, 10, 11, OLED_16x16_EMPTY};
static uint8_t menu1_CCSet[7] 						= {12, 6, 13, 14, 10, 11, OLED_16x16_EMPTY};
static uint8_t menu_LAR[6] 							= {3, 4, 7, 8, 9, OLED_16x16_EMPTY};
static uint8_t menu1_About[3] 						= {37, 38, OLED_16x16_EMPTY};
static uint8_t menu1_L4[3] 							= {39, 40, OLED_16x16_EMPTY};
static uint8_t menu_CCOutput[6] 					= {12, 6, 13, 14, 15, OLED_16x16_EMPTY};      
static uint8_t menu2_ready[9] 						= {22, 22, 0, 42, 43, 0, 22, 22, OLED_16x16_EMPTY};
static uint8_t menu2_lastLAR[5] 					= {44, 45, 46, 47, OLED_16x16_EMPTY};
static uint8_t menu2_testing[9] 					= {18, 18, 29, 30, 48, 49, 18, 18, OLED_16x16_EMPTY};
static uint8_t menu3_softwaredesigner[6] 			= {31, 32, 33, 0, 0, OLED_16x16_EMPTY};
static uint8_t menu3_hardwaredesigner[6] 			= {34, 35, 36, 0, 0, OLED_16x16_EMPTY};
static uint8_t menu4_autotest[5] 					= {50, 51, 48, 52, OLED_16x16_EMPTY};
static uint8_t menu4_ready[3] 						= {42, 43, OLED_16x16_EMPTY};
static uint8_t menu4_now[3] 						= {53, 54, OLED_16x16_EMPTY};
static uint8_t menu4_testing[9] 					= {18, 18, 29, 30, 48, 49, 18, 18, OLED_16x16_EMPTY};
static uint8_t menu4_default[3]						= {55, 56, OLED_16x16_EMPTY};	

// EOK: 用于接受用户的确认标志
typedef enum {
	EOK_NONE = 0x00,
	EOK_ACCEPTED = 0xAC,
	EOK_FIRST_CONFIRM = 0x77,
	EOK_DENIED = 0xDE,
	
	#ifdef DEBUG
	EOK_DBG_ENABLE = 0xAC,
	EOK_DBG_DISABLE = 0xDE,
	#endif
} EOK;

// BOOL: 自定义布尔类型
typedef enum {
	FALSE = 0,
	TRUE = (!FALSE),
} BOOL;

typedef struct {
	// is_inited:
	// 是否已初始化
	BOOL is_inited;
	
	// lastKeyCode: 
	// 储存最近一次输入的键码
	uint8_t lastKeyCode;
	
	// is_output_work: 
	// 是否正在使用恒流输出
	BOOL is_output_work;
	BOOL lastis_output_work;
	
	// menu[]: 选择模式（状态机）
	// 0
	// 00: 留空，默认
	// 01：页面：初始化界面
	// 02：页面：设置 I 电压 U 电流 I 左右箭头 恒流输出标志
	// 03：功能：恒流输出设定，保留 02 的显示
	// 1
	// 11：菜单项：测量界面
	// 12：菜单项：负载率调整
	// 13：菜单项：关于
	// 2
	// 21：页面：负载率调整
	// 22：功能：启用负载率测试
	// 3
	// 31：页面：软件版本
	// 32：页面：元件参数
	// 4
	// 41：页面：自动测试就绪界面
	// 42：功能：自动测试（电流工作点扫描）
	uint8_t menu[2];		
	
	// debug_flag: 调试模式
	// 0xAC: 启用调试模式
	// 0xDE: 默认标记 Default
	#ifdef DEBUG
	EOK debug_flag;
	#endif
	
	// sel_digit: 选定需要修改的数位
	// 合法值: 0 1 2 3，其中 3 留空，防止溢出
	uint8_t sel_digit;
	
	// confirm: 电流输出 - 确认选择标志
	EOK confirm;
	
	// input_val: 按十进制数位存储每一位
	uint8_t input_val[ INPUT_VAL_MAX ];
	
	// I_set: 根据 input_val 计算得出的设置电流，单位：mA
	// lastI_set: 用于输入不合法时保留原有的电流设置
	// is_I_valid: 用于标识当前设置的电流是否合法
	uint16_t I_set;
	uint16_t lastI_set;
	uint16_t I_set_actual;
	BOOL is_I_valid;
	BOOL lastis_I_valid;
	
	// is_overu: 过压标志
	// 1: 过压
	BOOL is_overu;
	
	// INA226 采集的数据
	uint32_t U;		// uV
	uint32_t I;		// uA
	uint32_t lastU;
	uint32_t lastI;

	// 上一次测试的负载调整率
	float lastLAR;
} ELoad;

ELoad ELoad_Init(void);
void ELoad_Start( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );
void ELoad_CollectData( ELoad *pEL, INA226_TypeDef *ina );
void ELoad_Show_Normal( ELoad *pEL );
void ELoad_Serial_Receive( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );
void ELoad_Keyboard_To_Flag_Handle( ELoad *pEL );
void ELoad_Flag_To_Hardware_Handle( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );
void ELoad_PID_Update( ELoad *pEL, MCP4725_TypeDef *mcp );
void ELoad_CurrentUpdate( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );

#endif
