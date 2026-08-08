#include "FIR_Filter.h"

/************************************************
 * 信号链: 微分(dP/dt) → 4点MA平滑 → FFT
 *
 * 微分压制漂移 >100:1
 * 4点MA (fc≈22Hz): 噪声平滑, 保 0~20Hz
 ************************************************/

#define DIFF_GAIN    8.0f    /* 灵敏度 */

static float FIR_Buffer[FIR_TAP_NUM];
static uint16_t FIR_Index = 0;
static float  prevInput  = 0.0f;
static bool   firstDiff  = true;

/* 4点MA + 61零 = 65 taps  (4+54+7=65) */
static const float FIR_Coeff[FIR_TAP_NUM] =
{
    0.25f, 0.25f, 0.25f, 0.25f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

void FIR_Init(void)
{
    for(uint16_t i=0; i<FIR_TAP_NUM; i++) FIR_Buffer[i] = 0.0f;
    FIR_Index  = 0;
    firstDiff  = true;
    prevInput  = 0.0f;
}

float FIR_Process(float input)
{
    /* 微分: dP/dt = P[n] - P[n-1] */
    float diff;
    if (firstDiff) { diff = 0.0f; firstDiff = false; }
    else           { diff = input - prevInput; }
    prevInput = input;
    diff *= DIFF_GAIN;

    /* 4点MA平滑 */
    FIR_Buffer[FIR_Index] = diff;
    float output = 0.0f;
    int idx = FIR_Index;
    for(int i=0; i<FIR_TAP_NUM; i++)
    {
        output += FIR_Coeff[i] * FIR_Buffer[idx];
        idx--;
        if(idx < 0) idx = FIR_TAP_NUM - 1;
    }
    FIR_Index++;
    if(FIR_Index >= FIR_TAP_NUM) FIR_Index = 0;

    return output;
}
