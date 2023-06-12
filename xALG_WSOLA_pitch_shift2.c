#include "stdint.h"
#include "math.h"//ZykeeAdd
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "xALG_WSOLA_pitch_shift2.h"

void acceptNewOverlapLength(int newOverlapLength, struct TD* ZyTD)
{
    int prevOvl;

    prevOvl = ZyTD->overlapLength;

    ZyTD->overlapLength = newOverlapLength;
    ZyTD->pMidBufferUnaligned = (SAMPLETYPE*)malloc(sizeof(SAMPLETYPE) * (ZyTD->overlapLength * 2 + 16 / sizeof(SAMPLETYPE)));
    ZyTD->pMidBuffer = (SAMPLETYPE*)SOUNDTOUCH_ALIGN_POINTER_16(ZyTD->pMidBufferUnaligned);
    clearMidBuffer(ZyTD);
}

static int _getClosest2Power(double value)
{
    return (int)(log(value) / log(2.0) + 0.5);
}
void calculateOverlapLength(int aoverlapMs, struct TD* ZyTD)
{
    int newOvl;

    ZyTD->overlapDividerBitsPure = _getClosest2Power((ZyTD->sampleRate * aoverlapMs) / 1000.0) - 1;
    if (ZyTD->overlapDividerBitsPure > 9) ZyTD->overlapDividerBitsPure = 9;
    if (ZyTD->overlapDividerBitsPure < 3) ZyTD->overlapDividerBitsPure = 3;
    newOvl = (int)pow(2.0, (int)ZyTD->overlapDividerBitsPure + 1);    // +1 => account for -1 above

    acceptNewOverlapLength(newOvl, ZyTD);

    ZyTD->overlapDividerBitsNorm = ZyTD->overlapDividerBitsPure;

    ZyTD->slopingDivider = (newOvl * newOvl - 1) / 3;
}

void setParameters(int aSampleRate, int aSequenceMS, int aSeekWindowMS, int aOverlapMS, struct TD* ZyWSOLA)
{
    if (aSampleRate > 0) ZyWSOLA->sampleRate = aSampleRate; //sampleRate=44100

    if (aOverlapMS > 0) ZyWSOLA->overlapMs = aOverlapMS;

    calcSeqParameters(ZyWSOLA);
    calculateOverlapLength(ZyWSOLA->overlapMs, ZyWSOLA);
    TDsetTempo(ZyWSOLA->tempo, ZyWSOLA);
}

void TDSet(struct TD* ZyWSOLA)
{
    ZyWSOLA->overlapLength = 0;
    ZyWSOLA->pMidBuffer = NULL;
    ZyWSOLA->pMidBufferUnaligned = NULL;
    ZyWSOLA->tempo = 1.0f;
    setParameters(44100, 0, 0, DEFAULT_OVERLAP_MS, ZyWSOLA);
}

void clearInput(struct TD* ZyWSOLA)
{
    clearMidBuffer(ZyWSOLA);
    ZyWSOLA->isBeginning = TRUE;
    ZyWSOLA->maxnorm = 0;
    ZyWSOLA->maxnormf = 1e8;
    ZyWSOLA->skipFract = 0;
}
void clearMidBuffer(struct TD* ZyWSOLA)
{
    memset(ZyWSOLA->pMidBuffer, 0, 2 * sizeof(SAMPLETYPE) * ZyWSOLA->overlapLength);
}

int transposeStereoLinear(SAMPLETYPE* dest, const SAMPLETYPE* src, int srcSamples, double* fract, double rate, int* SampleLost)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        double out0, out1;
        out0 = (1.0 - (*fract)) * src[0] + (*fract) * src[2];
        out1 = (1.0 - (*fract)) * src[1] + (*fract) * src[3];
        dest[2 * i] = (SAMPLETYPE)out0;
        dest[2 * i + 1] = (SAMPLETYPE)out1;
        i++;
        (*fract) += rate;
        int whole = (int)(*fract);
        (*fract) -= whole;
        src += 2 * whole;
        srcCount += whole;
    }
    *SampleLost = srcSamples - srcCount;
    return i;
}

void calcSeqParameters(struct TD* ZyWSOLA)
{
    ZyWSOLA->seekWindowLength = 1800;
    if (ZyWSOLA->seekWindowLength < 2 * ZyWSOLA->overlapLength)
    {
        ZyWSOLA->seekWindowLength = 2 * ZyWSOLA->overlapLength;
    }
    ZyWSOLA->seekLength = 1300;
}

void TDsetTempo(double Pitch, struct TD* ZyWSOLA)
{
    int intskip;
    ZyWSOLA->rate = exp(0.69314718056 * (Pitch / 12.0));
    ZyWSOLA->tempo = 1 / ZyWSOLA->rate;
    calcSeqParameters(ZyWSOLA);
    ZyWSOLA->nominalSkip = ZyWSOLA->tempo * ((ZyWSOLA->seekWindowLength) - (ZyWSOLA->overlapLength));
    intskip = (int)(ZyWSOLA->nominalSkip + 0.5);
    ZyWSOLA->sampleReq = max(intskip + (ZyWSOLA->overlapLength), ZyWSOLA->seekWindowLength) + ZyWSOLA->seekLength;
}

