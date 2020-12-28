#ifndef __API_H__
#define __API_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 主机命令 */
typedef struct
{
    CMD_IDX_E cmdIdx;                   /* 命令号 */
    unsigned char bHaveBuf;             /* 是否有数据缓冲 */
    unsigned char bReturn;              /* 是否有返回值 */
    unsigned int chan;                  /* 通道号 */
    unsigned int param;                 /* 参数 */
    void *pBuf;                         /* 应用传递的参数的首地址 */
    unsigned int bufLenth;              /* 缓冲数据有效长度 */
}HOST_CMD_S;


int DSP_API_Init(void);

#ifdef __cplusplus
}
#endif



#endif


