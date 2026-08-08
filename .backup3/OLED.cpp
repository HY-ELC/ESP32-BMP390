#include "OLED.h"
#include <U8g2lib.h>

#define OLED_SDA 21
#define OLED_SCL 22

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

#define CN u8g2_font_wqy12_t_gb2312a
#define EN u8g2_font_7x14_tf

bool OLED_Init(void)
{
    if (!u8g2.begin()) return false;
    u8g2.enableUTF8Print();
    return true;
}

void OLED_ShowBoot(void)
{
    u8g2.clearBuffer();
    u8g2.setFont(CN);
    u8g2.drawUTF8(10, 28, "次声波检测系统");
    u8g2.setFont(EN);
    u8g2.drawStr(28, 50, "BMP390 v3");
    u8g2.sendBuffer();
    delay(1500);
}

void OLED_ShowData(float freq, float rms, float db,
                   RiskLevel_t level, const char* band)
{
    u8g2.clearBuffer();
    char buf[16];

    /* 频率 */
    u8g2.setFont(CN);
    u8g2.drawUTF8(0, 14, "频率:");
    u8g2.setFont(EN);
    if (freq < 0.0f) snprintf(buf, sizeof(buf), "--Hz");
    else             snprintf(buf, sizeof(buf), "%.1fHz", freq);
    u8g2.drawStr(50, 14, buf);

    /* 声压 */
    u8g2.setFont(CN);
    u8g2.drawUTF8(0, 30, "声压:");
    u8g2.setFont(EN);
    snprintf(buf, sizeof(buf), "%.0fdB", db);
    u8g2.drawStr(50, 30, buf);

    /* 等级 */
    u8g2.setFont(CN);
    u8g2.drawUTF8(0, 46, "等级:");
    const char* lvl;
    switch (level)
    {
        case LEVEL_SAFE:    lvl = "安全"; break;
        case LEVEL_NOTICE:  lvl = "注意"; break;
        case LEVEL_WARNING: lvl = "警告"; break;
        case LEVEL_DANGER:  lvl = "危险"; break;
    }
    u8g2.drawUTF8(50, 46, lvl);

    /* 频段 */
    u8g2.setFont(CN);
    u8g2.drawUTF8(0, 62, "频段:");
    u8g2.setFont(EN);
    u8g2.drawStr(50, 62, band);

    u8g2.sendBuffer();
}
