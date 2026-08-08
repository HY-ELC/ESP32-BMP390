#ifndef __TEST_TONE_H
#define __TEST_TONE_H

#include <Arduino.h>

/* 测试频率列表 (Hz) */
#define TONE_COUNT  6

void TestTone_Init(void);
void TestTone_Update(void);       /* 每个采样周期调用一次, 更新DAC */
void TestTone_NextFreq(void);     /* 切换到下一个测试频率 */
void TestTone_SetFreq(float hz);  /* 设置指定频率 */
float TestTone_GetFreq(void);     /* 获取当前频率 */
bool TestTone_IsEnabled(void);    /* 是否启用 */
void TestTone_Enable(bool en);    /* 启用/禁用 */

#endif