int seekBestOverlapPositionQuick(const SAMPLETYPE* refPos, struct TD* ZyWSOLA)
{
#define _MIN(a, b)   (((a) < (b)) ? (a) : (b))
#define SCANSTEP    16
#define SCANWIND    8

    int bestOffs;
    int i;
    int bestOffs2;
    float bestCorr, corr;
    float bestCorr2;
    double norm;

    bestCorr = bestCorr2 = -FLT_MAX;
    bestOffs = bestOffs2 = SCANWIND;

    for (i = SCANSTEP; i < ZyWSOLA->seekLength - SCANWIND - 1; i += SCANSTEP)
    {
        corr = (float)calcCrossCorr(refPos + 2 * i, ZyWSOLA->pMidBuffer, &norm, ZyWSOLA);
        float tmp = (float)(2 * i - ZyWSOLA->seekLength - 1) / (float)ZyWSOLA->seekLength;
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));

        if (corr > bestCorr)
        {
            bestCorr2 = bestCorr;
            bestOffs2 = bestOffs;
            bestCorr = corr;
            bestOffs = i;
        }
        else if (corr > bestCorr2)
        {
            bestCorr2 = corr;
            bestOffs2 = i;
        }
    }

    int end = _MIN(bestOffs + SCANWIND + 1, ZyWSOLA->seekLength);
    for (i = bestOffs - SCANWIND; i < end; i++)
    {
        if (i == bestOffs) continue; 
        corr = (float)calcCrossCorr(refPos + 2 * i, ZyWSOLA->pMidBuffer, &norm, ZyWSOLA);
        float tmp = (float)(2 * i - ZyWSOLA->seekLength - 1) / (float)ZyWSOLA->seekLength;
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestOffs = i;
        }
    }

    end = _MIN(bestOffs2 + SCANWIND + 1, ZyWSOLA->seekLength);
    for (i = bestOffs2 - SCANWIND; i < end; i++)
    {
        if (i == bestOffs2) continue; 
        corr = (float)calcCrossCorr(refPos + 2 * i, ZyWSOLA->pMidBuffer, &norm, ZyWSOLA);
        float tmp = (float)(2 * i - (ZyWSOLA->seekLength) - 1) / (float)(ZyWSOLA->seekLength);
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestOffs = i;
        }
    }
    return bestOffs;
}

double calcCrossCorr(const float* mixingPos, const float* compare, double* anorm, struct TD* ZyWSOLA)
{
    float corr;
    float norm;
    int i;
    int ilength = (2 * ZyWSOLA->overlapLength) & -8;
    corr = norm = 0;
    for (i = 0; i < ilength; i++)
    {
        corr += mixingPos[i] * compare[i];
        norm += mixingPos[i] * mixingPos[i];
    }

    *anorm = norm;
    return corr / sqrt((norm < 1e-9 ? 1.0 : norm));
}

void overlapStereo(float* pOutput, const float* pInput, struct TD* ZyWSOLA)
{
    int i;
    float fScale;
    float f1;
    float f2;
    fScale = 1.0f / (float)ZyWSOLA->overlapLength;
    f1 = 0;
    f2 = 1.0f;
    for (i = 0; i < 2 * (int)ZyWSOLA->overlapLength; i += 2)
    {
        pOutput[i + 0] = pInput[i + 0] * f1 + ZyWSOLA->pMidBuffer[i + 0] * f2;
        pOutput[i + 1] = pInput[i + 1] * f1 + ZyWSOLA->pMidBuffer[i + 1] * f2;
        f1 += fScale;
        f2 -= fScale;
    }
}


int CalTD(struct TD* ZyWSOLA, int numSamples, float* Input, float* Output, int* count)
{
    int ovlSkip;
    int offset = 0;
    int temp = 0;
    int a = 0;
    int b = 0;
    float* pInput = Input;
    while (1)
    {
        if (numSamples * 2 - a <= 2 * ZyWSOLA->sampleReq)
        {
            if (pInput != Input)
            {
                memcpy(pInput, Input, sizeof(float) * (numSamples * 2 - a));
                pInput += numSamples * 2 - a;
                memset(pInput, 0, sizeof(float) * a);
            }
            *count = numSamples * 2 - a;
            break;
        }
        if (ZyWSOLA->isBeginning == TRUE) {
            ZyWSOLA->isBeginning = FALSE;
            int skip = (int)(ZyWSOLA->tempo * ZyWSOLA->overlapLength + 0.5 * ZyWSOLA->seekLength + 0.5);
            ZyWSOLA->skipFract -= skip;
            if ((ZyWSOLA->skipFract) <= -(ZyWSOLA->nominalSkip))
                (ZyWSOLA->skipFract) = -(ZyWSOLA->nominalSkip);
        }
        else {
            offset = seekBestOverlapPositionQuick(Input, ZyWSOLA);
            overlapStereo(Output, Input + offset * 2, ZyWSOLA, ZyWSOLA->pMidBuffer);
            Output += 2 * (uint)ZyWSOLA->overlapLength;
            b += 2 * (uint)ZyWSOLA->overlapLength;
            offset += ZyWSOLA->overlapLength;
        }
        temp = (ZyWSOLA->seekWindowLength - 2 * ZyWSOLA->overlapLength);
        memcpy(Output, Input + 2 * offset, sizeof(float) * (uint)temp * 2);
        Output = Output + 2 * temp;
        b += 2 * temp;
        memcpy(ZyWSOLA->pMidBuffer, Input + 2 * (temp + offset), 2 * sizeof(float) * ZyWSOLA->overlapLength);
        ZyWSOLA->skipFract += ZyWSOLA->nominalSkip;
        ovlSkip = (int)ZyWSOLA->skipFract;
        ZyWSOLA->skipFract -= ovlSkip;
        Input = Input + (uint)ovlSkip * 2;
        a += (uint)ovlSkip * 2;
    }
    return b;
}