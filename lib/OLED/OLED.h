#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include "SPL_Process.h"

bool OLED_Init();

void OLED_ShowBoot();

void OLED_ShowData(
    float freq,
    float rms,
    float db,
    float snr,
    RiskLevel_t level
);

#endif