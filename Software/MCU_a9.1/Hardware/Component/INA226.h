// 原文件信息：
//    FILE: INA226.h
//  AUTHOR: Rob Tillaart
// VERSION: 0.6.4
//    DATE: 2021-05-18
// PURPOSE: Arduino library for INA226 power sensor
//     URL: https://github.com/RobTillaart/INA226
//
//  Read the datasheet for the details
//  本文件基于 Arduino 中的 INA226.h 移植，
//  将尽可能保留源文件的内容，并翻译部分注释，以作对比学习

#ifndef __INA226_H
#define __INA226_H


#ifdef __cplusplus
extern "C" {
#endif


#include "stm32f10x.h"
#include "../ELoad_config.h"
#include <stdint.h>
#include <math.h>


// #define INA226_LIB_VERSION				(F("0.6.4"))
#define INA226_LIB_VERSION					(F("0.6.4-STM32F10x"))

//  通过 setAlertRegister() 设置
//  设置警报寄存器，地址 0x07
#define INA226_SHUNT_OVER_VOLTAGE			0x8000
#define INA226_SHUNT_UNDER_VOLTAGE			0x4000
#define INA226_BUS_OVER_VOLTAGE				0x2000
#define INA226_BUS_UNDER_VOLTAGE			0x1000
#define INA226_POWER_OVER_LIMIT				0x0800
#define INA226_CONVERSION_READY				0x0400


//  getAlertFlag() 的返回值
//  获得警报寄存器标志位，地址 0x07
#define INA226_ALERT_FUNCTION_FLAG			0x0010
#define INA226_CONVERSION_READY_FLAG		0x0008
#define INA226_MATH_OVERFLOW_FLAG			0x0004
#define INA226_ALERT_POLARITY_FLAG			0x0002
#define INA226_ALERT_LATCH_ENABLE_FLAG		0x0001


//	setMaxCurrentShunt() 的返回值: 0x8000 - 0x8003
//	设置最大采样电流是否成功的错误标志，地址
#define INA226_ERR_NONE						0x0000
#define INA226_ERR_SHUNTVOLTAGE_HIGH		0x8000
#define INA226_ERR_MAXCURRENT_LOW			0x8001
#define INA226_ERR_SHUNT_LOW				0x8002
#define INA226_ERR_NORMALIZE_FAILED			0x8003
#define INA226_ERR_I2C_TIMEOUT				0x8100
#define INA226_ERR_I2C_INVALID				0x8101	
#define	INA226_ERR_NULL_PTR					0x8102
#define INA226_ERR_ADDR_RANGE				0x8103


//  See issue #26
//  最小采样电阻阻值，推测为 INA226 实际工作时的采样电阻下限
#define INA226_MINIMAL_SHUNT_OHM			0.001f

//  INA226 的最大等待时间
//  推测：防止因 INA226 暂时性的工作错误导致全系统停止运行
//  查阅手册，总线时序图定义表中指出 FAST MODE 时
//  停止条件与开始条件之间的总线空闲时间 t_BUF 不小于 600 ms
//  若配置为 HIGH-SPEED MODE，其值可降至 160 ms
#define INA226_MAX_WAIT_MS					600   //  millis

//  INA226 的最大采样电压：±81.92 mV
//  该电压为 Abs( IN+ - IN- ) 的最大值
#define INA226_MAX_SHUNT_VOLTAGE			(81.92f / 1000)


//  for setAverage() and getAverage()
//  用于设置 INA226 每次采集数据时的采样次数
//  INA226 内部会先取平均再放到寄存器中
enum ina226_average_enum {
    INA226_1_SAMPLE     = 0,
    INA226_4_SAMPLES    = 1,
    INA226_16_SAMPLES   = 2,
    INA226_64_SAMPLES   = 3,
    INA226_128_SAMPLES  = 4,
    INA226_256_SAMPLES  = 5,
    INA226_512_SAMPLES  = 6,
    INA226_1024_SAMPLES = 7
};


//  for BVCT and SVCT conversion timing.
//  用于设置 INA226 的总线电压转换时间与分流电压转换时间
//  注意：这是两个不同的设定，并非改一次就能对两个电压同时生效
enum ina226_timing_enum {
    INA226_140_us  = 0,
    INA226_204_us  = 1,
    INA226_332_us  = 2,
    INA226_588_us  = 3,
    INA226_1100_us = 4,
    INA226_2100_us = 5,
    INA226_4200_us = 6,
    INA226_8300_us = 7
};

//  这两条语句还可以帮助省略 enum 关键字
typedef enum ina226_average_enum INA226_Average;
typedef enum ina226_timing_enum INA226_ConvTime;


typedef struct {
    I2C_TypeDef* 	I2Cx;        			// I2C 外设指针
    uint8_t 		address;        		// 设备地址
    float 			current_LSB;    		// 校准参数
    float 			shunt;					// 采样电阻阻值，单位：欧姆
    float 			maxCurrent;				// 最大电流
    float 			current_zero_offset;	// 电流的零偏参数（用于校准）
    uint16_t 		bus_V_scaling_e4;		// 总线电压放缩比例参数（？暂时不知道有什么用）
    int 			error;					// 错误码
} INA226_TypeDef;


//  address between 0x40 and 0x4F
//  INA226 可以通过将地址配置引脚 A0, A1 接到其他引脚上，
//  以修改自身作为 I2C 从机的地址，防止地址冲突
//  根据手册，INA226 地址的合法值为 0x40 ~ 0x4F
//  explicit INA226(const uint8_t address, TwoWire *wire = &Wire);
void INA226_Init(INA226_TypeDef* ina, I2C_TypeDef* I2Cx, uint8_t addr);
//	针对 STM32 加入的 I2C 初始化地址
void INA226_I2C_GPIO_Init(void);
void I2C1_Periph_Init(I2C_TypeDef *I2Cx, uint8_t addr, uint32_t clockspeed);

//  bool     begin();			// 检查 INA226 有没有正常连接（对外接口）
//  bool     isConnected();		// 检查 INA226 有没有正常连接（不对外）
//  uint8_t  getAddress();		// 获取 INA226 的 I2C 从机地址
uint8_t INA226_Begin(INA226_TypeDef* ina);
uint8_t INA226_IsConnected(INA226_TypeDef* ina);
uint8_t INA226_GetAddress(INA226_TypeDef* ina);


//  Core functions
//  核心测量函数
//  float    getBusVoltage();       //  Volt，总线电压
//  float    getShuntVoltage();     //  Volt，采样电压，用于计算电流
//  float    getCurrent();          //  Ampere，电流
//  float    getPower();            //  Watt，功率，根据电压与电流得出
float INA226_GetBusVoltage(INA226_TypeDef* ina);
float INA226_GetShuntVoltage(INA226_TypeDef* ina);
float INA226_GetCurrent(INA226_TypeDef* ina);
float INA226_GetPower(INA226_TypeDef* ina);

//  See #35
//  检查 INA226   的转换功能是否已就绪
//  推测：加入此功能的目的是防止 INA226 在转换时接收到来自主机的指令，
//        导致干扰其正常工作
//  bool     isConversionReady();   //  conversion ready flag is set.
//  bool     waitConversionReady(uint32_t timeout = INA226_MAX_WAIT_MS);
uint8_t INA226_IsConversionReady(INA226_TypeDef* ina);
uint8_t INA226_WaitConversionReady(INA226_TypeDef* ina, uint32_t timeout);


//  单位转换辅助函数
//  这部分并不是必须实现的，可根据需要自行实现
//  此处保留原有的函数接口，具体功能将移至 INA226.c
//  同时给予用户选择是否编译这部分方法的自由
#ifdef INA226_ENABLE_SCALE_HELPER
// 毫伏级辅助函数
float INA226_GetBusVoltage_mV(INA226_TypeDef* ina);
float INA226_GetShuntVoltage_mV(INA226_TypeDef* ina);
float INA226_GetCurrent_mA(INA226_TypeDef* ina);
float INA226_GetPower_mW(INA226_TypeDef* ina);
// 微伏级辅助函数
float INA226_GetBusVoltage_uV(INA226_TypeDef* ina);
float INA226_GetShuntVoltage_uV(INA226_TypeDef* ina);
float INA226_GetCurrent_uA(INA226_TypeDef* ina);
float INA226_GetPower_uW(INA226_TypeDef* ina);
#endif


//  Configuration
//  配置函数
//  bool     reset();														// 重置设定
//  bool     setAverage(uint8_t avg = INA226_1_SAMPLE);						// 设置平均采样次数
//  uint8_t  getAverage();													// 获取当前平均采样次数
//  bool     setBusVoltageConversionTime(uint8_t bvct = INA226_1100_us);	// 设置总线电压转换时间
//  uint8_t  getBusVoltageConversionTime();									// 获取当前总线电压转换时间
//  bool     setShuntVoltageConversionTime(uint8_t svct = INA226_1100_us);	// 设置采样电压转换时间
//  uint8_t  getShuntVoltageConversionTime();								// 获取当前总线电压转换时间
uint8_t INA226_Reset(INA226_TypeDef* ina);
uint8_t INA226_SetAverage(INA226_TypeDef* ina, INA226_Average avg);
uint8_t INA226_GetAverage(INA226_TypeDef* ina);
uint8_t INA226_SetBusVoltageConversionTime(INA226_TypeDef* ina, INA226_ConvTime bvct);
uint8_t INA226_GetBusVoltageConversionTime(INA226_TypeDef* ina);
uint8_t INA226_SetShuntVoltageConversionTime(INA226_TypeDef* ina, INA226_ConvTime svct);
uint8_t INA226_GetShuntVoltageConversionTime(INA226_TypeDef* ina);


//  Calibration
//  mandatory to set these values!
//  校准值设定函数：**必须设定这些值**！
//  datasheet limit == 81.92 mV;
//    to prevent math overflow 0.02 mV is subtracted.
//  为防止溢出，基于手册上的差分电压最大值 81.92，设定允许的最大电压为 81.9 mV
//  shunt * maxCurrent <= 81.9 mV otherwise returns INA226_ERR_SHUNTVOLTAGE_HIGH
//  maxCurrent >= 0.001           otherwise returns INA226_ERR_MAXCURRENT_LOW
//  shunt      >= 0.001           otherwise returns INA226_ERR_SHUNT_LOW
//  采样电阻与最大电流的积**不能超过** 81.9 mV，否则返回 INA226_ERR_SHUNTVOLTAGE_HIGH
//  最大电流设定**不能低于** 1 mA，否则返回 INA226_ERR_MAXCURRENT_LOW
//  采样电阻设定**不能低于 1 mOhm，否则返回 INA226_ERR_SHUNT_LOW
//  以上出现的报错标志可在本文件开头找到

//  int      setMaxCurrentShunt(float maxCurrent = 20.0, float shunt = 0.002, bool normalize = true);
//  设置最大电流与采样电阻
int INA226_SetMaxCurrentShunt(INA226_TypeDef* ina, float maxCurrent, float shunt, uint8_t normalize);

//  configure provides full user control, not requiring call to setMaxCurrentShunt(args) function
//  方法 configure() 提供了完整的用户级控制，因此不用再重复调用方法 INA226_SetMaxCurrentShunt()
//  int      configure(float shunt = 0.1, float current_LSB_mA = 0.1, float current_zero_offset_mA = 0, uint16_t bus_V_scaling_e4 = 10000);
//  通过检查 当前最低分辨率是否为 0 确认寄存器配置是否正常
//  bool     isCalibrated()     { return _current_LSB != 0.0; };
int INA226_Configure(INA226_TypeDef* ina, float shunt, float current_LSB_mA, float current_zero_offset_mA, uint16_t bus_V_scaling_e4);
uint8_t INA226_IsCalibrated(INA226_TypeDef* ina);


#ifdef INA226_ENABLE_CURRENT_LSB_HELPER
//	校准参数访问接口
//  These functions return zero if not calibrated!
//  若未校准，则这些函数返回 0
float INA226_GetCurrentLSB(INA226_TypeDef* ina);          			// 单位：A/bit
float INA226_GetCurrentLSB_mA(INA226_TypeDef* ina);			     	// 单位：mA/bit
float INA226_GetCurrentLSB_uA(INA226_TypeDef* ina);       			// 单位：μA/bit
float INA226_GetShuntResistance(INA226_TypeDef* ina);     			// 单位：Ω
float INA226_GetMaxCurrent(INA226_TypeDef* ina);          			// 单位：A
#endif

//  Operating mode
//  工作模式设定 / 获取
//  bool     setMode(uint8_t mode = 7);  //  default ModeShuntBusContinuous
//  uint8_t  getMode();
uint8_t INA226_SetMode(INA226_TypeDef* ina, uint8_t mode);
uint8_t INA226_GetMode(INA226_TypeDef* ina);


#ifdef INA226_ENABLE_OPERATING_MODE_HELPER
//  工作模式设定 / 获取 辅助函数
//  bool     shutDown()                  { return setMode(0); };
//  bool     setModeShuntTrigger()       { return setMode(1); };
//  bool     setModeBusTrigger()         { return setMode(2); };
//  bool     setModeShuntBusTrigger()    { return setMode(3); };
//  bool     setModeShuntContinuous()    { return setMode(5); };
//  bool     setModeBusContinuous()      { return setMode(6); };
//  bool     setModeShuntBusContinuous() { return setMode(7); };  //  default.
uint8_t INA226_Shutdown(INA226_TypeDef* ina);
uint8_t INA226_SetModeShuntTrigger(INA226_TypeDef* ina);
uint8_t INA226_SetModeBusTrigger(INA226_TypeDef* ina);
uint8_t INA226_SetModeShuntBusTrigger(INA226_TypeDef* ina);
uint8_t INA226_SetModeShuntContinuous(INA226_TypeDef* ina);
uint8_t INA226_SetModeBusContinuous(INA226_TypeDef* ina);
uint8_t INA226_SetModeShuntBusContinuous(INA226_TypeDef* ina);
#endif

//  Alert
//  - separate functions per flag?
//  - what is a reasonable limit?
//  - which units to define a limit per mask ?
//    same as voltage registers ?
//  - how to test
//  警报标志
//  - 每个标志是否使用单独的函数处理?
//  - 什么是合理的限制?
//  - 每个掩码的限制应该使用哪种单位定义? 与电压寄存器相同吗?
//  - 如何测试？
//  bool     setAlertRegister(uint16_t mask);
//  uint16_t getAlertFlag();
//  bool     setAlertLimit(uint16_t limit);
//  uint16_t getAlertLimit();
uint8_t INA226_SetAlertRegister(INA226_TypeDef* ina, uint16_t mask);
uint16_t INA226_GetAlertFlag(INA226_TypeDef* ina);
uint8_t INA226_SetAlertLimit(INA226_TypeDef* ina, uint16_t limit);
uint16_t INA226_GetAlertLimit(INA226_TypeDef* ina);



//  Meta information
//  出厂数据：制造商代码，模具代码
//                               typical value 典型值
//  uint16_t getManufacturerID();  //  0x5449
//  uint16_t getDieID();           //  0x2260
uint16_t INA226_GetManufacturerID(INA226_TypeDef* ina);
uint16_t INA226_GetDieID(INA226_TypeDef* ina);


//  DEBUG
//  调试用方法
//  uint16_t getRegister(uint8_t reg)  { return _readRegister(reg); };
uint16_t INA226_GetRegister(INA226_TypeDef* ina, uint8_t reg);


//  ERROR HANDLING
//  错误处理：获取最近一个错误
//	int      getLastError();
int INA226_GetLastError(INA226_TypeDef* ina);


#ifdef __cplusplus
}
#endif

//  -- END OF FILE --
#endif
