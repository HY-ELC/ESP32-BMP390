#ifndef BMP390_H
#define BMP390_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP3XX.h>


#define BMP390_SDA 18
#define BMP390_SCL 19


#define BMP390_I2C_ADDR 0x77


#define BMP390_SAMPLE_RATE 100



bool BMP390_Init();


bool BMP390_Update();



float BMP390_GetPressure();



float BMP390_GetTemperature();



float BMP390_GetDeltaPressure();



float BMP390_GetBaseline();



void BMP390_ResetBaseline();



#endif