#include "FFT_Process.h"

#include <arduinoFFT.h>
#include <math.h>

/************************************************
 * FFT Object
 ************************************************/
ArduinoFFT<double> FFT;

/************************************************
 * FFT Buffer
 ************************************************/
static double vReal[FFT_SAMPLES];   /* 原始样本环形缓冲 (不修改) */
static double vImag[FFT_SAMPLES];   /* FFT 虚部 */
static double vWork[FFT_SAMPLES];   /* FFT 工作区 (拷贝后处理) */

/* 滑动重叠: 75%叠加 → 256新样本 → 1.28s刷新 */
#define OVERLAP_SHIFT         128   /* 50%重叠→0.64s刷新 */

static uint16_t SampleIndex = 0;
static bool     FFTReady   = false;

/************************************************
 * 时域 RMS 累加器
 ************************************************/
static double timeDomainSumSq = 0.0;

/************************************************
 * Result
 ************************************************/
static float MainFrequency  = 0.0f;
static float PressureRMS    = 0.0f;
static float EquivalentDB   = 0.0f;
static float PeakSNR        = 0.0f;  /* 峰值信噪比 */
static RiskLevel_t RiskLevel = LEVEL_SAFE;

/* EMA 平滑后的频率 (稳定输出) */
static float SmoothedFreq   = 0.0f;
static bool  smoothSeeded   = false;

/************************************************
 * 声压参考值: 20 μPa = 0 dB SPL
 ************************************************/
#define REFERENCE_PRESSURE   0.00002f

/* 峰值搜索起始 bin */
#define PEAK_START_BIN       1    /* 0.20Hz, 漂移已被微分压制 */
/* SNR 阈值: 低于此值视为无信号 */
#define SNR_THRESHOLD        1.8f
/* 频率 EMA 平滑系数 */
#define FREQ_SMOOTH_ALPHA    0.70f  /* 快速跟踪 */

/************************************************
 * 快速排序 (用于中位数计算)
 ************************************************/
static void swap(double *a, double *b)
{
    double t = *a; *a = *b; *b = t;
}

static double medianOfBins(double *mags, uint16_t start, uint16_t count,
                           uint16_t excludeIdx)
{
    /* 复制到临时数组，排除峰值 bin */
    double tmp[64];  /* 足够大 */
    uint16_t n = 0;
    for (uint16_t i = start; i < start + count && n < 64; i++)
    {
        if (i != excludeIdx)
            tmp[n++] = mags[i];
    }
    if (n == 0) return 0.0;

    /* 简单冒泡排序找中位数 (n ≤ 61, 可接受) */
    for (uint16_t i = 0; i < n - 1; i++)
    {
        for (uint16_t j = i + 1; j < n; j++)
        {
            if (tmp[i] > tmp[j])
                swap(&tmp[i], &tmp[j]);
        }
    }
    return tmp[n / 2];
}

/************************************************
 * Init
 ************************************************/
void FFT_Init(void)
{
    SampleIndex = 0;
    FFTReady    = false;

    MainFrequency = 0.0f;
    PressureRMS   = 0.0f;
    EquivalentDB  = 0.0f;
    PeakSNR       = 0.0f;
    RiskLevel     = LEVEL_SAFE;

    SmoothedFreq  = (PEAK_START_BIN*SAMPLE_FREQ)/(float)FFT_SAMPLES;
    smoothSeeded  = false;

    timeDomainSumSq = 0.0;

    memset(vReal, 0, sizeof(vReal));
    memset(vImag, 0, sizeof(vImag));
}

/************************************************
 * 添加样本到FFT缓冲区
 ************************************************/
