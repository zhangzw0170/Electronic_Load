#ifndef __ELOAD_CONFIG_H
#define __ELOAD_CONFIG_H

/* --- 工作参数 --- */
// 输出电流范围 mA
#define I_SET_MIN 					100
#define I_SET_MAX					1000
#define OVER_U 						18.0f	// 判断时的单位为 V
#define RSHUNT						0.050f	// Ohm
#define MCP4725_VOL_REF				4.096f	// V
#define INA226_CURRENT_LSB_mA		0.05f	// mA	
#define INA226_CURRENT_ZERO_ERROR	0.0f 	// mA
#define INA226_VBUS_SCALE			10003
#define CALIB_DELAY_MS				5000

#define AUTOTEST_STEP				(50)		// mA
#define LAR_DEFAULT_FULLLOAD		(1000)		// mA

/* --- 校准参数 --- */
// #define CALIB_U_K					(+0.037f)
// #define CALIB_U_B					(+4816200.0f)
#define CALIB_I_K					(+0.958680f)
#define CALIB_I_B					(+1.5f)
#define CALIB_R_LINE				(+2.48f)
// MCP4725 的输出校正在 ELoad.c 中

/* --- LED 灯 --- */
// C13 用于指示系统正常工作 / 键盘输入
// C14 用于指示是否正在恒流输出
// C15 用于指示是否过压
#define LED_GPIO_PORT 			GPIOC
#define LED_RCC 				RCC_APB2Periph_GPIOC
#define LED_KBIN_PIN			GPIO_Pin_13
#define	LED_CCOUT_PIN			GPIO_Pin_14
#define	LED_OVERU_PIN			GPIO_Pin_15

/* --- 过压保护 --- */
// A8 用于输出高电平以关断恒流电路
// 该支路存在问题
#define OVERU_GPIO_PORT 		GPIOA
#define OVERU_RCC				RCC_APB2Periph_GPIOA
#define OVERU_GPIO_PIN			GPIO_Pin_8

/* --- I2C 配置 --- */
#define I2C_TIMEOUT_MS			400
#define I2C_CLOCKSPEED			400000

/* --- INA226 工作参数 --- */
#define INA226_I2C				I2C1
#define INA226_ADDR 			0x40
#define INA226_I2C_CLOCKSPEED	I2C_CLOCKSPEED
#define INA226_RSHUNT			RSHUNT
#define	INA226_GPIO				GPIOB
#define INA226_PERIPH_GPIO		RCC_APB2Periph_GPIOB
#define INA226_PERIPH_I2C		RCC_APB1Periph_I2C1
#define INA226_I2C_PIN_SCL		GPIO_Pin_6
#define INA226_I2C_PIN_SDA		GPIO_Pin_7

/* --- INA226 头文件保留接口 --- */
#define INA226_ENABLE_CURRENT_LSB_HELPER
#define INA226_ENABLE_SCALE_HELPER
#define INA226_ENABLE_OPERATING_MODE_HELPER

/* --- MCP4725 工作参数 --- */
#define MCP4725_I2C				I2C2
#define MCP4725_ADDR			0x60
#define MCP4725_I2C_CLOCKSPEED	I2C_CLOCKSPEED
#define MCP4725_RSHUNT			RSHUNT
#define	MCP4725_GPIO			GPIOB
#define MCP4725_PERIPH_GPIO		RCC_APB2Periph_GPIOB
#define MCP4725_I2C_PIN_SCL		GPIO_Pin_10
#define MCP4725_I2C_PIN_SDA		GPIO_Pin_11

/* --- OLED 屏幕 --- */
#define OLED_GPIO				GPIOB
#define OLED_GPIO_RCC			RCC_APB2Periph_GPIOB
#define OLED_GPIO_PIN_SCL		GPIO_Pin_13
#define OLED_GPIO_PIN_SDA		GPIO_Pin_12

#endif
