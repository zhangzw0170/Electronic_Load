
// 原文件信息：
//    FILE: INA226.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.6.4
//    DATE: 2021-05-18
// PURPOSE: Arduino library for INA226 power sensor
//     URL: https://github.com/RobTillaart/INA226
//
//  Read the datasheet for the details
//  本文件基于 Arduino 中的 INA226.h 移植
//	为节省篇幅，实现文件 .c 不再保留原代码

#include "../ELoad_config.h"
#include "INA226.h"
#include "stm32f10x.h"
#include <math.h>
#include "SysTick.h"
#include "Delay.h"
#include "OLED.h"
#include "../Periph/Serial.h"

//  REGISTERS
//  INA226 的寄存器地址
#define INA226_CONFIGURATION              0x00
#define INA226_SHUNT_VOLTAGE              0x01
#define INA226_BUS_VOLTAGE                0x02
#define INA226_POWER                      0x03
#define INA226_CURRENT                    0x04
#define INA226_CALIBRATION                0x05
#define INA226_MASK_ENABLE                0x06
#define INA226_ALERT_LIMIT                0x07
#define INA226_MANUFACTURER               0xFE
#define INA226_DIE_ID                     0xFF


//  CONFIGURATION MASKS
//  配置用掩码
#define INA226_CONF_RESET_MASK            0x8000
#define INA226_CONF_AVERAGE_MASK          0x0E00
#define INA226_CONF_BUSVC_MASK            0x01C0
#define INA226_CONF_SHUNTVC_MASK          0x0038
#define INA226_CONF_MODE_MASK             0x0007


/*===========================================================================*/
// 私有函数声明
/*===========================================================================*/
static uint8_t INA226_I2C_WaitEvent(INA226_TypeDef* ina, uint32_t event, uint8_t enable_stop);
static uint16_t INA226_ReadReg(INA226_TypeDef* ina, uint8_t reg);
static void INA226_WriteReg(INA226_TypeDef* ina, uint8_t reg, uint16_t value);


/*===========================================================================*/
// I2C 底层驱动实现（私有，即不在 .h 中提供）
/*===========================================================================*/

/**
  * @brief  等待 I2C 事件发生（带超时）
  * @param  I2Cx: I2C外设
  * @param  event: 等待的事件
  * @param  timeout: 超时时间(ms)
  * @retval 0-超时 1-事件发生
  */
#define INA226_I2C_WAITEVENT_STOP_ENABLE        1
#define INA226_I2C_WAITEVENT_STOP_DISABLE       0
uint8_t INA226_I2C_WaitEvent(INA226_TypeDef* ina, uint32_t event, uint8_t enable_stop)
{
    uint32_t Timeout;                                     
	Timeout = I2C_TIMEOUT_MS * 1000;					// 给定超时计数时间
	while (I2C_CheckEvent(ina->I2Cx, event) != SUCCESS)	// 循环等待指定事件
	{
		Timeout --;										// 等待时，计数值自减
		if (Timeout == 0)								// 自减到0后，等待超时
		{
            ina->error = INA226_ERR_I2C_TIMEOUT;
            if (enable_stop == INA226_I2C_WAITEVENT_STOP_ENABLE) {
                I2C_GenerateSTOP(ina->I2Cx, ENABLE);
            }
			return 0;
		}
	}
    return 1;
}

/*===========================================================================*/
// INA226 寄存器读写实现（私有，即不在 .h 中提供）
/*===========================================================================*/

/**
  * @brief  读取寄存器
  * @param	ina: INA226 结构
  * @param	reg: 需要读取的寄存器
  */
