#ifndef __FFT_PROCESS_H
#define __FFT_PROCESS_H

#include <Arduino.h>

#define FFT_SAMPLES    256    /* 256点 → 1.28s窗, 0.64s刷新 */
#define SAMPLE_FREQ    200

typedef enum
{
    LEVEL_SAFE = 0,
    LEVEL_NOTICE,
    LEVEL_WARNING,
    LEVEL_DANGER
} RiskLevel_t;

void FFT_Init(void);
void FFT_AddSample(float sample);
bool FFT_IsReady(void);

/* 平滑频率 (EMA, SNR门控) — 推荐使用 */
float FFT_GetMainFrequency(void);

/* 原始插值频率 (响应快但跳动大) */
float FFT_GetRawFrequency(void);

/* 峰值信噪比 (SNR < 1.8 → 无清晰信号) */
float FFT_GetSNR(void);

float FFT_GetRMS(void);
float FFT_GetEquivalentDB(void);
RiskLevel_t FFT_GetRiskLevel(void);

#endif
