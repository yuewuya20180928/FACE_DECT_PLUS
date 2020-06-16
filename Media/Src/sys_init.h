#ifndef __SYS_INIT_H__
#define __SYS_INIT_H__

#include "media_com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VB_WIDTH 1920
#define MAX_VB_HEIGHT 1080
#define MAX_VB_COUNT 15
#define MAX_POOL_COUNT 3

typedef struct
{
    unsigned int width;
    unsigned int height;
    unsigned int count;
}MEDIA_VB_S;


typedef struct
{
    MEDIA_VB_S stVbInfo[MAX_POOL_COUNT];
    unsigned int poolNum;
}MEDIA_POOL_S;


int Media_Sys_Init(MEDIA_POOL_S *pVb);


#ifdef __cplusplus
}
#endif



#endif

