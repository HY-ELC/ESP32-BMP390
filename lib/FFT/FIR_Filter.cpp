#include "FIR_Filter.h"



/*
    二阶IIR带通状态

*/

static float x1=0;
static float x2=0;

static float y1=0;
static float y2=0;



void FIR_Init()
{

    x1=0;
    x2=0;

    y1=0;
    y2=0;

}




float FIR_Process(float input)
{


    /*
        二阶带通滤波

        Fs=100Hz

        通带:
        0.1Hz~20Hz


    */


    float output;



    output =
        0.3913f*input
        +0.7826f*x1
        +0.3913f*x2
        -0.3695f*y1
        +0.1958f*y2;



    x2=x1;

    x1=input;


    y2=y1;

    y1=output;



    return output;

}