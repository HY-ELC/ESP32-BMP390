#include "FFT_Process.h"

#include <arduinoFFT.h>

#include <math.h>



static float vReal[FFT_SIZE];

static float vImag[FFT_SIZE];



static uint16_t sampleCount=0;



static bool ready=false;



static float mainFrequency=0;


static float rmsValue=0;


static float snrValue=0;




ArduinoFFT<float> FFT(
    vReal,
    vImag,
    FFT_SIZE,
    FFT_SAMPLE_RATE
);





void FFT_Init()
{

    sampleCount=0;

    ready=false;


    mainFrequency=0;

    rmsValue=0;

    snrValue=0;



    for(int i=0;i<FFT_SIZE;i++)
    {
        vReal[i]=0;
        vImag[i]=0;
    }

}







void FFT_AddSample(float sample)
{

    vReal[sampleCount]=sample;

    vImag[sampleCount]=0;


    sampleCount++;


    if(sampleCount>=FFT_SIZE)
    {

        sampleCount=0;

        ready=true;

    }

}







bool FFT_IsReady()
{

    if(!ready)
        return false;


    ready=false;



    /*
        去除DC

    */


    float mean=0;


    for(int i=0;i<FFT_SIZE;i++)
    {
        mean+=vReal[i];
    }


    mean/=FFT_SIZE;



    float sum=0;



    for(int i=0;i<FFT_SIZE;i++)
    {

        vReal[i]-=mean;


        sum+=
        vReal[i]*vReal[i];


        vImag[i]=0;

    }



    rmsValue =
    sqrt(sum/FFT_SIZE);



    /*
        FFT窗函数

    */

    FFT.windowing(
        FFTWindow::Hann,
        FFTDirection::Forward
    );


    FFT.compute(
        FFTDirection::Forward
    );


    FFT.complexToMagnitude();



    /*
        搜索0.2~20Hz

    */


    uint16_t startBin =
    ceil(
    0.2f/
    (FFT_SAMPLE_RATE/FFT_SIZE)
    );


    uint16_t endBin =
    floor(
    20.0f/
    (FFT_SAMPLE_RATE/FFT_SIZE)
    );



    float maxValue=0;


    uint16_t maxIndex=startBin;



    for(
        uint16_t i=startBin;
        i<=endBin;
        i++
    )
    {

        if(vReal[i]>maxValue)
        {

            maxValue=vReal[i];

            maxIndex=i;

        }

    }




    /*
        抛物线插值

        提高频率精度

    */


    float offset=0;



    if(
        maxIndex>1 &&
        maxIndex<FFT_SIZE-1
    )
    {

        float a=vReal[maxIndex-1];

        float b=vReal[maxIndex];

        float c=vReal[maxIndex+1];



        float d =
        a-2*b+c;



        if(
        fabs(d)>0.001
        )
        {

            offset=
            0.5f*
            (a-c)/
            d;

        }

    }



    mainFrequency =
    (
    maxIndex+offset
    )
    *
    FFT_SAMPLE_RATE
    /
    FFT_SIZE;





    /*
        频谱SNR

    */


    float noise=0;


    uint16_t count=0;



    for(
        uint16_t i=startBin;
        i<=endBin;
        i++
    )
    {

        if(i!=maxIndex)
        {

            noise+=vReal[i];

            count++;

        }

    }



    if(count)
        noise/=count;



    if(noise>0)
        snrValue=
        maxValue/noise;

    else
        snrValue=0;



    return true;

}





float FFT_GetMainFrequency()
{

    return mainFrequency;

}



float FFT_GetRMS()
{

    return rmsValue;

}



float FFT_GetSNR()
{

    return snrValue;

}