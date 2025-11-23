# Copilot / AI Agent 指南 — Electronic_Load

目标：让自动化编码/补全代理快速理解仓库结构、关键约定与可以安全修改的切入点。

总体架构（大图）：
- **固件 MCU（主线）**：位于 `Software/MCU_a9.1/`，基于 STM32F103、使用 STM32 标准外设库（SPL）。
- **硬件模块化**：驱动代码分为 `Hardware/Component`（外设驱动，如 `INA226`, `MCP4725`, `OLED` 等）和 `Hardware/Periph`（板级/平台封装，如 `Serial`, `LED`, `OverU`）。
- **用户逻辑与状态机**：主程序在 `Software/MCU_a9.1/User/main.c`，业务状态与 UI 在 `Software/MCU_a9.1/Hardware/ELoad.c` / `ELoad.h` 中实现。

关键开发/构建流程：
- 使用 Keil uVision 打开 `Software/MCU_a9.1/STM32Template.uvprojx` 并编译；存档有 `JLinkSettings.ini` 和 `EventRecorderStub.scvd` 帮助调试。烧录常用 J-Link / ST-Link。
- 仓库内没有自动 CI/Makefile；对固件修改后通常在 Keil 中构建并用 J-Link 烧录。

重要代码约定（请严格遵守）：
- 外设/驱动放在 `Software/MCU_a9.1/Hardware/Component`（每个设备一对 `.c/.h`），板级封装放 `Hardware/Periph`。新增驱动请同样放在 `Component`，并在 `ELoad.h` 中包含与在 `ELoad_Start` 中初始化。
- 全局/配置宏集中在 `Software/MCU_a9.1/Hardware/ELoad_config.h`。修改地址、校准系数、OVP 阈值等应先改该文件。
- I2C 地址处理细节：驱动里通常用 `addr << 1` 存储（STM32 HAL/SPL expects 8-bit address），但 I2C 初始化时 `OwnAddress1` 使用原 7-bit 值。注意这个“左移一位”的惯例（见 `INA226_Init` / `MCP4725_Init`）。
- 单位约定：
  - 电压在代码中以微伏 `uV` 表示（例如 `ELoad.U` 使用 `INA226_GetBusVoltage_uV()`）。
  - 电流以微安 `uA` 表示（例如 `ELoad.I` 使用 `INA226_GetCurrent_uA()`）。
  - 校准常数（`CALIB_I_K`, `CALIB_I_B` 等）在 `ELoad_config.h`，有时需要将 `uA/uV` 转换为 mA/V 用于显示。

常见代码模式与示例（可直接参考）：
- 状态机与 UI：`Software/MCU_a9.1/Hardware/ELoad.c` 中 `ELoad` 结构体（`ELoad.h`）承载所有 UI/运行时状态；主循环在 `User/main.c`：
  - 初始化：`ELoad EL = ELoad_Init(); ELoad_Start(&EL, &u_ina, &u_mcp);`
  - 采集：`ELoad_CollectData(&EL, &u_ina);` （注意数据单位为 uV/uA 并随后做校准）
  - 控制：`ELoad_Flag_To_Hardware_Handle(&EL, &u_ina, &u_mcp);` 用于把 UI/标志映射到实际硬件动作。
- DAC / 校准示例：`MCP4725_SetCurrent()` 使用 `MCP4725_RSHUNT` 与 `MCP4725_VOL_REF` 计算输出代码（见 `Hardware/Component/MCP4725.c`）。
- 传感器驱动：`INA226` 的寄存器读写直接基于 SPL 的 I2C 事件（参见 `Hardware/Component/INA226.c`），错误通过结构体内 `error` 字段传播。

调试/日志约定：
- 串口输出通过 `Periph/Serial.c`（`Serial_Init()` / `Serial_SendString()`）打印启动与错误信息；`ELoad.h` 中通过 `#define DEBUG` 控制调试。修改日志级别请检查 `ELoad.h`。
- OLED 用于主要交互，显示函数集中在 `Component/OLED.c`，UI 索引字符表与 layout 在 `ELoad.h` 内的静态数组提供示例。

安全与硬件注意事项（代理应提示开发者）：
- 过压保护：代码中以 `OVER_U`（单位 V）做检查，但比较时使用 `OVER_U * 1000000`（microvolt）。修改阈值请在 `ELoad_config.h` 更改并验证。
- 修改涉及 MOSFET 或功率通路的代码前务必提醒人工检查硬件（散热、限流），并建议先在低电压/低电流条件下验证。

如何扩展或修复驱动（步骤提示）：
1. 在 `Hardware/Component` 添加 `foo.c` + `foo.h`（遵循现有驱动样例风格）。
2. 在 `ELoad.h` 中包含 `Component/foo.h` 并在 `ELoad` 结构或调用点添加必要字段/实例。
3. 在 `ELoad_Start()` 中添加初始化流程，在 `ELoad_Flag_To_Hardware_Handle()` 或对应位置添加运行时调用。
4. 使用 `Serial_SendString()` 输出初始化/错误信息以便现场调试。

参考文件（快速跳转）：
- 主程序：`Software/MCU_a9.1/User/main.c`
- 业务与 UI：`Software/MCU_a9.1/Hardware/ELoad.c`, `Software/MCU_a9.1/Hardware/ELoad.h`
- 配置：`Software/MCU_a9.1/Hardware/ELoad_config.h`
- 外设驱动：`Software/MCU_a9.1/Hardware/Component/INA226.c`, `MCP4725.c`, `OLED.c`
- 平台层：`Software/MCU_a9.1/Library/`（STM32 SPL）与 `Software/MCU_a9.1/Start/`, `System/`

说明请求：我已将仓库内可发现的约定与示例写入该文件。请审阅以下两点并回复：
- 哪些硬件/构建步骤在文档中不够详细（例如你用的是 ST-Link 还是 J-Link，或有特定烧录脚本）？
- 是否需要把 `ELoad_config.h` 中的常用宏（如 `OVER_U`, `INA226_ADDR`, `MCP4725_ADDR`, `CALIB_*`）逐条摘录到本文件中以便 AI 快速查看？

—— End
