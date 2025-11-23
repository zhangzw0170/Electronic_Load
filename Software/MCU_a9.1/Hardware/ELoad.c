#include "ELoad.h"

#define MENU_1_MAX				3
#define MENU_3_MAX				2

// MCP4725 修正参数
const short correct_data[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	// 从这里开始是 100 mA
	1, 1, 1, 1, 1, 1, 2, 3, 2, 3,
	3, 5, 5, 5, 5, 5, 5, 4, 4, 4,
	3, 3, 2, 1, 2, 1, 1, 2, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 2, 4, 4, 4, 5, 3, 2, 1, 1,
	1, 3, 3, 2, 2, 4, 2, 4, 3, 4,
	4, 4, 2, 2, 3, 4, 4, 3, 2, 1,
	1, 4, 4, 1, 1, 1, 1, 0, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 3, 3, 3,
	5
};

/* --- 私有函数原型 --- */
void ELoad_Menu_JumpTo( ELoad *pEL, uint8_t m0, uint8_t m1 );					// 菜单跳转函数
void ELoad_CheckOverU( ELoad *pEL, MCP4725_TypeDef *mcp );						// 错误处理函数
void ELoad_RefreshOutputCurrent( ELoad *pEL, MCP4725_TypeDef *mcp );			// 刷新输出电流
void ELoad_LARTest( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );	// 负载调整率测试
void ELoad_AutoTest( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp );	// 自动校准数据采集
void ELoad_EnableCC( ELoad *pEL, MCP4725_TypeDef *mcp );
void ELoad_DisableCC( ELoad *pEL, MCP4725_TypeDef *mcp );

ELoad ELoad_Init(void) {
	ELoad EL;
	
	EL.is_inited = FALSE;
	EL.lastKeyCode = 0; 		// uint8_t
	EL.is_output_work = FALSE;	// BOOL
	EL.lastis_output_work = EL.is_output_work;
	EL.menu[0] = 0;				// uint8_t
	EL.menu[1] = 0;				// uint8_t
	#ifdef DEBUG
	EL.debug_flag = EOK_DBG_DISABLE;
	#endif
	EL.sel_digit = 0x01;
	EL.confirm = EOK_NONE;
	EL.input_val[0] = 0;
	EL.input_val[1] = 0;
	EL.input_val[2] = 0;
	EL.I_set = 0;
	EL.lastI_set = 0;
	EL.I_set_actual = 0;
	EL.is_I_valid = FALSE;
	EL.lastis_I_valid = EL.is_I_valid;
	EL.is_overu = FALSE;
	EL.U = 0;
	EL.I = 0;
	EL.lastU = 0;
	EL.lastI = 0;
	EL.lastLAR = -1.0f;
	return EL;
}

void ELoad_Start( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp ) {
	// 初始化界面字符矩阵显示
	uint8_t start_display[4][9] = 
	{ 
		{21, 22, 22, 22, 22, 22, 22, 23, OLED_16x16_EMPTY},
		{28,  0,  1,  2,  3,  4,  0, 24, OLED_16x16_EMPTY},
		{28,  0,  0,  0,  0,  0,  0, 24, OLED_16x16_EMPTY},
		{27, 26, 26, 26, 26, 26, 26, 25, OLED_16x16_EMPTY},
	};
	
	// 其他模块初始化
	Serial_Init();
	Serial_SendString("[Startup] Electronic Load\r\n");
	Serial_SendString("[Startup] Version: ");
	Serial_SendString( SOFTWARE_VER );
	Serial_SendString("\r\n");
	ELoad_Menu_JumpTo( pEL, 0, 1 );
	Serial_SendString("[Info] System Init OK \r\n");
	OLED_Init();
	Serial_SendString("[Info] OLED Init OK \r\n");
	LED_Init();
	Keyboard_Init();
	// OverU_Init();
	
	// 开机界面展示
	for (int row = 1; row <= 4; row++) {
		OLED_ShowString_16x16( row, 1, start_display[row-1] );
	}
	OLED_ShowString_8x16( 3, 4, SOFTWARE_VER );
	Delay_ms(300);
	OLED_Clear();
	
	// INA226 初始化与配置
	Serial_SendString("[Info] INA226 Initializing... \r\n");
	do {
		I2C1_Periph_Init( INA226_I2C, INA226_ADDR, INA226_I2C_CLOCKSPEED );
		INA226_I2C_GPIO_Init();
		INA226_Init( ina, INA226_I2C, INA226_ADDR );		
		INA226_IsConnected( ina );
	} while (ina->error != INA226_ERR_NONE);
	OLED_Clear();
	
	// MCP4725 初始化
	Serial_SendString("[Info] MCP4725 Initializing... \r\n");
	do {
		I2C2_Periph_Init( MCP4725_I2C, MCP4725_ADDR, MCP4725_I2C_CLOCKSPEED );
		MCP4725_I2C_GPIO_Init();
		MCP4725_Init( mcp, MCP4725_I2C, MCP4725_ADDR );
		MCP4725_IsConnected( mcp );
	} while (mcp->error != MCP4725_ERR_NONE);
	OLED_Clear();
	
	// 配置 INA226 的参数:     采样电阻，电流最低位（分辨率），零偏电流，总线电压放缩系数
	if ( INA226_Configure( ina, INA226_RSHUNT, INA226_CURRENT_LSB_mA, INA226_CURRENT_ZERO_ERROR, INA226_VBUS_SCALE ) != INA226_ERR_NONE ) {
		// 字符标尺；		 		123456789ABCDEF+
		Serial_SendString( "[Error] INA226 Configure Failed \r\n" );
		OLED_ShowString_8x16_Reverse( 1, 1, "X Err: ADC Fail " );
		OLED_ShowString_8x16_Reverse( 2, 1, "Conf: " );
		OLED_ShowHexNum( 2, 7, ina->error, 4 );
		Delay_ms(300);
		OLED_Clear();
	}
	INA226_SetAverage( ina, INA226_256_SAMPLES );
	
	Serial_SendString("[Info] Init Complete \r\n");
	pEL->is_inited = TRUE;
	GPIO_ResetBits( LED_GPIO_PORT, LED_KBIN_PIN | LED_CCOUT_PIN | LED_OVERU_PIN );
	ELoad_Menu_JumpTo( pEL, 0, 2 );
}

