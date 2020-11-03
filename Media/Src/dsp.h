#ifndef __DSP_H__
#define __DSP_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    CMD_IDX_E enCmdIdx;
    unsigned int bHaveBuf;
    unsigned int bReturn;
    unsigned int bufLenth;
    HOST_DSP_CMD_FUNC pFunc;
}DSP_CMD_S;

int Media_Init(unsigned int chan, unsigned int param, void *pBuf);
int Media_SetVideoDisp(unsigned int chan, unsigned int param, void *pBuf);
int Media_SetRecord(unsigned int chan, unsigned int param, void *pBuf);



#ifdef __cplusplus
}
#endif



#endif


