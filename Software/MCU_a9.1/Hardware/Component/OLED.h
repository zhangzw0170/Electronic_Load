#ifndef __OLED_H
#define __OLED_H

#define OLED_16x16_EMPTY 0xFF
#define SH1106

void OLED_Init(void);
void OLED_Clear(void);

void OLED_ShowChar_8x16(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowChar_16x16(uint8_t Line, uint8_t Column, uint8_t idx);
void OLED_ShowChar_8x16_Reverse(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowChar_16x16_Reverse(uint8_t Line, uint8_t Column, uint8_t idx);
void OLED_ShowString_8x16(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowString_16x16(uint8_t Line, uint8_t Column, uint8_t *pidx);
void OLED_ShowString_8x16_Reverse(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowString_16x16_Reverse(uint8_t Line, uint8_t Column, uint8_t *pidx);
void OLED_ShowString_8x16_Selected(uint8_t Line, uint8_t Column, char *String, uint8_t selected_col);

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowFloatNum_Single(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Weight, uint8_t PointPos);
void OLED_ShowFloatNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t MaxWeight, uint8_t MinWeight, uint8_t PointPos);
void OLED_ShowAntiNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
