#include "BMP390.h"



Adafruit_BMP3XX bmp;


TwoWire BMP390_I2C = TwoWire(0);



static float pressureBaseline = 0;


static float currentPressure = 0;


static float currentTemperature = 0;



static bool baselineReady = false;



/*
    基线跟踪

    100Hz采样

    0.002:
    慢变化跟踪

    保留0.1Hz以上变化

*/

#define BASELINE_ALPHA 0.002f




bool BMP390_Init()
{

    BMP390_I2C.begin(
        BMP390_SDA,
        BMP390_SCL,
        400000
    );



    if(!bmp.begin_I2C(
        BMP390_I2C_ADDR,
        &BMP390_I2C
    ))
    {

        Serial.println(
            "[BMP390] ERROR"
        );

        return false;

    }



    /*
        压力过采样

        提高压力分辨率

    */

    bmp.setPressureOversampling(
        BMP3_OVERSAMPLING_8X
    );



    /*
        温度只用于补偿

    */

    bmp.setTemperatureOversampling(
        BMP3_OVERSAMPLING_2X
    );



    /*
        关闭内部IIR

        避免低频相位延迟

    */

    bmp.setIIRFilterCoeff(
        BMP3_IIR_FILTER_DISABLE
    );



    /*
        输出速率

        100Hz

    */

    bmp.setOutputDataRate(
        BMP3_ODR_100_HZ
    );



    baselineReady=false;



    Serial.println(
        "[BMP390] OK"
    );


    return true;

}





bool BMP390_Update()
{

    if(!bmp.performReading())
    {
        return false;
    }



    currentPressure =
        bmp.pressure;



    currentTemperature =
        bmp.temperature;



    /*
        初始化基线

    */

    if(!baselineReady)
    {

        pressureBaseline =
            currentPressure;


        baselineReady=true;

    }



    /*
        自动跟踪环境压力

        去除天气变化和手持漂移

    */

    pressureBaseline +=
        BASELINE_ALPHA *
        (
        currentPressure -
        pressureBaseline
        );



    return true;

}






float BMP390_GetPressure()
{

    return currentPressure;

}






float BMP390_GetTemperature()
{

    return currentTemperature;

}






float BMP390_GetBaseline()
{

    return pressureBaseline;

}






float BMP390_GetDeltaPressure()
{


    if(!baselineReady)
        return 0;



    float delta =
        currentPressure -
        pressureBaseline;



    /*
        防止FFT被冲击饱和

        正常次声:
        <10Pa

        敲击:
        可能几十Pa

    */

    if(delta>500)
        delta=500;



    if(delta<-500)
        delta=-500;



    return delta;

}






void BMP390_ResetBaseline()
{

    pressureBaseline =
        currentPressure;


    baselineReady=true;

}