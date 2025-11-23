#include "../ELoad_config.h"
#include "MCP4725.h"
#include "Delay.h"
#include "OLED.h"
#include "Periph/Serial.h"

void MCP4725_I2C_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
		
    RCC_APB2PeriphClockCmd(MCP4725_PERIPH_GPIO, ENABLE);
	GPIO_InitStruct.GPIO_Pin = MCP4725_I2C_PIN_SCL | MCP4725_I2C_PIN_SDA;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MCP4725_GPIO, &GPIO_InitStruct);
}

void I2C2_Periph_Init(I2C_TypeDef* I2Cx, uint8_t addr, uint32_t clockspeed) {
	I2C_InitTypeDef I2C_InitStruct;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    
	I2C_DeInit(I2Cx);
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = addr;	// 7 位
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStruct.I2C_ClockSpeed = clockspeed;
	
    I2C_Init(I2Cx, &I2C_InitStruct);
    I2C_Cmd(I2Cx, ENABLE);
}

void MCP4725_Init(MCP4725_TypeDef* mcp, I2C_TypeDef* I2Cx, uint8_t address) {
    // 参数校验
    if (!mcp) {
		mcp->error = MCP4725_ERR_NULL_PTR;
		return;
	}
    if (!I2Cx) {
        mcp->error = MCP4725_ERR_I2C_INVALID;
		return;
    }
	
	mcp->I2Cx = I2Cx;
    mcp->addr = address << 1; // 转换为7位地址
	/*
	 * 结构体中存储的地址（即之后需要访问的地址）需要左移一位
	 * 但是 I2C 初始化中的那个地址 OwnAddress1 不需要
	 */
}

#define MCP4725_I2C_WAITEVENT_STOP_ENABLE        1
#define MCP4725_I2C_WAITEVENT_STOP_DISABLE       0
uint8_t MCP4725_I2C_WaitEvent(MCP4725_TypeDef* mcp, uint32_t event, uint8_t enable_stop)
{
    uint32_t Timeout;                                     
	Timeout = I2C_TIMEOUT_MS * 1000;					// 给定超时计数时间
	while (I2C_CheckEvent(mcp->I2Cx, event) != SUCCESS)	// 循环等待指定事件
	{
		Timeout --;										// 等待时，计数值自减
		if (Timeout == 0)								// 自减到0后，等待超时
		{
            mcp->error = MCP4725_ERR_I2C_TIMEOUT;
            if (enable_stop == MCP4725_I2C_WAITEVENT_STOP_ENABLE) {
                I2C_GenerateSTOP(mcp->I2Cx, ENABLE);
            }
			return 0;
		}
	}
    return 1;
}

// I2C发送数据函数
/*
static uint8_t I2C_Write(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *pData, uint8_t len) {
	// 等待总线空闲
    while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));
    
    // 发送起始条件
    I2C_GenerateSTART(I2Cx, ENABLE);
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
    
    // 发送设备地址
    I2C_Send7bitAddress(I2Cx, addr, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    // 发送数据
    for(uint8_t i=0; i<len; i++) {
        I2C_SendData(I2Cx, pData[i]);
        while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    
    // 发送停止条件
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return 1;
}*/


uint8_t MCP4725_IsConnected(MCP4725_TypeDef* mcp) {
	OLED_ShowString_8x16( 1, 1, " MCP4725 Check  " );
	
	MCP4725_FastDAC( mcp, 0, 0 );
	if ( mcp->error != MCP4725_ERR_NONE ) {
		Serial_SendString( "[Error] MCP4725 Init Failed \r\n" );
		OLED_ShowChar_16x16( 2, 1, 20 );
		OLED_ShowChar_16x16( 2, 15, 20 );
		OLED_ShowString_8x16( 2, 3, "   Failed   " );
		OLED_ShowString_8x16( 3, 1, "  Press  RESET  " );
		OLED_ShowString_8x16( 4, 1, "  Code :        " );
		OLED_ShowHexNum( 4, 10, mcp->error, 4 );
		// while(1) {}
		return 0;
	} else {
		// 验证通过
		Serial_SendString( "[Info] MCP4725 Init Complete \r\n" );
		OLED_ShowChar_16x16( 2, 1, 19 );
		OLED_ShowChar_16x16( 2, 15, 19 );
		OLED_ShowString_8x16_Reverse( 2, 3, "  Accepted  " );
		Delay_ms(300);
		return 1;
	}	
}

