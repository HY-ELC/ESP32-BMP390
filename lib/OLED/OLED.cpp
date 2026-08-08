#include "OLED.h"
#include <U8g2lib.h>

#define OLED_SDA 21
#define OLED_SCL 22


U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0,
    OLED_SCL,
    OLED_SDA,
    U8X8_PIN_NONE
);


#define CN_FONT u8g2_font_wqy12_t_gb2312a
#define EN_FONT u8g2_font_6x12_tf



bool OLED_Init()
{
    if(!u8g2.begin())
        return false;


    u8g2.enableUTF8Print();

    return true;
}



void OLED_ShowBoot()
{
    u8g2.clearBuffer();


    u8g2.setFont(CN_FONT);

    u8g2.drawUTF8(
        8,
        28,
        "次声波检测系统"
    );


    u8g2.setFont(EN_FONT);

    u8g2.drawStr(
        35,
        48,
        "BMP390"
    );


    u8g2.sendBuffer();


    delay(1000);
}



static const char* LevelToString(
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
            return "----";
    }
}



void OLED_ShowData(
    float freq,
    float rms,
    float db,
    float snr,
    RiskLevel_t level
)
{

    char buf[24];


    u8g2.clearBuffer();


    u8g2.setFont(EN_FONT);


    /*
    Frequency
    */

    if(freq < 0)
    {
        snprintf(
            buf,
            sizeof(buf),
            "Freq: -- Hz"
        );
    }
    else
    {
        snprintf(
            buf,
            sizeof(buf),
            "Freq:%5.2fHz",
            freq
        );
    }


    u8g2.drawStr(
        0,
        12,
        buf
    );



    /*
    SPL
    */

    snprintf(
        buf,
        sizeof(buf),
        "SPL :%5.1fdB",
        db
    );


    u8g2.drawStr(
        0,
        26,
        buf
    );



    /*
    RMS
    */

    snprintf(
        buf,
        sizeof(buf),
        "RMS :%5.2fPa",
        rms
    );


    u8g2.drawStr(
        0,
        40,
        buf
    );



    /*
    SNR
    */

    snprintf(
        buf,
        sizeof(buf),
        "SNR :%5.2f",
        snr
    );


    u8g2.drawStr(
        0,
        52,
        buf
    );



    /*
    Risk

    */

    snprintf(
        buf,
        sizeof(buf),
        "LV:%s",
        LevelToString(level)
    );


    u8g2.drawStr(
        75,
        52,
        buf
    );


    u8g2.sendBuffer();

}