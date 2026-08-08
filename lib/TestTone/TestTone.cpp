#include "TestTone.h"

#define TONE_PIN      23            /* GPIO23 — LEDC PWM */
#define LEDC_CHANNEL  0             /* LEDC 通道 0 */
#define LEDC_FREQ     5000          /* PWM 基频 5kHz */
#define LEDC_RES      8             /* 8-bit 分辨率 (0-255) */
#define SAMPLE_RATE   200.0f        /* 与传感器采样率一致 */
#define PWM_AMPLITUDE 120           /* 幅度 (max 127) */
#define PWM_CENTER    127           /* 中点 */

static const float toneFreqs[TONE_COUNT] =
{
    5.0f,    /* 5 Hz  — 次声波低频 */
    8.0f,    /* 8 Hz  — 中频次声波 */
    10.0f,   /* 10 Hz — 标准测试 */
    15.0f,   /* 15 Hz — 高频次声波 */
    20.0f,   /* 20 Hz — 次声波上限 */
    0.0f     /* OFF  — 关闭输出 */
};

static uint8_t  toneIndex   = TONE_COUNT - 1;
static float    phaseAcc    = 0.0f;
static bool     enabled     = false;

void TestTone_Init(void)
{
    /* 配置 LEDC PWM → GPIO23 */
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RES);
    ledcAttachPin(TONE_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, PWM_CENTER); /* 中点 ~50% duty */

    phaseAcc = 0.0f;
    toneIndex = TONE_COUNT - 1;
    enabled = false;

    Serial.println(F("[TONE] PWM测试信号就绪 (GPIO23, 5kHz/8bit)"));
    Serial.println(F("[TONE] 命令: t=下一频率, 1~5=指定频率, 0=关, o=开关"));
    Serial.println(F("[TONE] 频率: 5/8/10/15/20 Hz, Off"));
    Serial.println(F("[TONE] 硬件: GPIO23 → 杜邦线 → 靠近BMP390传感器孔"));
}

void TestTone_Update(void)
{
    float freq = toneFreqs[toneIndex];

    if (!enabled || freq <= 0.01f)
    {
        ledcWrite(LEDC_CHANNEL, PWM_CENTER);
        return;
    }

    /* 相位累加器 */
    phaseAcc += 2.0f * PI * freq / SAMPLE_RATE;
    if (phaseAcc > 2.0f * PI)
        phaseAcc -= 2.0f * PI;

    /* 正弦波 → 8-bit PWM duty */
    float sinVal = sinf(phaseAcc);
    uint8_t duty = (uint8_t)(PWM_CENTER + (int)(PWM_AMPLITUDE * sinVal));

    ledcWrite(LEDC_CHANNEL, duty);
}

void TestTone_NextFreq(void)
{
    toneIndex = (toneIndex + 1) % TONE_COUNT;
    phaseAcc = 0.0f;

    float f = toneFreqs[toneIndex];
    if (f > 0.01f)
    {
        enabled = true;
        Serial.print(F("[TONE] → "));
        Serial.print(f, 0);
        Serial.println(F(" Hz"));
    }
    else
    {
        enabled = false;
        Serial.println(F("[TONE] → OFF"));
    }
}

void TestTone_SetFreq(float hz)
{
    phaseAcc = 0.0f;
    for (int i = 0; i < TONE_COUNT; i++)
    {
        if (fabsf(toneFreqs[i] - hz) < 0.1f)
        {
            toneIndex = i;
            enabled = (hz > 0.01f);
            Serial.print(F("[TONE] → "));
            Serial.print(toneFreqs[toneIndex], 0);
            Serial.println(F(" Hz"));
            return;
        }
    }
    enabled = (hz > 0.01f);
    Serial.print(F("[TONE] 自定义 "));
    Serial.print(hz, 1);
    Serial.println(F(" Hz"));
}

float TestTone_GetFreq(void)  { return toneFreqs[toneIndex]; }
bool  TestTone_IsEnabled(void) { return enabled && (toneFreqs[toneIndex] > 0.01f); }

void TestTone_Enable(bool en)
{
    enabled = en;
    phaseAcc = 0.0f;
    Serial.println(en ? F("[TONE] 开启") : F("[TONE] 关闭"));
}
