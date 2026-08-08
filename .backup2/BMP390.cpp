#include "BMP390.h"
#include <Wire.h>
#include <Adafruit_BMP3XX.h>

#define BMP390_ADDR_A  0x76
#define BMP390_ADDR_B  0x77

Adafruit_BMP3XX bmp;
static uint8_t  bmp_addr = 0;

/* 独立 I2C 总线 (GPIO18/19), 与 OLED(GPIO21/22) 分开 */
TwoWire BMP390_I2C = TwoWire(0);
static float    BaselinePressure = 0;

/* 缓存单次读取的温度/气压，避免重复 performReading */
static float    cachedPressure    = 0;
static float    cachedTemperature = 0;

/* 压力单位: true=库返回Pa, false=库返回hPa需要×100 */
static bool     pressureInPa = false;
static bool     unitDetected = false;

bool BMP390_Init(void)
{
    BMP390_I2C.begin(18, 19, 400000);  /* SDA=GPIO18, SCL=GPIO19 */
    Serial.println(F("[DMP390] ─── BMP390 初始化 (I2C0, GPIO18/19) ───"));

    Serial.print(F("[DMP390] 尝试地址 0x77... "));
    if (bmp.begin_I2C(BMP390_ADDR_B, &BMP390_I2C))
    {
        bmp_addr = BMP390_ADDR_B;
        Serial.println(F("成功!"));
        goto config;
    }
    Serial.println(F("失败"));

    Serial.print(F("[DMP390] 尝试地址 0x76... "));
    if (bmp.begin_I2C(BMP390_ADDR_A, &BMP390_I2C))
    {
        bmp_addr = BMP390_ADDR_A;
        Serial.println(F("成功!"));
        goto config;
    }
    Serial.println(F("失败"));

    Serial.println(F("[DMP390] ✗ 未找到 BMP390"));
    Serial.println(F("[DMP390]   接线: SDA→GPIO18, SCL→GPIO19"));
    return false;

config:
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_32X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    /* COEFF_3 @200Hz → 带宽≈21Hz, 覆盖全频段次声波
     * 之前 COEFF_127 带宽仅0.66Hz → 滤掉了所有信号! */
    bmp.setOutputDataRate(BMP3_ODR_200_HZ);

    /* 检测压力单位 */
    delay(100);
    if (bmp.performReading())
    {
        Serial.print(F("[BMP390] bmp.pressure="));
        Serial.println(bmp.pressure, 2);

        if (bmp.pressure >= 300.0f && bmp.pressure <= 1250.0f)
        {
            Serial.println(F("[BMP390] 单位=hPa (×100→Pa)"));
            pressureInPa = false;
        }
        else if (bmp.pressure >= 30000.0f && bmp.pressure <= 125000.0f)
        {
            Serial.println(F("[BMP390] 单位=Pa"));
            pressureInPa = true;
        }
        else
        {
            Serial.println(F("[BMP390] ⚠ 异常范围, 按hPa处理"));
            pressureInPa = false;
        }
        unitDetected = true;
    }

    /* 固定基线校准 (100次平均) */
    delay(200);
    float sum = 0;
    int ok = 0;
    Serial.print(F("[DMP390] 基线校准... "));
    for (int i = 0; i < 100; i++)
    {
        if (bmp.performReading())
        {
            float p = pressureInPa
                ? (float)bmp.pressure
                : (float)(bmp.pressure * 100.0);
            sum += p;
            ok++;
        }
        delay(10);
    }
    BaselinePressure = (ok > 0) ? (sum / ok) : 101325.0f;
    cachedPressure = BaselinePressure;
    Serial.print(BaselinePressure, 2);
    Serial.println(F(" Pa"));
    return true;
}

bool BMP390_Update(void)
{
    if (!bmp.performReading())
        return false;

    cachedTemperature = (float)bmp.temperature;

    /* 补做单位检测 (init未完成时) */
    if (!unitDetected)
    {
        unitDetected = true;
        if (bmp.pressure >= 300.0f && bmp.pressure <= 1250.0f)
            pressureInPa = false;
        else if (bmp.pressure >= 30000.0f && bmp.pressure <= 125000.0f)
            pressureInPa = true;
        else
            pressureInPa = false;
    }

    cachedPressure = pressureInPa
        ? (float)bmp.pressure
        : (float)(bmp.pressure * 100.0);

    /* ── 自动基线重校准 ──
     *
     * 每30秒或 |ΔP|>80Pa 时重校准,
     * 防止大气压漂移累积导致 ΔP 失控
     */
    static unsigned long lastRecal = 0;
    float currentDP = cachedPressure - BaselinePressure;
    if (millis() - lastRecal > 30000 || fabsf(currentDP) > 80.0f)
    {
        BaselinePressure = cachedPressure;
        lastRecal = millis();
    }

    return true;
}

float BMP390_ReadPressure(void)        { return cachedPressure; }
float BMP390_ReadTemperature(void)     { return cachedTemperature; }
float BMP390_ReadDeltaPressure(void)   { return cachedPressure - BaselinePressure; }
float BMP390_GetBaseline(void)         { return BaselinePressure; }

void BMP390_Calibration(void)
{
    /* 重新校准固定基线 */
    float sum = 0;
    int ok = 0;
    for (int i = 0; i < 50; i++)
    {
        if (bmp.performReading())
        {
            float p = pressureInPa ? (float)bmp.pressure : (float)(bmp.pressure * 100.0);
            sum += p;
            ok++;
        }
        delay(5);
    }
    if (ok > 0) BaselinePressure = sum / ok;
    cachedPressure = BaselinePressure;
    Serial.print(F("[DMP390] 基线重校准: "));
    Serial.print(BaselinePressure, 2);
    Serial.println(F(" Pa"));
}