void ELoad_CollectData( ELoad *pEL, INA226_TypeDef *ina ) {
	// 备份数据
	pEL->lastU = pEL->U;
	pEL->lastI = pEL->I;
	
	// 采集数据：获取 INA226 的电压与电流
	pEL->U = INA226_GetBusVoltage_uV( ina );
	pEL->I = INA226_GetCurrent_uA( ina );
	
	pEL->I = CALIB_I_K * pEL->I + CALIB_I_B;
	// pEL->U = CALIB_U_K * pEL->U + CALIB_U_B;
	pEL->U += pEL->I * CALIB_R_LINE;
	
	#ifdef DEBUG
	if ( pEL->debug_flag == EOK_DBG_ENABLE ) {
		Serial_SendString( "[Debug] U = " );
		Serial_SendNumber( pEL->U / 1000000, 2 );
		Serial_SendString( "." );
		Serial_SendNumber( pEL->U / 1000 % 1000, 3 );
		Serial_SendString( " " );
		Serial_SendNumber( pEL->U % 1000, 3 );
		Serial_SendString( " V    I = " );
		Serial_SendNumber( pEL->I / 1000000, 1 );
		Serial_SendString( " " );
		Serial_SendNumber( pEL->I / 1000 % 1000, 3 );
		Serial_SendString( "." );
		Serial_SendNumber( pEL->I % 1000, 3 );
		Serial_SendString( " mA \r\n" );
	}
	#endif
	if ( pEL->U >= OVER_U * 1000000 ) {
		Serial_SendString("[Error] !!! Voltage > 18V, stop CC output !!! \r\n");
		// OVER_U 单位为 V
		pEL->is_overu = TRUE;
	}
}

