#ifndef __MODULE_H__
#define __MODULE_H__



#ifdef __cplusplus
extern "C" {
#endif


#include <pthread.h>
#include <stdio.h>

#include "media_com.h"

/* 定义最大的硬件模块个数 */
#define MAX_HD_MODULE_NUM 16
#define MAX_POOL_NUM 16

/* 定义海思各硬件模块的枚举类型 */
typedef enum
{
    HI_MODULE_BASE = 0,
    HI_DEC,
    HI_ENC,
    HI_VPSS,
    HI_RGN,
    HI_VGS,
    HI_VI,
    HI_VO,

    HI_MODULE_BUTT
} MEDIA_MODULE_E;

/* 定义缓冲池信息 */
typedef struct
{
    MEDIA_MODULE_E module;                       /* 模块名称 */
    unsigned char bUsed[MAX_POOL_NUM];      /* 用于记录缓冲池中资源的消耗 */
} MEDIA_POOL_INFO;

/* 定义各模块缓冲池信息 */
typedef struct
{
    pthread_mutex_t mutex;                  /* 互斥锁 */
    MEDIA_POOL_INFO poolInfo[MAX_HD_MODULE_NUM];  /* 缓存池信息 */
    unsigned int NUM;                       /* 注册模块数 */
} MEDIA_MODULE_POOL_S;

typedef struct
{
    MEDIA_MODULE_E enModule;
    unsigned int devNo;
    unsigned int chnNo;
}MEDIA_BIND_INFO_S;

int Media_Module_GetChan(MEDIA_MODULE_E module, unsigned int *pChnValue);

int Media_Module_ReleaseChan(MEDIA_MODULE_E module, int modChan);

int Media_Module_Bind(MEDIA_BIND_INFO_S *pSrc, MEDIA_BIND_INFO_S *pDst);

#ifdef __cplusplus
}
#endif



#endif

