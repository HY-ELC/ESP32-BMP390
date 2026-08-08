#ifndef __OLED_H
#define __OLED_H

#include <Arduino.h>
#include "FFT_Process.h"

bool OLED_Init(void);
void OLED_ShowData(float freq, float rms, float db,
                   RiskLevel_t level, const char* band);
void OLED_ShowBoot(void);

#endif