void ELoad_Show_Normal( ELoad *pEL ) {
	// OLED 界面长时显示
	// * 设计目标上，所有临时显示后面必定跟随 Delay 函数
	// 每次 while 都会更新数据
	
	#ifdef DEBUG
	if ( (pEL->menu[0] != 3) && (pEL->debug_flag == EOK_DBG_ENABLE) ) {
		OLED_ShowHexNum( 2, 1, (pEL->menu[0] << 4) + pEL->menu[1], 2 );
	}
	#endif
	
	// 02：页面：设置 I 电压 pEL->U 电流 I 左右箭头 恒流输出标志
	// 03：功能：恒流输出设定，保留 02 的显示
	if ( (pEL->menu[0] == 0) && 
			( (pEL->menu[1] == 2) || (pEL->menu[1] == 3) 
	) ) {
		
		#ifdef DEBUG
		if (pEL->debug_flag == EOK_DBG_ENABLE) {
			OLED_ShowNum( 2, 3, pEL->sel_digit, 1 );
			OLED_ShowNum( 3, 3, pEL->input_val[ pEL->sel_digit ], 1 );
			OLED_ShowNum( 2, 4, pEL->I_set, 3 );
			OLED_ShowNum( 3, 4, pEL->lastI_set, 3 );
		} else 
		#endif
		{
			// 显示 "电压 U"
			OLED_ShowString_16x16( 2, 1, menu0_U );
			OLED_ShowString_8x16( 2, 5, " U" );
			// 显示 "电流 I"
			OLED_ShowString_16x16( 3, 1, menu0_I );
			OLED_ShowString_8x16( 3, 5, " I" );
		}
		
		OLED_ShowString_8x16( 3, 15, "mA" );
		OLED_ShowString_8x16( 2, 15, " V" );
		// 显示 左右箭头
		OLED_ShowChar_16x16( 4,  1, 17 );
		OLED_ShowChar_16x16( 4, 15, 18 );
		
		// 将是否恒流输出的显示切换到这里
		if ( pEL->is_output_work == TRUE ) {
			OLED_ShowString_16x16_Reverse( 4, 4, menu_CCOutput );
			OLED_ShowChar_8x16( 4, 3, ' ' );
			OLED_ShowChar_8x16( 4, 14, ' ' );
		} else {
			OLED_ShowString_16x16( 4, 5, menu0_L4 );
			OLED_ShowString_8x16( 4, 3, "  " );
			OLED_ShowString_8x16( 4, 13, "  " );
		}
		
		// 测量数据
		// 这里需要隐去最后一位，但开发阶段还不需要
		if ( pEL->U > 10000000 ) {
			OLED_ShowChar_8x16( 2, 7 , pEL->U / 10000000 % 10 + '0' );
		} else {
			OLED_ShowChar_8x16( 2, 7 , ' ' );
		}
		
		OLED_ShowChar_8x16( 2, 8 , pEL->U / 1000000  % 10 + '0' );
		OLED_ShowChar_8x16( 2, 9 , '.' );
		OLED_ShowChar_8x16( 2, 10, pEL->U / 100000   % 10 + '0' );
		OLED_ShowChar_8x16( 2, 11, pEL->U / 10000    % 10 + '0' );
		OLED_ShowChar_8x16( 2, 12, pEL->U / 1000     % 10 + '0' );
		OLED_ShowChar_8x16( 2, 13, pEL->U / 100      % 10 + '0' );
		
		if ( pEL->I > 1000000 )
			OLED_ShowChar_8x16( 3, 8 , pEL->I / 1000000  % 10 + '0' );
		else
			OLED_ShowChar_8x16( 3, 8 , ' ' );
		
		if ( pEL->I > 100000 )
			OLED_ShowChar_8x16( 3, 9 , pEL->I / 100000   % 10 + '0' );
		else
			OLED_ShowChar_8x16( 3, 9 , ' ' );
		
		if ( pEL->I > 10000 ) 
			OLED_ShowChar_8x16( 3, 10, pEL->I / 10000	 % 10 + '0' );
		else
			OLED_ShowChar_8x16( 3, 10, ' ' );
		
		OLED_ShowChar_8x16( 3, 11, pEL->I / 1000 	 % 10 + '0' );
		OLED_ShowChar_8x16( 3, 12, '.' );
		OLED_ShowChar_8x16( 3, 13, pEL->I / 100  	 % 10 + '0' );
		
		/*
		OLED_ShowFloatNum( 2, 8, pEL->U, 7, 2, 6 );
		OLED_ShowFloatNum( 3, 8, pEL->I, 6, 2, 3 );
		*/
		
		// 恒流设定信息提示
		if (pEL->menu[1] == 2) {
			// 02: 采集模式正色显示
			OLED_ShowString_16x16( 1, 1, menu0_Set );
			OLED_ShowString_8x16( 1, 5, " I = " );
			OLED_ShowNum( 1, 10, pEL->I_set, 4 );
			OLED_ShowString_8x16( 1, 14, " mA" );
		} else if (pEL->menu[1] == 3) {
			// 03: 设置模式反色显示
			static char menu0_input[ INPUT_VAL_MAX + 2 ] = "0000";			
			for (int i = 0; i < INPUT_VAL_MAX; i++) 
				menu0_input[i] = pEL->input_val[i] + '0';
			OLED_ShowString_16x16_Reverse( 1, 1, menu0_Set );
			OLED_ShowString_8x16_Reverse( 1, 5, " I " );
			if ( pEL->confirm == EOK_FIRST_CONFIRM )
				OLED_ShowChar_8x16( 1, 8, '?' );
			else
				OLED_ShowChar_8x16_Reverse( 1, 8, '=' );
			OLED_ShowChar_8x16_Reverse( 1, 9, ' ' );
			OLED_ShowString_8x16_Selected( 1, 10, menu0_input, 1+pEL->sel_digit );
			OLED_ShowString_8x16_Reverse( 1, 14, " mA" );
			
		}
		return;    
	}

	// 11：菜单项：测量界面
	// 12：菜单项：负载率调整
	// 13：菜单项：关于
	if ( pEL->menu[0] == 1 ) {
		OLED_ShowString_16x16( 1, 3, menu1_CCSet );
		OLED_ShowString_16x16( 2, 3, menu_LAR );
		OLED_ShowString_16x16( 3, 3, menu1_About );
		uint8_t menu1_arrow_pos = 1;
		#ifdef DEBUG
		if ( pEL->debug_flag == EOK_DBG_ENABLE ) {
			menu1_arrow_pos = 16;
		} else {
			menu1_arrow_pos = 1;
		}
		#endif
		switch (pEL->menu[1]) {
			case 1:
				OLED_ShowChar_8x16( 1, menu1_arrow_pos, '>' );
				OLED_ShowChar_8x16( 2, menu1_arrow_pos, ' ' );
				OLED_ShowChar_8x16( 3, menu1_arrow_pos, ' ' );
				break;
			case 2:
				OLED_ShowChar_8x16( 1, menu1_arrow_pos, ' ' );
				OLED_ShowChar_8x16( 2, menu1_arrow_pos, '>' );
				OLED_ShowChar_8x16( 3, menu1_arrow_pos, ' ' );
				break;
			case 3:
				OLED_ShowChar_8x16( 1, menu1_arrow_pos, ' ' );
				OLED_ShowChar_8x16( 2, menu1_arrow_pos, ' ' );
				OLED_ShowChar_8x16( 3, menu1_arrow_pos, '>' );
				break;
			default:
				break;
		}
		// 显示 左右箭头
		OLED_ShowChar_16x16( 4,  1, 17 );
		OLED_ShowChar_16x16( 4, 15, 18 );
		// 将是否恒流输出的显示切换到这里
		if ( pEL->is_output_work == TRUE ) {
			OLED_ShowString_16x16_Reverse( 4, 4, menu_CCOutput );
			OLED_ShowChar_8x16( 4, 3, ' ' );
			OLED_ShowChar_8x16( 4, 14, ' ' );
		} else {
			OLED_ShowString_16x16( 4, 7, menu1_L4 );
			OLED_ShowString_8x16( 4, 3, "    " );
			OLED_ShowString_8x16( 4, 11, "    " );
		}
	}
	
	// 21：页面：负载率调整
	// 22：功能：启用负载率测试
	if ( pEL->menu[0] == 2 ) {
		OLED_ShowString_16x16( 1, 4, menu_LAR );
		OLED_ShowString_16x16( 2, 1, menu2_ready );
		OLED_ShowString_16x16( 3, 1, menu2_lastLAR );
		uint32_t LAR_display = (uint32_t)(pEL->lastLAR * 10);
		if ( LAR_display / 100 > 0 ) {
			OLED_ShowChar_8x16( 3, 11, LAR_display / 100 % 10 + '0' );
		} else {
			OLED_ShowChar_8x16( 3, 11, ' ');
		}
		OLED_ShowChar_8x16( 3, 12, LAR_display / 10  % 10 + '0' );
		OLED_ShowChar_8x16( 3, 13, '.' );
		OLED_ShowChar_8x16( 3, 14, LAR_display       % 10 + '0' );
		/*
		OLED_ShowFloatNum( 3, 11, (uint32_t)(pEL->lastLAR * 10), 3, 1, 2 );
		*/
		OLED_ShowChar_8x16( 3, 16, '%' );
		
		OLED_ShowString_16x16( 4, 1, menu0_Set );
		OLED_ShowString_8x16( 4, 5, " I = " );
		OLED_ShowNum( 4, 10, pEL->I_set, 4 );
		OLED_ShowString_8x16( 4, 14, " mA" );
	}
	
	// 31：页面：软件版本
	// 32：页面：元件参数
	if ( pEL->menu[0] == 3 ) {
		// 关于(About)页面
		// 该模式下给出作者信息，最近更新时间与主要应用的器件
		if ( pEL->menu[1] == 1 ) {
			OLED_ShowString_8x16( 1, 1, "Ver : " );
			OLED_ShowString_8x16( 2, 1, "Sft : " );
			OLED_ShowString_8x16( 3, 1, "Hrd : " );
			OLED_ShowString_8x16( 4, 1, "Upd : " );
			OLED_ShowString_8x16( 1, 7, SOFTWARE_VER );
			// OLED_ShowString_8x16( 2, 7, SOFTWARE_DESIGNER );
			// OLED_ShowString_8x16( 3, 7, HARDWARE_DESIGNER );
			OLED_ShowString_16x16( 2, 7, menu3_softwaredesigner );
			OLED_ShowString_16x16( 3, 7, menu3_hardwaredesigner );
			OLED_ShowString_8x16( 4, 7, LAST_UPDATE );
		} else if ( pEL->menu[1] == 2 ) {
			OLED_ShowString_8x16( 1, 1, CTL_NAME );
			OLED_ShowString_8x16( 2, 1, "ADC : " );
			OLED_ShowString_8x16( 3, 1, "DAC : " );
			OLED_ShowString_8x16( 4, 1, "MOS : " );
			OLED_ShowString_8x16( 2, 7, ADC_NAME );
			OLED_ShowString_8x16( 3, 7, DAC_NAME );
			OLED_ShowString_8x16( 4, 7, MOS_NAME );
			#ifdef DEBUG
			if (pEL->debug_flag == EOK_DBG_ENABLE) {
				OLED_ShowString_8x16_Reverse( 2, 14, "Dbg" );
			} else
			#endif
			{
				OLED_ShowString_8x16( 2, 14, "   " );
			}
		}
	}
	
	// 41：页面：自动测试就绪
	// 42：页面：自动测试进行中
	if ( pEL->menu[0] == 4 ) {
		OLED_ShowString_16x16( 1, 1, menu4_ready );
		OLED_ShowString_8x16( 1, 5, " I = " );
		OLED_ShowString_8x16( 1, 14, " mA" );
		OLED_ShowString_8x16( 2, 1, "U = " );
		OLED_ShowString_8x16( 2, 15, " V" );
		OLED_ShowString_8x16( 3, 1, "I = " );
		OLED_ShowString_8x16( 3, 15, "mA" );
		OLED_ShowString_16x16( 4, 5, menu4_autotest );
		// 显示 左右箭头
		OLED_ShowChar_16x16( 4,  1, 17 );
		OLED_ShowChar_16x16( 4, 15, 18 );
	}
}

