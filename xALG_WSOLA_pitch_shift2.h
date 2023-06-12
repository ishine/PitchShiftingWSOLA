#pragma once
typedef float SAMPLETYPE;
typedef unsigned int uint;
typedef int BOOL;
typedef unsigned long   ulong;
typedef ulong ulongptr;
typedef float LONG_SAMPLETYPE;

#define _CRT_SECURE_NO_WARNINGS£»
#define DEFAULT_OVERLAP_MS      9
#define USE_AUTO_SEEKWINDOW_LEN     0
#define TRUE 1
#define FALSE 0
#define FLT_MIN 1.175494351e-38F 
#define FLT_MAX 3.402823466e+38F 
#define PI       3.14159265358979323846
#define TWOPI    (2 * PI)
#define SOUNDTOUCH_ALIGN_POINTER_16(x)      ( ( (ulongptr)(x) + 15 ) & ~(ulongptr)15 )   
#define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))
static const float _coeffs[] =
{ -0.5f,  1.0f, -0.5f, 0.0f,
   1.5f, -2.5f,  0.0f, 1.0f,
  -1.5f,  2.0f,  0.5f, 0.0f,
   0.5f, -0.5f,  0.0f, 0.0f };

struct TD {
    int sampleReq;
    int overlapLength;
    int seekLength;
    int seekWindowLength;
    int overlapDividerBitsNorm;
    int overlapDividerBitsPure;
    int slopingDivider;
    int sampleRate;
    float sequenceMs;
    int seekWindowMs;
    int overlapMs;
    unsigned long maxnorm;
    float maxnormf;

    double tempo;
    double rate;
    double nominalSkip;
    double skipFract;

    BOOL isBeginning;

    SAMPLETYPE* pMidBuffer;
    SAMPLETYPE* pMidBufferUnaligned;
};

void TDSet(struct TD* ZyWSOLA);
void clearInput(struct TD* ZyWSOLA);
void TDsetTempo(double newTempo, struct TD* ZyWSOLA);
int transposeStereoLinear(SAMPLETYPE* dest, const SAMPLETYPE* src, int srcSamples, double* fract, double rate, int* SampleLost);
int CalTD(struct TD* ZyWSOLA, int numSamples, float* p, float* q, int* count);
double calcCrossCorr(const float* mixingPos, const float* compare, double* anorm, struct TD* ZyWSOLA);
void calcSeqParameters(struct TD* ZyWSOLA);
void clearMidBuffer(struct TD* ZyTD);
