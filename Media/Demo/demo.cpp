#include "media_api.h"
#include <stdio.h>

unsigned int sensorNumber = 1;

int main()
{
    int s32Ret = 0;
    MEDIA_VIDEO_DISP_S stVideoDisp = {0};

    s32Ret = Media_Init(sensorNumber);
    if (0 != s32Ret)
    {
        printf("Media_Init error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    stVideoDisp.bOpen = 1;
    stVideoDisp.stRect.x = 0;
    stVideoDisp.stRect.y = 0;
    stVideoDisp.stRect.w = 1024;
    stVideoDisp.stRect.h = 600;
    s32Ret = Media_SetVideoDisp(MEDIA_SENSOR_RGB, &stVideoDisp);
    if (0 != s32Ret)
    {
        printf("Media_SetVideoDisp error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return 0;
}