void ELoad_Keyboard_To_Flag_Handle( ELoad *pEL ) {
	// 按键事件处理
	// 除错误处理外，本方法只按照键码修改标记变量，不做其他操作
	// 该方法先获取键码，再对键码做对应处理
	// 不采用矩阵方案是为了将来和 HX1838 兼容
	// 键盘：		键码：
	// 1 2 3 A      45 46 47 7A
	// 4 5 6 B      44 40 43 7B
	// 7 8 9 C      07 15 09 7C
	// * 0 # D      16 19 0D 7D
	
	#ifdef DEBUG
	static uint8_t lastKeyCode_display = 0x00;
	if (pEL->debug_flag == EOK_DBG_ENABLE) {
		if (pEL->lastKeyCode) {
			lastKeyCode_display = pEL->lastKeyCode;
		}
		if ( pEL->menu[0] != 3 ) {
			OLED_ShowHexNum( 3, 1, lastKeyCode_display, 2 );
		}
	}
	#endif
	
	// 02: 数据采集
	if ( (pEL->menu[0] == 0) && (pEL->menu[1] == 2) ) {
		switch ( pEL->lastKeyCode ) {
			case KB_KEYA:
				// key = A：输出只有是或否两种模式
				// 这里用三目运算符会出 bug，推测是因为该运算符线程不安全
				if ( pEL->is_output_work == TRUE ) {
					pEL->is_output_work = FALSE;
				} else {
					pEL->is_output_work = TRUE;
				}
				break;
			case KB_KEYC:										
				// key = C：一级菜单移到上一项，02 -> 41
				// pEL->menu[1] = ( pEL->menu[1] + 1 ) % MENU_0_MAX;
				// 250613 更新：加入自动测试
				ELoad_Menu_JumpTo( pEL, 4, 1 );
				break;
			case KB_KEYD:										
				// key = D：菜单移到下一项，11 <- 02
				// pEL->menu[1] = ( pEL->menu[1] + MENU_0_MAX - 1 ) % MENU_0_MAX;
				ELoad_Menu_JumpTo( pEL, 1, 1 );
				break;
			case KB_KEYstar:
				// key = *: 取消电流设置，02 <- 03
				ELoad_Menu_JumpTo( pEL, 0, 2 );
				break;
			case KB_KEYsharp:
				// key = #: 确认进入电流设置，02 -> 03
				ELoad_Menu_JumpTo( pEL, 0, 3 );
				break;
			default:
				break;
		}
		// 标志位更新完成后清空键码，防止干扰下一轮循环
		pEL->lastKeyCode = 0;
		return;
	}
	
	// 03: 恒流设定
	if ( (pEL->menu[0] == 0) && (pEL->menu[1] == 3) ) {
		switch ( pEL->lastKeyCode ) {
			// 功能键 CD
			// 选定设置位
			case KB_KEYC:                                        
				// key = C：设定左移一位
				pEL->sel_digit = (pEL->sel_digit + INPUT_VAL_MAX - 1) % INPUT_VAL_MAX;
				break;
			case KB_KEYD:                                        
				// key = D：设定右移一位
				pEL->sel_digit = (pEL->sel_digit + 1) % INPUT_VAL_MAX;
				break;

			// 确认 # / 取消 *
			case KB_KEYstar: // *
				pEL->confirm = EOK_DENIED;
				break;
			case KB_KEYsharp: // #
				if (pEL->confirm == EOK_FIRST_CONFIRM) { 	// 第二次确认才给出真正的标志位
					pEL->confirm = EOK_ACCEPTED;
				} else {								// 第一次确认时给出"是否确认"标志
					pEL->confirm = EOK_FIRST_CONFIRM;
				}
				break;
				
			// 数字键 1234567890
			// 对应键码 11 12 13 21 22 23 31 32 33 42
			case KB_KEY1:
				pEL->input_val[ pEL->sel_digit ] = 1;
				break;
			case KB_KEY2:
				pEL->input_val[ pEL->sel_digit ] = 2;
				break;
			case KB_KEY3:
				pEL->input_val[ pEL->sel_digit ] = 3;
				break;
			case KB_KEY4:
				pEL->input_val[ pEL->sel_digit ] = 4;
				break;
			case KB_KEY5:
				pEL->input_val[ pEL->sel_digit ] = 5;
				break;
			case KB_KEY6:
				pEL->input_val[ pEL->sel_digit ] = 6;
				break;
			case KB_KEY7:
				pEL->input_val[ pEL->sel_digit ] = 7;
				break;
			case KB_KEY8:
				pEL->input_val[ pEL->sel_digit ] = 8;
				break;
			case KB_KEY9:
				pEL->input_val[ pEL->sel_digit ] = 9;
				break;
			case KB_KEY0:
				pEL->input_val[ pEL->sel_digit ] = 0;
				break;

			// 未指定的键码忽略
			default:
				break;
		}
		// 标志位更新完成后清空键码
		pEL->lastKeyCode = 0;
		return;
	}
	
	// 1X: 菜单页
	if ( pEL->menu[0] == 1 ) {
		switch ( pEL->lastKeyCode ) {
			case KB_KEYA: // 上
				ELoad_Menu_JumpTo( pEL, 1, (pEL->menu[1] + MENU_1_MAX - 2) % MENU_1_MAX + 1 );
				break;
			case KB_KEYB: // 下
				ELoad_Menu_JumpTo( pEL, 1, pEL->menu[1] % MENU_1_MAX + 1 );
				break;
			case KB_KEYC: // 左
				ELoad_Menu_JumpTo( pEL, 0, 2 );
				break;
			case KB_KEYD: // 右
				// 250613 更新：加入自动测试
				ELoad_Menu_JumpTo( pEL, 4, 1 );
				break;
			case KB_KEYsharp: // 确认          
				if ( pEL->menu[1] == 1 ) {	
					ELoad_Menu_JumpTo( pEL, 0, 2 );
				} else if ( pEL->menu[1] == 2 ) {
					ELoad_Menu_JumpTo( pEL, 2, 1 );
				} else if ( pEL->menu[1] == 3 ) {
					ELoad_Menu_JumpTo( pEL, 3, 1 );
				}
			default:
				break;
		}
		pEL->lastKeyCode = 0;
		return;
	}
	
	// 2X: 负载调整率测量
	if ( pEL->menu[0] == 2 ) { 
		switch ( pEL->lastKeyCode ) {
			// Key = B / #: 启动负载调整率测量
			case KB_KEYB:
			case KB_KEYsharp:
				// 确定后开始测量负载调整率
				ELoad_Menu_JumpTo( pEL, 2, 2 );
				break;
			case KB_KEYstar:
				// 退回到进入页
				ELoad_Menu_JumpTo( pEL, 1, 2 );
				break;
			default:
				break;
		}
		pEL->lastKeyCode = 0;
		return;
	}
	
	// 3X: 关于页面
	if ( pEL->menu[0] == 3 ) { 
		#ifdef DEBUG
		// 调试模式按键计数
		static uint8_t debugcnt = 0;
		#endif
		
		switch ( pEL->lastKeyCode ) {
			// 功能键 AB 切换不同页面 
			case KB_KEYC:										
				// key = C: 向左翻一页，若为第一页则退出
				if ( pEL->menu[1] <= 1 ) {
					ELoad_Menu_JumpTo( pEL, 0, 2 );
					break;
				}
				ELoad_Menu_JumpTo( pEL, 3, pEL->menu[1] - 1 );
				break;
			case KB_KEYD:                                        
				// key = D: 向右翻一页，若为最后一页则退出
				if ( pEL->menu[1] >= MENU_3_MAX ) {
					ELoad_Menu_JumpTo( pEL, 0, 2 );
					break;
				}
				ELoad_Menu_JumpTo( pEL, 3, pEL->menu[1] + 1 );
				break;
			// 取消 * 直接退出
			case KB_KEYstar:
				// key = *：直接退出
				#ifdef DEBUG
				debugcnt = 0;
				#endif
				ELoad_Menu_JumpTo( pEL, 0, 2 );	
				break;
			
			#ifdef DEBUG
			case KB_KEY1:
				// key = 1: 进入调试模式
				debugcnt = ( debugcnt + 1 ) % 6;
				if (debugcnt == 0) {
					pEL->debug_flag = EOK_DBG_DISABLE;
					Serial_SendString( "[Debug] Debug mode is disable \r\n" );
				} else if ( debugcnt == 5 ) {
					pEL->debug_flag = EOK_DBG_ENABLE;
					Serial_SendString( "[Debug] Debug mode is enable \r\n" );
				} else if ( debugcnt == 4 ) {
					Serial_SendString( "[Debug] One more step to enable debug mode \r\n" );
				}
				break;
			
			case KB_KEYB:
				// key = B: 自动校准模式
				ELoad_Menu_JumpTo( pEL, 4, 1 );
				break;
			#endif
			// 未指定的键码忽略
			default:
				break;
		}
		// 标志位更新完成后清空键码
		pEL->lastKeyCode = 0;
		return;
	}
	
	// 4X：自动测试模式
	if (pEL->menu[0] == 4) {
		// 41：就绪 42：开始测试
		switch ( pEL->lastKeyCode ) {
			case KB_KEYB:
			case KB_KEYsharp:
				ELoad_Menu_JumpTo( pEL, 4, 2 );
				break;
			case KB_KEYstar:
				ELoad_Menu_JumpTo( pEL, 0, 2 );
				break;
			case KB_KEYC:										
				ELoad_Menu_JumpTo( pEL, 1, 1 );
				break;
			case KB_KEYD:
				ELoad_Menu_JumpTo( pEL, 0, 2 );
			default:
				break;
		}
		pEL->lastKeyCode = 0;
		return;
	}
}