static uint16_t INA226_ReadReg(INA226_TypeDef* ina, uint8_t reg)
{
    uint16_t value = 0;
    ina->error = INA226_ERR_NONE;

    // 发送START条件
    I2C_GenerateSTART(ina->I2Cx, ENABLE);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_MODE_SELECT, INA226_I2C_WAITEVENT_STOP_DISABLE);

    // 发送设备地址（写模式 Transmitter）
    I2C_Send7bitAddress(ina->I2Cx, ina->address, I2C_Direction_Transmitter);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 发送寄存器地址
    I2C_SendData(ina->I2Cx, reg);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_TRANSMITTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 重复START条件
    I2C_GenerateSTART(ina->I2Cx, ENABLE);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_MODE_SELECT, INA226_I2C_WAITEVENT_STOP_DISABLE);

    // 发送设备地址（读模式 Receiver）
    I2C_Send7bitAddress(ina->I2Cx, ina->address, I2C_Direction_Receiver);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 读取高字节（发送ACK）
    I2C_AcknowledgeConfig(ina->I2Cx, ENABLE);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_RECEIVED, INA226_I2C_WAITEVENT_STOP_ENABLE);
    value = I2C_ReceiveData(ina->I2Cx) << 8;

    // 读取低字节（发送NACK）
    I2C_AcknowledgeConfig(ina->I2Cx, DISABLE);
    I2C_GenerateSTOP(ina->I2Cx, ENABLE);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_RECEIVED, INA226_I2C_WAITEVENT_STOP_DISABLE);
    value |= I2C_ReceiveData(ina->I2Cx);

    // 恢复ACK使能
    I2C_AcknowledgeConfig(ina->I2Cx, ENABLE);
    return value;
}

/**
  * @brief  写入寄存器
  * @param	ina: INA226 结构体
  * @param	reg: 准备写入的寄存器
  * @param	value: 准备写入 reg 的值
  */
static void INA226_WriteReg(INA226_TypeDef* ina, uint8_t reg, uint16_t value)
{
    ina->error = INA226_ERR_NONE;

    // 发送START条件
    I2C_GenerateSTART(ina->I2Cx, ENABLE);
    uint8_t temp = INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_MODE_SELECT, INA226_I2C_WAITEVENT_STOP_DISABLE);

    // 发送设备地址（写模式）
    I2C_Send7bitAddress(ina->I2Cx, ina->address, I2C_Direction_Transmitter);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 发送寄存器地址
    I2C_SendData(ina->I2Cx, reg);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_TRANSMITTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 发送高字节
    I2C_SendData(ina->I2Cx, (value >> 8) & 0xFF);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_TRANSMITTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    // 发送低字节
    I2C_SendData(ina->I2Cx, value & 0xFF);
    INA226_I2C_WaitEvent(ina, I2C_EVENT_MASTER_BYTE_TRANSMITTED, INA226_I2C_WAITEVENT_STOP_ENABLE);

    I2C_GenerateSTOP(ina->I2Cx, ENABLE);
}


/*===========================================================================*/
// INA226 GPIO 初始化与外设初始化（私有）
/*===========================================================================*/
void INA226_I2C_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
		
    RCC_APB2PeriphClockCmd(INA226_PERIPH_GPIO, ENABLE);  
    GPIO_InitStruct.GPIO_Pin = INA226_I2C_PIN_SCL | INA226_I2C_PIN_SDA;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(INA226_GPIO, &GPIO_InitStruct);
}

