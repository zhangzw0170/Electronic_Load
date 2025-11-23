#include "stm32f10x.h"           
#include "../ELoad_config.h"
#include "OLED.h"
#include "OLED_Font.h"

#define OLED_W_SCL(x)			GPIO_WriteBit(GPIOB, OLED_GPIO_PIN_SCL, (BitAction)(x))
#define OLED_W_SDA(x)			GPIO_WriteBit(GPIOB, OLED_GPIO_PIN_SDA, (BitAction)(x))

/*引脚初始化*/
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(OLED_GPIO_RCC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	// I2C要求两条总线均为开漏输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = OLED_GPIO_PIN_SCL;
 	GPIO_Init(OLED_GPIO, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = OLED_GPIO_PIN_SDA;
 	GPIO_Init(OLED_GPIO, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x40);		//写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	#ifdef SSD1306
	OLED_WriteCommand(0x00 | (X & 0x0F));		//设置X位置低4位
	#endif
	#ifdef SH1106
	OLED_WriteCommand(0x00 | (X & 0x0F) + 2);		//设置X位置低4位
	#endif
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		#ifdef SH1106
		for(i = 0; i < 132; i++)
		#endif
		#ifdef SSD1306
		for(i = 0; i < 128; i++)
		#endif
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar_8x16(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示一个中文字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  idx 字库中对应字的索引位置
  * @retval 无
  */
void OLED_ShowChar_16x16(uint8_t Line, uint8_t Column, uint8_t idx)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		
	for (i = 0; i < 8; i++)
	{
	    OLED_WriteData(SP_F16x16[2*idx][i]);			//显示左上角
	}
	OLED_SetCursor((Line - 1) * 2, Column * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(SP_F16x16[2*idx][i + 8]);		//显示右上角
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(SP_F16x16[2*idx + 1][i]);		//显示左下角
	}
	OLED_SetCursor((Line - 1) * 2 + 1, Column * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(SP_F16x16[2*idx + 1][i + 8]);	//显示右下角
	}
}

/**
  * @brief  OLED显示反色字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar_8x16_Reverse(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData( ~(OLED_F8x16[Char - ' '][i]) );			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData( ~(OLED_F8x16[Char - ' '][i + 8]) );		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示一个中文字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  idx 字库中对应字的索引位置
  * @retval 无
  */
void OLED_ShowChar_16x16_Reverse(uint8_t Line, uint8_t Column, uint8_t idx)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		
	for (i = 0; i < 8; i++)
	{
	    OLED_WriteData( ~SP_F16x16[2*idx][i] );			//显示左上角
	}
	OLED_SetCursor((Line - 1) * 2, Column * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData( ~SP_F16x16[2*idx][i + 8] );		//显示右上角
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData( ~SP_F16x16[2*idx + 1][i] );		//显示左下角
	}
	OLED_SetCursor((Line - 1) * 2 + 1, Column * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData( ~SP_F16x16[2*idx + 1][i + 8] );	//显示右下角
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString_8x16(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar_8x16(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED显示中文字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  pidx 要显示的中文字符串，以数组中索引的形式提供，范围：字库
  * @retval 无
  */
void OLED_ShowString_16x16(uint8_t Line, uint8_t Column, uint8_t *pidx)
{
	uint8_t i;
	for (i = 0; pidx[i] != OLED_16x16_EMPTY; i++)
	{
		OLED_ShowChar_16x16(Line, Column + 2*i, pidx[i]);
	}
}

/**
  * @brief  OLED显示反色字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString_8x16_Reverse(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar_8x16_Reverse(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED显示中文字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  pidx 要显示的中文字符串，以数组中索引的形式提供，范围：字库
  * @retval 无
  */
void OLED_ShowString_16x16_Reverse(uint8_t Line, uint8_t Column, uint8_t *pidx)
{
	uint8_t i;
	for (i = 0; pidx[i] != OLED_16x16_EMPTY; i++)
	{
		OLED_ShowChar_16x16_Reverse(Line, Column + 2*i, pidx[i]);
	}
}

/**
  * @brief  OLED显示反色字符串，但可以指定一个字符正色显示
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString_8x16_Selected(uint8_t Line, uint8_t Column, char *String, uint8_t selected_col)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		if ( i == selected_col - 1 ) {
			OLED_ShowChar_8x16(Line, Column + i, String[i]);
			continue;
		}
		OLED_ShowChar_8x16_Reverse(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar_8x16(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}
/**
  * @brief  OLED 显示单个数字（十进制，浮点数）
  * @param  Line 最高位起始行位置，范围：1~4
  * @param  Column 最高位起始列位置，范围：1~16
  * @param  Number 要显示的浮点数
  * @param  Weight 要显示的位置权重
  * @retval 无
  */
void OLED_ShowFloatNum_Single(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Weight, uint8_t PointPos)
{
	// 只有权重大于 10^0 = 1 的数位才可能产生不需要显示的前导 0
	// 当这个位数可能因为前导零而不显示时， prefix0_mode == 1
	uint8_t prefix0_mode = ( Weight <= 0 );
	uint8_t digit = ( Number / OLED_Pow(10, Weight) % 10 );
	if (!( prefix0_mode || digit )) {
		// 仅当可能是前导零且digit确实是0时才不显示
		OLED_ShowChar_8x16( Line, Column, digit + '0' );
	}
}
/**
  * @brief  OLED 显示整个数字（十进制，浮点数）
  * @param  Line 最高位起始行位置，范围：1~4
  * @param  Column 最高位起始列位置，范围：1~16
  * @param  Number 要显示的浮点数（提前转换成整数）
  * @param  MaxWeight 最高权重位
  * @param	MinWeight 最低权重位
* @param	PointPos 小数点位置，以该位置后面为准
  * @retval 无
  */
void OLED_ShowFloatNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t MaxWeight, uint8_t MinWeight, uint8_t PointPos)
{
	for (int weight = MaxWeight; weight >= MinWeight; weight++ ) {
		OLED_ShowFloatNum_Single( Line, Column, Number, weight, PointPos );
	}
}
/**
  * @brief  OLED显示反色数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowAntiNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar_8x16_Reverse(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar_8x16(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar_8x16(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar_8x16(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar_8x16(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar_8x16(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar_8x16(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化，SSD1306
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_I2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}