void ELoad_Flag_To_Hardware_Handle( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp ) {
	// EL 标志位事件处理
	
	// XX: 任何模式下都先检查是否过压
	ELoad_CheckOverU( pEL, mcp );
	
	ELoad_RefreshOutputCurrent( pEL, mcp );
	
	// 0X: 数据监控与恒流设定
	if ( pEL->menu[0] == 0 ) {
		if ( ( pEL->lastis_output_work != pEL->is_output_work )
			|| ( pEL->lastis_I_valid != pEL->is_I_valid ) 
		) {
			pEL->lastis_output_work = pEL->is_output_work;
			pEL->lastis_I_valid = pEL->is_I_valid;
														
			if ( ( pEL->is_output_work == TRUE ) && ( pEL->is_I_valid == TRUE ) )
				ELoad_EnableCC( pEL, mcp );
			else
				ELoad_DisableCC( pEL, mcp );
		}
	}
		
	// 22: 负载调整率测试
	// 该模式下 C14, C15 交替闪烁
	if ( (pEL->menu[0] == 2) && (pEL->menu[1] == 2) ) {
		ELoad_LARTest( pEL, ina, mcp );
	}
	
	// 42: 指标自动测试模式
	// 该模式下
	if ( (pEL->menu[0] == 4) && (pEL->menu[1] == 2) ) {
		ELoad_AutoTest( pEL, ina, mcp );
	}
}