void FFT_AddSample(float sample)
{
    /* FFTReady 期间丢弃样本 (旧结果未被读取) */
    if (FFTReady) return;

    vReal[SampleIndex] = (double)sample;
    vImag[SampleIndex] = 0.0;
    timeDomainSumSq += (double)sample * (double)sample;
    SampleIndex++;

    if (SampleIndex >= FFT_SAMPLES)
    {
        /* ── 拷贝原始样本到工作区, vReal保持原样供重叠 ── */
        memcpy(vWork, vReal, sizeof(vReal));

        /* ============================================
         * 窗内去均值 (在 vWork 上操作)
         * ============================================ */
        double windowMean = 0.0;
        for (uint16_t i = 0; i < FFT_SAMPLES; i++)
            windowMean += vWork[i];
        windowMean /= (double)FFT_SAMPLES;

        for (uint16_t i = 0; i < FFT_SAMPLES; i++)
            vWork[i] -= windowMean;

        /* ============================================
         * 时域 RMS (去均值后的 AC 分量)
         * ============================================ */
        double acSumSq = 0.0;
        for (uint16_t i = 0; i < FFT_SAMPLES; i++)
            acSumSq += vWork[i] * vWork[i];
        PressureRMS = (float)sqrt(acSumSq / (double)FFT_SAMPLES);
        timeDomainSumSq = 0.0;

        /* dB SPL */
        if (PressureRMS > REFERENCE_PRESSURE)
            EquivalentDB = 20.0f * log10f(PressureRMS / REFERENCE_PRESSURE);
        else
            EquivalentDB = 0.0f;

        /* FFT (零化虚部, vWork上操作) */
        memset(vImag, 0, sizeof(vImag));
        FFT.windowing(vWork, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(vWork, vImag, FFT_SAMPLES, FFT_FORWARD);
        FFT.complexToMagnitude(vWork, vImag, FFT_SAMPLES);

        uint16_t usableBins = FFT_SAMPLES / 2;

        /* 全频段搜索: 微分已压制漂移 */
        double peakMagnitude = 0.0;
        uint16_t peakIndex = PEAK_START_BIN;
        for (uint16_t i = PEAK_START_BIN; i < usableBins; i++)
        {
            if (vWork[i] > peakMagnitude)
            {
                peakMagnitude = vWork[i];
                peakIndex = i;
            }
        }

        /* 二次插值 */
        double interpFreq = 0.0;
        if (peakIndex > PEAK_START_BIN && peakIndex < usableBins - 1)
        {
            double magL = vWork[peakIndex - 1];
            double magP = vWork[peakIndex];
            double magR = vWork[peakIndex + 1];
            double denom = 2.0 * magP - magL - magR;
            if (denom > 1e-12)
            {
                double delta = (magR - magL) / denom;
                if (delta > 0.5)  delta = 0.5;
                if (delta < -0.5) delta = -0.5;
                interpFreq = (double)(peakIndex + delta)
                           * (double)SAMPLE_FREQ / (double)FFT_SAMPLES;
            }
            else
                interpFreq = (double)peakIndex
                           * (double)SAMPLE_FREQ / (double)FFT_SAMPLES;
        }
        else
            interpFreq = (double)peakIndex
                       * (double)SAMPLE_FREQ / (double)FFT_SAMPLES;

        MainFrequency = (float)interpFreq;

        /* SNR (局部窗口) */
        uint16_t snrStart = (peakIndex > PEAK_START_BIN + 10)
                            ? (peakIndex - 10) : PEAK_START_BIN;
        uint16_t snrEnd   = (peakIndex + 10 < usableBins)
                            ? (peakIndex + 10) : (usableBins - 1);
        uint16_t snrCount = snrEnd - snrStart + 1;
        double noiseFloor = medianOfBins(vWork, snrStart, snrCount, peakIndex);
        PeakSNR = (noiseFloor > 1e-12)
                  ? (float)(peakMagnitude / noiseFloor) : 99.0f;

        /* EMA 频率平滑 */
        if (PeakSNR >= SNR_THRESHOLD)
        {
            if (!smoothSeeded)
            { SmoothedFreq = MainFrequency; smoothSeeded = true; }
            else
                SmoothedFreq = FREQ_SMOOTH_ALPHA * MainFrequency
                             + (1.0f - FREQ_SMOOTH_ALPHA) * SmoothedFreq;
        }

        /* 诊断: 每2个周期打印频谱快照 */
        static uint32_t diagCount = 0;
        diagCount++;
        if (diagCount % 2 == 1)
        {
            Serial.print(F("[DIAG] RMS="));
            Serial.print(PressureRMS, 2);
            Serial.print(F("Pa SNR="));
            Serial.print(PeakSNR, 1);
            Serial.print(F(" pkBin="));
            Serial.print(peakIndex);
            Serial.print(F(" f="));
            Serial.print(SmoothedFreq, 2);
            Serial.print(F("Hz | bins3-12:"));
            for (uint16_t j = 3; j < 13 && j < usableBins; j++)
            {
                Serial.print(vWork[j], 2);
                if (j < 12) Serial.print(F(","));
            }
            Serial.println();
        }

        /* 风险等级 */
        if (EquivalentDB < 110.0f)          RiskLevel = LEVEL_SAFE;
        else if (EquivalentDB < 120.0f)     RiskLevel = LEVEL_NOTICE;
        else if (EquivalentDB < 135.0f)     RiskLevel = LEVEL_WARNING;
        else                                RiskLevel = LEVEL_DANGER;

        FFTReady = true;
    }
}

bool FFT_IsReady(void)
{
    if (FFTReady)
    {
        FFTReady = false;

        /* ── 滑动重叠: 75%叠加, 256新样本→1.28s刷新 ── */
        uint16_t keepCount = FFT_SAMPLES - OVERLAP_SHIFT;
        memmove(vReal, vReal + OVERLAP_SHIFT, keepCount * sizeof(double));
        SampleIndex = keepCount;

        return true;
    }
    return false;
}

float FFT_GetMainFrequency(void)
{
    /* 返回平滑后的频率 (更稳定) */
    return SmoothedFreq;
}

float FFT_GetRawFrequency(void)
{
    /* 返回原始插值频率 (响应更快但跳动大) */
    return MainFrequency;
}

float FFT_GetSNR(void)
{
    return PeakSNR;
}

float FFT_GetRMS(void)
{
    return PressureRMS;
}

float FFT_GetEquivalentDB(void)
{
    return EquivalentDB;
}

RiskLevel_t FFT_GetRiskLevel(void)
{
    return RiskLevel;
}
