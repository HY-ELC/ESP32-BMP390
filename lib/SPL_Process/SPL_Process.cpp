#include "SPL_Process.h"

#include <math.h>



/*
    标准参考声压

    20uPa

*/

#define PREF 0.00002f



/*
    系统校准偏移

    根据实际测试调整

*/

static float calibrationOffset = 0;



void SPL_Init()
{

    calibrationOffset = 0;

}





float SPL_Process(
    float rmsPressure
)
{


    /*
        防止log(0)

    */


    if(rmsPressure < 0.000001f)
    {
        return 0;
    }



    /*
        声压级公式

        SPL =
        20log10(P/P0)

    */


    float db;


    db =
    20.0f *
    log10(
        rmsPressure /
        PREF
    );



    db += calibrationOffset;



    /*
        限制显示范围

        避免异常

    */

    if(db<0)
        db=0;


    if(db>160)
        db=160;



    return db;

}







RiskLevel_t SPL_GetRiskLevel(
    float db,
    float snr
)
{


    /*
        信号可信度不足

        环境噪声不报警

    */


    if(snr < 2.0f)
    {

        return LEVEL_SAFE;

    }



    /*
        人体影响等级

    */


    if(db < 90.0f)
    {

        return LEVEL_SAFE;

    }

    else if(db < 105.0f)
    {

        return LEVEL_NOTICE;

    }

    else if(db < 120.0f)
    {

        return LEVEL_WARNING;

    }

    else
    {

        return LEVEL_DANGER;

    }

}






const char* SPL_LevelString(
    RiskLevel_t level
)
{

    switch(level)
    {

        case LEVEL_SAFE:

            return "SAFE";


        case LEVEL_NOTICE:

            return "NOTICE";


        case LEVEL_WARNING:

            return "WARNING";


        case LEVEL_DANGER:

            return "DANGER";


        default:

            return "UNKNOWN";

    }

}