void ELoad_Menu_JumpTo( ELoad *pEL, uint8_t m0, uint8_t m1 ) {
	if (pEL->menu[0] != m0) {
		OLED_Clear();
	}	
	pEL->menu[0] = m0;
	pEL->menu[1] = m1;
	Serial_SendString( "[Info] Menu: " );
	Serial_SendNumber( m0, 1 );
	Serial_SendNumber( m1, 1 );
	Serial_SendString( "\r\n" );
}

void ELoad_Serial_Receive( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp ) {
	if ( Serial_RxFlag == 1 ) {
		if ( strcmp( Serial_RxPacket, "test overu" ) == 0 ) {
			pEL->is_overu = TRUE;
		} else if ( strcmp( Serial_RxPacket, "test LAR" ) == 0 ) {
			ELoad_Menu_JumpTo( pEL, 2, 2 );
		} else if ( strcmp( Serial_RxPacket, "reset" ) == 0 ) {
			ELoad_Start( pEL, ina, mcp );
		// } else if ( strcmp( Serial_RxPacket, "reset" ) == 0 ) {
			// ELoad_
		#ifdef DEBUG
		} else if ( strcmp( Serial_RxPacket, "debug on" ) == 0 ) {
			pEL->debug_flag = EOK_DBG_ENABLE;
		} else if ( strcmp( Serial_RxPacket, "debug off" ) == 0 ) {
			pEL->debug_flag = EOK_DBG_DISABLE;
		} else if ( strcmp( Serial_RxPacket, "autotest" ) == 0 ) {
			// pEL->debug_flag = EOK_DBG_ENABLE;
			ELoad_Menu_JumpTo( pEL, 4, 2 );
		#endif
		
		} else {
			Serial_SendString( "[Error] Serial: Undefined command \r\n" );
		}
		Serial_RxFlag = 0;
	}
}

void ELoad_CheckOverU( ELoad *pEL, MCP4725_TypeDef *mcp ) {
	if ( pEL->is_overu == TRUE ) {
		// 过压检测		
		ELoad_DisableCC( pEL, mcp );	// 关闭恒流输出
		OverU();						// 启动恒流关断
		GPIO_SetBits( LED_GPIO_PORT, LED_OVERU_PIN );
		
		// 报错显示（此处设置死循环让用户手动重置）
		Serial_SendString( "[Error] INA226 Voltage > 18 V, CC stopping \r\n" );
		// 字符标尺；		 				 123456789ABCDEF+
		OLED_ShowString_8x16_Reverse( 1, 1, "X Vol over 18 V " );
		OLED_ShowString_8x16_Reverse( 3, 1, "  Close CC now  " );
		OLED_ShowString_8x16_Reverse( 4, 1, "  Press  RESET  " );
		while(1) { }
	}
}