/**
  * @brief	MCP4725 快速模式转换 DAC
  * @param	mcp
  * @param	工作模式（正常/关断）work_mode
  * @param	待设定电压值（二进制格式） v_bin
  * @retval	None
  *
  */
void MCP4725_FastDAC(MCP4725_TypeDef* mcp, uint8_t work_mode, uint16_t v_bin)
// 根据手册，本方法仅在 f = 400 kHz (Fast mode) 有效
{
    mcp->error = MCP4725_ERR_NONE;

    // 发送START条件
    I2C_GenerateSTART(mcp->I2Cx, ENABLE);
    MCP4725_I2C_WaitEvent(mcp, I2C_EVENT_MASTER_MODE_SELECT, MCP4725_I2C_WAITEVENT_STOP_DISABLE);

    // Byte 1: 发送设备地址（写模式）
    I2C_Send7bitAddress(mcp->I2Cx, mcp->addr, I2C_Direction_Transmitter);
    MCP4725_I2C_WaitEvent(mcp, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, MCP4725_I2C_WAITEVENT_STOP_ENABLE);

	// 检查 work_mode 的高 6 位是否为 0
	// 因为一共只有 4 个模式，合法值只有 0 1 2 3
	// 若不为 0 则报错
	/* if ( ! (work_mode & 0xFC) ) { 
		I2C_GenerateSTOP(mcp->I2Cx, ENABLE);
		mcp->error = MCP4725_ERR_WORKMODE_INVALID;
		return;
	}
	*/
	
	// Byte 2 ~ 3: 0 0（快速模式标志位） PD1/0 D11~D0 
	// 表5-2：PD1~0 关断位
	// 0 0 正常 01 10 11 代表 1k, 100k, 500k 电阻接地，MCP4725 关断
	// D15, D14 为 0，不用调
	// D13, D12 为 work_mode，使用宏定义给出的 0bXX，注意 work_mode 的类型
	// D11 ~ D0 为 12 位寄存器设置，D11 ~ D8 在 Byte 2
	// D7 ~ D0 在 Byte 3	
	uint8_t databyte2 = (work_mode << 4) + ( (v_bin & 0x0F00) >> 8 );
	uint8_t databyte3 = v_bin & 0x00FF;                   
	// OLED_ShowHexNum( 4, 1, databyte2, 2 );
	// OLED_ShowHexNum( 4, 4, databyte3, 2 );
	
    // 发送 Byte 2
    I2C_SendData( mcp->I2Cx, databyte2 );
    MCP4725_I2C_WaitEvent(mcp, I2C_EVENT_MASTER_BYTE_TRANSMITTED, MCP4725_I2C_WAITEVENT_STOP_ENABLE);

    // 发送 Byte 3
    I2C_SendData( mcp->I2Cx, databyte3 );
    MCP4725_I2C_WaitEvent(mcp, I2C_EVENT_MASTER_BYTE_TRANSMITTED, MCP4725_I2C_WAITEVENT_STOP_ENABLE);

    I2C_GenerateSTOP(mcp->I2Cx, ENABLE);
}
/*
uint8_t MCP4725_SetVoltage(MCP4725_TypeDef* mcp, uint16_t output, uint8_t writeEEPROM) {
    uint8_t cmd = writeEEPROM ? MCP4725_CMD_WRITEDACEEPROM : MCP4725_CMD_WRITEDAC;
    uint8_t data[3];
    
    data[0] = cmd;
    data[1] = output >> 4;          // 高8位 (D11-D4)
    data[2] = (output & 0x0F) << 4; // 低4位移到高4位
    
    return I2C_Write(mcp->I2Cx, mcp->addr, data, 3);
}*/

void MCP4725_SetCurrent(MCP4725_TypeDef* mcp, uint16_t I_set) { 
	// 200 mOhm shunt: k = 0.002 kOhm = (2V - 0.2V) / (1000 mA - 100 mA)
	//  50 mOhm shunt: k = 0.0005 kOhm = (0.5V - 0.05V) / (1000 mA - 100 mA)
	uint16_t U_set = (uint16_t)( ( (I_set * MCP4725_RSHUNT * 0.01f ) / MCP4725_VOL_REF) * 4096);
	//
	// 粗调
	U_set = 1.01f * U_set + 5.0f;
	// 
	MCP4725_FastDAC( mcp, MCP4725_ON, U_set );
}
