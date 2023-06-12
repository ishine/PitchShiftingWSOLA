
/**************************************************START OF FILE*****************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

    /*------------------------------------------------------------------------------------------------------------------
    Includes
    */
#include "stdint.h"
#include "math.h"//ZykeeAdd
#include "string.h"
#include "stdlib.h"

#include "xALG_FX_Tremolo.h"
#include "xALG_WSOLA_pitch_shift2.h"//ZykeeAdd

    float parm[10] = { 50,50,50,50,50,50,50,50,50,50 };


    /*------------------------------------------------------------------------------------------------------------------
    Code
    */
    void Input_BufferChange(float* Input_r, float* Input_l, float* Output, uint32_t Len);
    void Output_BufferChange(float* Output_r, float* Output_l, float* Input, uint32_t Len);




    /*
    *********************************************************************************************************************
    @ Brief  :

    @ Param  : NONE

    @ Return : NONE

    @ Author : YWJ(QQ:872180981)

    @ Data   : 2022-07-18 14:42
    *********************************************************************************************************************
    */
    struct TD ZyTD;
    void xALG_Basic_Init(void)
    {
        xALG_FxTremolo_Init();
        TDSet(&ZyTD);
        clearInput(&ZyTD);
        double Pitch = -12;
        TDsetTempo(Pitch, &ZyTD);
    }




    /*
    *********************************************************************************************************************
    @ Brief  :

    @ Param  : NONE

    @ Return : NONE

    @ Author : YWJ(QQ:872180981)

    @ Data   : 2021-10-04 17:59
    *********************************************************************************************************************
    */
    double fract = 0;
    int LostSamples = 0;
    int count = 0;
    static float TempBuffer[8 * 1024];
    float TransformBuffer[16 * 1024];
    float LenPut[16 * 1024];
    int a = 0;
    int b = 0;

    void xALG_Basic_Deal(float* InputR, float* InputL, float* Output_R, float* Output_L, uint32_t Len)
    {
        float* p = TempBuffer;
        float* q = TransformBuffer;
        float* Out = LenPut;

        Input_BufferChange((float*)InputR, (float*)InputL, p + LostSamples * 2, Len);
        int num = transposeStereoLinear(q + count, TempBuffer, Len + LostSamples, &fract, ZyTD.rate, &LostSamples);
        TempBuffer[0] = TempBuffer[1] = TempBuffer[Len * 2];
        int result = CalTD(&ZyTD, num + count / 2, q, Out + b, &count);
        if (result != 0 && a < 1) {
            a++;
            b += result;
        }
        else if (a >= 1)
        {
            b += result;
            Output_BufferChange(Output_R, Output_L, LenPut, Len);
            memcpy(Out, Out + Len * 2, sizeof(SAMPLETYPE) * (b - Len * 2));
            memset(Out + (b - Len * 2), 0, sizeof(SAMPLETYPE) * Len * 2);
            b -= Len * 2;
        }
    }





    /*
    *********************************************************************************************************************
    @ Brief  : 左右声道交替

    @ Param  : NONE

    @ Return : NONE

    @ Author : YWJ(QQ:872180981)

    @ Data   : 2021-07-09 17:30
    *********************************************************************************************************************
    */
    void Input_BufferChange(float* Input_r, float* Input_l, float* Output, uint32_t Len)
    {
        for (uint32_t i = 0; i < Len; i++)
        {
            Output[2 * i + 0] = Input_r[i];
            Output[2 * i + 1] = Input_l[i];
        }

    }
    /*
    *********************************************************************************************************************
    @ Brief  : 左右声道分开

    @ Param  : NONE

    @ Return : NONE

    @ Author : YWJ(QQ:872180981)

    @ Data   : 2021-07-09 17:30
    *********************************************************************************************************************
    */
    void Output_BufferChange(float* Output_r, float* Output_l, float* Input, uint32_t Len)
    {
        for (uint32_t i = 0; i < Len; i++)
        {
            Output_r[i] = Input[2 * i];
            Output_l[i] = Input[2 * i + 1];
        }
    }








#ifdef __cplusplus
}
#endif

/**************************************************END OF FILE*******************************************************/