void ELoad_EnableCC( ELoad *pEL, MCP4725_TypeDef *mcp ) {
	Serial_SendString("[Info] Output enable \r\n");
	GPIO_SetBits(LED_GPIO_PORT, LED_CCOUT_PIN);
	
	// 实际输出修正项
	pEL->I_set_actual = pEL->I_set - correct_data[ pEL->I_set / 10 ];
	#ifdef DEBUG
	// Serial_SendString("I_set_actual = ");
	// Serial_SendNumber( pEL->I_set_actual, 4 );
	// Serial_SendString(" mA \r\n");
	#endif
	MCP4725_SetCurrent(mcp, pEL->I_set_actual);
}

void ELoad_DisableCC( ELoad *pEL, MCP4725_TypeDef *mcp ) {
	Serial_SendString("[Info] Output disable \r\n");
	GPIO_ResetBits(LED_GPIO_PORT, LED_CCOUT_PIN);
	MCP4725_FastDAC(mcp, MCP4725_OFF_500K, 0);
}

void ELoad_RefreshOutputCurrent( ELoad *pEL, MCP4725_TypeDef *mcp ) {
	// 按照电流范围检查并调整电流输出
	// 一旦用户想修改电流，就马上先备份电流数据            
	pEL->lastI_set = pEL->I_set;
	
	// 第二次确认
	if ( pEL->confirm == EOK_ACCEPTED ) {
		pEL->I_set = pEL->input_val[0] * 1000 + pEL->input_val[1] * 100 + pEL->input_val[2] * 10;
		
		if ( ( (pEL->I_set >= I_SET_MIN) && (pEL->I_set <= I_SET_MAX) ) \
			|| (pEL->I_set == 0) ) {
			// 若电流合法则设置输出电流合法标志位为 1
			// 然后更新电流数据
			pEL->is_I_valid = TRUE;
			Serial_SendString( "[Info] Set CC Output: " );
			Serial_SendNumber( pEL->I_set, 4 );
			Serial_SendString( " mA \r\n" );
			if ( pEL->is_output_work == TRUE ) {
				ELoad_EnableCC( pEL, mcp );
			}
		} else {
			// 若输入值不合法则回溯到之前保留的电流值
			pEL->is_I_valid = FALSE;
			if ( pEL->I_set > I_SET_MAX ) {
				Serial_SendString( "[Error] Invalid current value: I > 1000 mA \r\n" );
				OLED_ShowString_8x16_Reverse( 1, 1, "X Err: I>1000 mA" );
			} else if ( pEL->I_set < I_SET_MIN ) {
				Serial_SendString( "[Error] Invalid current value: I < 100 mA \r\n" );
				OLED_ShowString_8x16_Reverse( 1, 1, "X Err: I< 100 mA" );
			}
			pEL->I_set = pEL->lastI_set;
			pEL->input_val[0] = (pEL->lastI_set / 1000) % 10;
			pEL->input_val[1] = (pEL->lastI_set / 100 ) % 10;
			pEL->input_val[2] = (pEL->lastI_set / 10  ) % 10;
			Delay_ms(500);
		}
		pEL->confirm = EOK_NONE;		// 重置确认标志位
		ELoad_Menu_JumpTo( pEL, 0, 2 );
		return;
	}
	
	// 用户取消
	if ( pEL->confirm == EOK_DENIED ) {
		Serial_SendString( "[Info] User cancelled current setting \r\n" );
		OLED_ShowString_8x16_Reverse( 1, 1, "X Cancel: I_set " );
		pEL->I_set = pEL->lastI_set;
		pEL->input_val[0] = (pEL->lastI_set / 1000) % 10;
		pEL->input_val[1] = (pEL->lastI_set / 100 ) % 10;
		pEL->input_val[2] = (pEL->lastI_set / 10  ) % 10;
		Delay_ms(500);
		pEL->confirm = EOK_NONE; 			// 重置确认标志位
		ELoad_Menu_JumpTo( pEL, 0, 2 );
		return;
	}
}

void ELoad_LARTest( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp ) {
	float v_open = 0.0f, v_fullload = 0.0f, v_normal = 0.0f;
	// 备份电流数据以便之后重置
	pEL->lastI_set = pEL->I_set;
	
	OLED_ShowString_16x16_Reverse( 2, 1, menu2_testing );
	
	OLED_ShowNum( 4, 10, 0, 4 );
	MCP4725_FastDAC( mcp, MCP4725_OFF_500K, 0 );
	ELoad_CheckOverU( pEL, mcp );
	for (int i = 0; i < 5; i++) {
		GPIO_SetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
		GPIO_ResetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
	}
	ELoad_CollectData( pEL, ina );
	v_open = pEL->U;
	/*
	OLED_ShowNum( 4, 10, 1000, 4 );
	MCP4725_SetCurrent( mcp, 1000 );
	ELoad_CheckOverU( pEL, mcp );
	for (int i = 0; i < 5; i++) {
		GPIO_SetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
		GPIO_ResetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
	}
	ELoad_CollectData( pEL, ina );
	v_fullload = pEL->U;
	*/
	
	// 以事先设置的电流作为额定负载，若未设置则默认为 1000 mA
	if ( pEL->I_set == 0 ) {
		OLED_ShowNum( 4, 10, LAR_DEFAULT_FULLLOAD, 4 );
		OLED_ShowString_16x16_Reverse( 4, 1, menu4_default );
		MCP4725_SetCurrent( mcp, LAR_DEFAULT_FULLLOAD );
	} else {
		OLED_ShowNum( 4, 10, pEL->I_set, 4 );
		MCP4725_SetCurrent( mcp, pEL->I_set );
	}
	ELoad_CheckOverU( pEL, mcp );
	for (int i = 0; i < 5; i++) {
		GPIO_SetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
		GPIO_ResetBits( GPIOC, GPIO_Pin_14 | GPIO_Pin_15 );
		Delay_ms(500);
	}
	ELoad_CollectData( pEL, ina );
	v_normal = pEL->U;
	
	// ！！！250614：修改 LAR 测试逻辑
	v_fullload = pEL->U;	
	// 计算 LAR
	pEL->lastLAR = ( fabs( v_open - v_fullload ) / v_normal ) * 100.0f;
	// 限幅处理
	// pEL->lastLAR = fmaxf( 0.1f, fminf( pEL->lastLAR, 19.9f ) );
	
	// 串口发送调试数据
	uint32_t LAR_display = (uint32_t)( pEL->lastLAR * 10 );
	Serial_SendString( "[Info] Last LAR = " );
	Serial_SendNumber( LAR_display / 100 % 10, 1 );
	Serial_SendNumber( LAR_display / 10  % 10, 1 );
	Serial_SendString( "." );
	Serial_SendNumber( LAR_display       % 10, 1 );
	Serial_SendString( " % \r\n" );
	
	// 恢复恒流数据
	MCP4725_SetCurrent( mcp, pEL->lastI_set );
	OLED_ShowString_16x16( 4, 1, menu0_Set );
	// 交还控制权
	ELoad_Menu_JumpTo( pEL, 2, 1 );
}

