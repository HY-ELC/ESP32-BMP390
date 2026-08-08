#ifndef FFT_PROCESS_H
#define FFT_PROCESS_H


#include <Arduino.h>


#define FFT_SIZE 512


#define FFT_SAMPLE_RATE 100.0f



void FFT_Init();



void FFT_AddSample(float sample);



bool FFT_IsReady();



float FFT_GetMainFrequency();



float FFT_GetRMS();



float FFT_GetSNR();



#endif