void I2C1_Periph_Init(I2C_TypeDef* I2Cx, uint8_t addr, uint32_t clockspeed) {
	I2C_InitTypeDef I2C_InitStruct;
    RCC_APB1PeriphClockCmd(INA226_PERIPH_I2C, ENABLE);
    
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


/*===========================================================================*/
// 公有函数实现
/*===========================================================================*/


////////////////////////////////////////////////////////
//
//  构造函数
//
void INA226_Init(INA226_TypeDef* ina, I2C_TypeDef* I2Cx, uint8_t addr )
{
     // 参数校验
    if (!ina) {
		ina->error = INA226_ERR_NULL_PTR;
		return;
	}
    if (!I2Cx) {
        ina->error = INA226_ERR_I2C_INVALID;
		return;
    }
    
    // 地址有效性验证（0x40-0x4F）
    if ((addr < 0x40) || (addr > 0x4F)) {
        ina->error = INA226_ERR_ADDR_RANGE;
		return;
    }
	
	ina->I2Cx = I2Cx;
	ina->address = addr << 1; // STM32地址需要左移1位
    // 默认无校准值
    ina->current_LSB = 0.0f;
    ina->shunt = 0.0f;
    ina->maxCurrent = 0.0f;
    ina->current_zero_offset = 0.0f;
    ina->bus_V_scaling_e4 = 10000;
    ina->error = INA226_ERR_NONE;
}


uint8_t INA226_Begin(INA226_TypeDef* ina)
{
	if (! INA226_IsConnected(ina)) return 0; // 0 => false
	return (ina->error == INA226_ERR_NONE);
}


/*
//  本段原始代码保留，以验证其功能是否一致
bool INA226::isConnected()
{
  _wire->beginTransmission(_address);
  return ( _wire->endTransmission() == 0);
}
*/
uint8_t INA226_IsConnected(INA226_TypeDef* ina)
{
	//  通过检查能否读取到制造商寄存器的方式
	//  来确认是否正确连接到 INA226
	OLED_ShowString_8x16( 1, 1, "  INA226 Check  " );
	uint16_t manufactID = 0x0000;
	manufactID = INA226_ReadReg(ina, INA226_MANUFACTURER);
	OLED_ShowString_8x16( 3, 1, "Manu :    0x" );
	OLED_ShowHexNum( 3, 13, manufactID, 4 );
	
	uint16_t dieID = 0x0000;
	dieID = INA226_ReadReg(ina, INA226_DIE_ID);
	OLED_ShowString_8x16( 4, 1, "Die  :    0x" );
	OLED_ShowHexNum( 4, 13, dieID, 4 );
	if ( (manufactID != 0x5449) || (dieID != 0x2260) ) {
		Serial_SendString( "[Error] INA226 Init Failed \r\n" );
		OLED_ShowChar_16x16( 2, 1, 20 );
		OLED_ShowChar_16x16( 2, 15, 20 );
		OLED_ShowString_8x16( 2, 3, "   Failed   " );
		OLED_ShowString_8x16( 3, 1, "  Press  RESET  " );
		OLED_ShowString_8x16( 4, 1, "  Code :        " );
		OLED_ShowHexNum( 4, 10, ina->error, 4 );
		return 0;
	} else {
		// 验证通过
		Serial_SendString( "[Info] INA226 Init Complete \r\n" );
		OLED_ShowChar_16x16( 2, 1, 19 );
		OLED_ShowChar_16x16( 2, 15, 19 );
		OLED_ShowString_8x16_Reverse( 2, 3, "  Accepted  " );
		Delay_ms(300);	// 只有通过的时候需要延迟，没通过的时候在死循环里
		OLED_ShowChar_16x16( 2, 1, 2 );
		return 1;
	}
}


uint8_t INA226_GetAddress(INA226_TypeDef * ina) {
	return ina->address;
}


////////////////////////////////////////////////////////
//
//  核心函数
//
float INA226_GetBusVoltage(INA226_TypeDef* ina)
{
    uint16_t raw = INA226_ReadReg(ina, INA226_BUS_VOLTAGE);
    float voltage = raw * 1.25e-3f; // LSB=1.25mV
    if(ina->bus_V_scaling_e4 != 10000)
    {
        voltage *= ina->bus_V_scaling_e4 * 1.0e-4f;
    }
    return voltage;
}


float INA226_GetShuntVoltage(INA226_TypeDef* ina)
{
    int16_t raw = (int16_t)INA226_ReadReg(ina, INA226_SHUNT_VOLTAGE);
    return raw * 2.5e-6f; // LSB=2.5μV
}


float INA226_GetCurrent(INA226_TypeDef* ina)
{
    int16_t raw = (int16_t)INA226_ReadReg(ina, INA226_CURRENT);
    return (raw * ina->current_LSB) - ina->current_zero_offset;
}


float INA226_GetPower(INA226_TypeDef* ina)
{
    uint16_t raw = INA226_ReadReg(ina, INA226_POWER);
    return raw * 25.0f * ina->current_LSB; // P = 25 * Current_LSB * reading, fixed 25 Watt
}


uint8_t INA226_IsConversionReady(INA226_TypeDef* ina)
{
    uint16_t mask = INA226_ReadReg(ina, INA226_MASK_ENABLE);
    return (mask & INA226_CONVERSION_READY_FLAG) == INA226_CONVERSION_READY_FLAG;
}


/*
bool INA226::waitConversionReady(uint32_t timeout)
{
  uint32_t start = millis();
  while ( (millis() - start) <= timeout)
  {
    if (isConversionReady()) return true;
    delay(1);  //  implicit yield();
  }
  return false;
}
*/
uint8_t INA226_WaitConversionReady(INA226_TypeDef* ina, uint32_t timeout)
{
    uint32_t start = SysTick_GetTick();
    while( (SysTick_GetTick() - start ) <= timeout)
    {
        if(INA226_IsConversionReady(ina)) return 1;
        SysTick_Delay(1);
    }
    return 0;
}

#ifdef INA226_ENABLE_SCALE_HELPER
//  Scale helpers milli range
float    INA226_GetBusVoltage_mV(INA226_TypeDef* ina)			   	{ return INA226_GetBusVoltage(ina)   * 1e3; };
float    INA226_GetShuntVoltage_mV(INA226_TypeDef* ina)				{ return INA226_GetShuntVoltage(ina) * 1e3; };
float    INA226_GetCurrent_mA(INA226_TypeDef* ina)      			{ return INA226_GetCurrent(ina)      * 1e3; };
float    INA226_GetPower_mW(INA226_TypeDef* ina)        			{ return INA226_GetPower(ina)        * 1e3; };
//  Scale helpers micro range
float    INA226_GetBusVoltage_uV(INA226_TypeDef* ina)   			{ return INA226_GetBusVoltage(ina)   * 1e6; };
float    INA226_GetShuntVoltage_uV(INA226_TypeDef* ina) 			{ return INA226_GetShuntVoltage(ina) * 1e6; };
float    INA226_GetCurrent_uA(INA226_TypeDef* ina)      			{ return INA226_GetCurrent(ina)      * 1e6; };
float    INA226_GetPower_uW(INA226_TypeDef* ina)        			{ return INA226_GetPower(ina)        * 1e6; };
#endif


////////////////////////////////////////////////////////
//
//  配置函数
//
uint8_t INA226_Reset(INA226_TypeDef* ina)
{
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    mask |= INA226_CONF_RESET_MASK;
    INA226_WriteReg(ina, INA226_CONFIGURATION, mask);
    // 复位校准参数
    ina->current_LSB = 0.0f;
    ina->maxCurrent = 0.0f;
    ina->shunt = 0.0f;
    return (ina->error == INA226_ERR_NONE);
}


uint8_t INA226_SetAverage(INA226_TypeDef* ina, INA226_Average avg)
{
    if(avg > 7) return 0;
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    mask &= ~INA226_CONF_AVERAGE_MASK;
    mask |= (avg << 9);
    INA226_WriteReg(ina, INA226_CONFIGURATION, mask);
    return (ina->error == INA226_ERR_NONE);
}


uint8_t INA226_GetAverage(INA226_TypeDef* ina)
{
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    mask &= INA226_CONF_AVERAGE_MASK;
    return (INA226_Average)(mask >> 9);
}


uint8_t INA226_SetBusVoltageConversionTime(INA226_TypeDef* ina, INA226_ConvTime bvct)
{
    if(bvct > 7) 
    {
        ina->error = INA226_ERR_NORMALIZE_FAILED;
        return 0;
    }
    
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    mask &= ~INA226_CONF_BUSVC_MASK;
    mask |= (bvct << 6);
    INA226_WriteReg(ina, INA226_CONFIGURATION, mask);
    return (ina->error == INA226_ERR_NONE);
}


uint8_t INA226_GetBusVoltageConversionTime(INA226_TypeDef* ina)
{
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    return (INA226_ConvTime)(mask & INA226_CONF_BUSVC_MASK) >> 6;
}


uint8_t INA226_SetShuntVoltageConversionTime(INA226_TypeDef* ina, INA226_ConvTime svct)
{
    if(svct > 7) 
    {
        ina->error = INA226_ERR_NORMALIZE_FAILED;
        return 0;
    }
    
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    mask &= ~INA226_CONF_SHUNTVC_MASK;
    mask |= (svct << 3);
    INA226_WriteReg(ina, INA226_CONFIGURATION, mask);
    return (ina->error == INA226_ERR_NONE);
}


uint8_t INA226_GetShuntVoltageConversionTime(INA226_TypeDef* ina)
{
    uint16_t mask = INA226_ReadReg(ina, INA226_CONFIGURATION);
    return (mask & INA226_CONF_SHUNTVC_MASK) >> 3;
}


////////////////////////////////////////////////////////
//
//  校准函数
//
int INA226_Configure(INA226_TypeDef* ina, float shunt, float current_LSB_mA, 
                    float current_zero_offset_mA, uint16_t bus_V_scaling_e4)
{
    // 参数检查
    if(shunt < INA226_MINIMAL_SHUNT_OHM)
        return INA226_ERR_SHUNT_LOW;
    
    float maxCurrent = fminf((INA226_MAX_SHUNT_VOLTAGE / shunt), 
                          32768.0f * current_LSB_mA * 1e-3f);
    if(maxCurrent < 0.001f)
        return INA226_ERR_MAXCURRENT_LOW;

    // 更新参数
    ina->shunt = shunt;
    ina->current_LSB = current_LSB_mA * 1e-3f;
    ina->current_zero_offset = current_zero_offset_mA * 1e-3f;
    ina->bus_V_scaling_e4 = bus_V_scaling_e4;
    ina->maxCurrent = maxCurrent;

    // 计算校准值
    uint16_t cal = (uint16_t)roundf(0.00512f / (ina->current_LSB * ina->shunt));
    INA226_WriteReg(ina, INA226_CALIBRATION, cal);

    return (ina->error == INA226_ERR_NONE) ? INA226_ERR_NONE : INA226_ERR_NORMALIZE_FAILED;
}

#ifdef INA226_ENABLE_CURRENT_LSB_HELPER
float    INA226_GetCurrentLSB(INA226_TypeDef* ina)    				{ return ina->current_LSB;       };
float    INA226_GetCurrentLSB_mA(INA226_TypeDef* ina)			 	{ return ina->current_LSB * 1e3; };
float    INA226_GetCurrentLSB_uA(INA226_TypeDef* ina) 				{ return ina->current_LSB * 1e6; };
float    INA226_GetShuntResistance(INA226_TypeDef* ina)     		{ return ina->shunt;             };
float    INA226_GetMaxCurrent(INA226_TypeDef* ina)    				{ return ina->maxCurrent;        };
#endif


////////////////////////////////////////////////////////
//
//  操作模式
//
uint8_t INA226_SetMode(INA226_TypeDef* ina, uint8_t mode)
{
    if(mode > 7)
    {
        ina->error = INA226_ERR_NORMALIZE_FAILED;
        return 0;
    }
    
    uint16_t config = INA226_ReadReg(ina, INA226_CONFIGURATION);
    config &= ~INA226_CONF_MODE_MASK;
    config |= mode;
    INA226_WriteReg(ina, INA226_CONFIGURATION, config);
    return (ina->error == INA226_ERR_NONE);
}

uint8_t INA226_GetMode(INA226_TypeDef* ina)
{
    uint16_t mode = INA226_ReadReg(ina, INA226_CONFIGURATION);
    return (mode & INA226_CONF_MODE_MASK);
}

#ifdef INA226_ENABLE_OPERATING_MODE_HELPER
uint8_t INA226_Shutdown(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 0);
}

