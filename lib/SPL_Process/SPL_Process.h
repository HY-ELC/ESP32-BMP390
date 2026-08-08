#ifndef SPL_PROCESS_H
#define SPL_PROCESS_H

#include <Arduino.h>


/*
    风险等级

    全工程唯一来源

*/

typedef enum
{

    LEVEL_SAFE = 0,

    LEVEL_NOTICE,

    LEVEL_WARNING,

    LEVEL_DANGER


}RiskLevel_t;



void SPL_Init();



/*
    RMS压力转换声压级

    输入:
    Pa

    输出:
    dB

*/

float SPL_Process(
    float rmsPressure
);



/*
    获取风险等级

    输入:

    dB
    SNR

*/

RiskLevel_t SPL_GetRiskLevel(
    float db,
    float snr
);



const char* SPL_LevelString(
    RiskLevel_t level
);



#endif