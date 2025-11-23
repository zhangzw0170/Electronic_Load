#ifndef __MCP4725_H
#define __MCP4725_H

#include "stm32f10x.h"                  	// Device header

// MCP4725 I2C配置信息
#define MCP4725_CMD_WRITEDAC				0x40
#define MCP4725_CMD_WRITEDACEEPROM			0x60

// 正常 / 关断模式
// 表5-2：PD1~0 关断位
// 0 0 正常 01 10 11 代表 1k, 100k, 500k 电阻接地
// MCP4725 关断
#define MCP4725_ON							0
#define MCP4725_OFF_1K						1
#define MCP4725_OFF_100K					2
#define MCP4725_OFF_500K					3

// 错误码
#define MCP4725_ERR_NONE					0x0000
#define MCP4725_ERR_I2C_TIMEOUT				0x8100
#define MCP4725_ERR_I2C_INVALID				0x8101	
#define	MCP4725_ERR_NULL_PTR				0x8102
#define MCP4725_ERR_ADDR_RANGE				0x8103
#define MCP4725_ERR_WORKMODE_INVALID		0x8200

typedef struct {
    I2C_TypeDef* 	I2Cx;      	// 使用的I2C外设
	uint8_t 		addr;      	// I2C地址
	int 			error;		// 错误码
} MCP4725_TypeDef;

void MCP4725_I2C_GPIO_Init(void);
void I2C2_Periph_Init(I2C_TypeDef* I2Cx, uint8_t addr, uint32_t clockspeed);
// 初始化结构体
void MCP4725_Init(MCP4725_TypeDef* mcp, I2C_TypeDef* I2Cx, uint8_t address);
uint8_t MCP4725_IsConnected(MCP4725_TypeDef* mcp);
void MCP4725_FastDAC(MCP4725_TypeDef* mcp, uint8_t work_mode, uint16_t v_bin);

// 设置输出电压
// uint8_t MCP4725_SetVoltage(MCP4725_TypeDef* mcp, uint16_t output, uint8_t writeEEPROM);
void MCP4725_SetCurrent(MCP4725_TypeDef* mcp, uint16_t I_set);

#endif