uint8_t INA226_SetModeShuntTrigger(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 1);
}

uint8_t INA226_SetModeBusTrigger(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 2);
}

uint8_t INA226_SetModeShuntBusTrigger(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 3);
}

uint8_t INA226_SetModeShuntContinuous(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 5);
}

uint8_t INA226_SetModeBusContinuous(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 6);
}

uint8_t INA226_SetModeShuntBusContinuous(INA226_TypeDef* ina) {
    return INA226_SetMode(ina, 7);
}
#endif



////////////////////////////////////////////////////////
//
//  警报相关函数
//
uint8_t INA226_SetAlertRegister(INA226_TypeDef* ina, uint16_t mask)
{
    // 只允许写入高10位（0xFC00）
    INA226_WriteReg(ina, INA226_MASK_ENABLE, (mask & 0xFC00));
    return (ina->error == INA226_ERR_NONE);
}

uint16_t INA226_GetAlertFlag(INA226_TypeDef* ina)
{
    return INA226_ReadReg(ina, INA226_MASK_ENABLE) & 0x001F;
}

uint8_t INA226_SetAlertLimit(INA226_TypeDef* ina, uint16_t limit)
{
    INA226_WriteReg(ina, INA226_ALERT_LIMIT, limit);
    return (ina->error == INA226_ERR_NONE);
}

uint16_t INA226_GetAlertLimit(INA226_TypeDef* ina)
{
    return INA226_ReadReg(ina, INA226_ALERT_LIMIT);
}



////////////////////////////////////////////////////////
//
//  出厂数据
//
uint16_t INA226_GetManufacturerID(INA226_TypeDef* ina)
{
    return INA226_ReadReg(ina, INA226_MANUFACTURER);
}

uint16_t INA226_GetDieID(INA226_TypeDef* ina)
{
    return INA226_ReadReg(ina, INA226_DIE_ID);
}



////////////////////////////////////////////////////////
//
//  错误处理
//
int INA226_GetLastError(INA226_TypeDef* ina)
{
    int err = ina->error;
    ina->error = INA226_ERR_NONE; // 清除错误标志
    return err;
}
//  -- END OF FILE --