void ELoad_AutoTest( ELoad *pEL, INA226_TypeDef *ina, MCP4725_TypeDef *mcp ) {
	// 42: 自动测试模式
	// !!! 该模式获得完全的控制权
	if ( (pEL->menu[0] == 4) && (pEL->menu[1] == 2) ) {
		pEL->lastI_set = pEL->I_set;
		
		OLED_ShowString_16x16_Reverse( 1, 1, menu4_now );
		OLED_ShowString_16x16_Reverse( 4, 1, menu4_testing );
		
		for (uint16_t this_current = I_SET_MIN; this_current <= I_SET_MAX; this_current += AUTOTEST_STEP) {
			pEL->I_set = this_current;
			ELoad_CheckOverU( pEL, mcp );
			// 先设置恒流输出的参考电压
			ELoad_EnableCC( pEL, mcp );
			// 再等待系统稳定
			Delay_ms( CALIB_DELAY_MS );
			
			Serial_SendString("\n[Debug] I(set) = ");
			Serial_SendNumber( this_current, 4 );
			Serial_SendString(" mA \r\n");
			ELoad_CollectData( pEL, ina );
			
			// 显示当前电流
			OLED_ShowNum( 1, 10, pEL->I_set, 4 );
			
			// 采集好数据之后再显示
			if ( pEL->U < 10000000 ) {
				OLED_ShowChar_8x16( 2, 7 , ' ' );
			} else {
				OLED_ShowChar_8x16( 2, 7 , pEL->U / 10000000 % 10 + '0' );
			}
			OLED_ShowChar_8x16( 2, 8 , pEL->U / 1000000  % 10 + '0' );
			OLED_ShowChar_8x16( 2, 8 , pEL->U / 1000000  % 10 + '0' );
			OLED_ShowChar_8x16( 2, 9 , '.' );
			OLED_ShowChar_8x16( 2, 10, pEL->U / 100000   % 10 + '0' );
			OLED_ShowChar_8x16( 2, 11, pEL->U / 10000    % 10 + '0' );
			OLED_ShowChar_8x16( 2, 12, pEL->U / 1000     % 10 + '0' );
			OLED_ShowChar_8x16( 2, 13, pEL->U / 100      % 10 + '0' );
			
			if ( pEL->I > 1000000 )
				OLED_ShowChar_8x16( 3, 8 , pEL->I / 1000000  % 10 + '0' );
			else
				OLED_ShowChar_8x16( 3, 8 , ' ' );
			
			if ( pEL->I > 100000 )
				OLED_ShowChar_8x16( 3, 9 , pEL->I / 100000   % 10 + '0' );
			else
				OLED_ShowChar_8x16( 3, 9 , ' ' );
			
			if ( pEL->I > 10000 ) 
				OLED_ShowChar_8x16( 3, 10, pEL->I / 10000	 % 10 + '0' );
			else
				OLED_ShowChar_8x16( 3, 10, ' ' );
			OLED_ShowChar_8x16( 3, 11, pEL->I / 1000 	 % 10 + '0' );
			OLED_ShowChar_8x16( 3, 12, '.' );
			OLED_ShowChar_8x16( 3, 13, pEL->I / 100  	 % 10 + '0' );
			
			Serial_SendString( "[Debug] U = " );
			Serial_SendNumber( pEL->U / 1000000, 2 );
			Serial_SendString( "." );
			Serial_SendNumber( pEL->U / 1000 % 1000, 3 );
			Serial_SendString( " " );
			Serial_SendNumber( pEL->U % 1000, 3 );
			Serial_SendString( " V    I = " );
			Serial_SendNumber( pEL->I / 1000000, 1 );
			Serial_SendString( " " );
			Serial_SendNumber( pEL->I / 1000 % 1000, 3 );
			Serial_SendString( "." );
			Serial_SendNumber( pEL->I % 1000, 3 );
			Serial_SendString( " mA \r\n" );
		}
		pEL->I_set = pEL->lastI_set;
		ELoad_EnableCC( pEL, mcp );
		Serial_SendString("[Debug] Test DONE \n \r\n");
		pEL->debug_flag = EOK_DBG_DISABLE;
		ELoad_Menu_JumpTo( pEL, 0, 2 );
	}
}
