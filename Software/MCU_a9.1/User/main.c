#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "ELoad.h"

int main(void)
{
	INA226_TypeDef u_ina;
	MCP4725_TypeDef u_mcp;
	
	SysTick_Init(1000);	// ms 级中断
	ELoad EL = ELoad_Init();
	do {
		// 初始化直到完成
		ELoad_Start( &EL, &u_ina, &u_mcp );
	} while (EL.is_inited == FALSE);
	
	while (1)             
	{
		// 输入处理
		ELoad_CollectData( &EL, &u_ina );
		ELoad_Serial_Receive( &EL, &u_ina, &u_mcp );
		// 屏幕输出控制
		ELoad_Show_Normal( &EL );
		
		EL.lastKeyCode = Keyboard_Scan();
		if (EL.lastKeyCode != KB_NONE) {
			// 根据键码处理标志位
			ELoad_Keyboard_To_Flag_Handle( &EL );
		}
		
		// 根据标志位处理硬件
        ELoad_Flag_To_Hardware_Handle(&EL, &u_ina, &u_mcp);
	}
}
