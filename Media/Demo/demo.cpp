#include "media_api.h"
#include <stdio.h>

unsigned int sensorNumber = 1;

int main()
{
    int s32Ret = 0;

    /* 显示区域位置设置 */
    unsigned int dispX = 0;
    unsigned int dispY = 0;
    unsigned int dispW = 1024;
    unsigned int dispH = 600;

    MEDIA_VIDEO_DISP_S stVideoDisp = {0};

    s32Ret = Media_Init(sensorNumber);
    if (0 != s32Ret)
    {
        printf("Media_Init error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    stVideoDisp.bOpen = 1;
    stVideoDisp.stRect.x = dispX;
    stVideoDisp.stRect.y = dispY;
    stVideoDisp.stRect.w = dispW;
    stVideoDisp.stRect.h = dispH;
    stVideoDisp.enRotation = MEDIA_ROTATION_0;
    s32Ret = Media_SetVideoDisp(MEDIA_SENSOR_RGB, &stVideoDisp);
    if (0 != s32Ret)
    {
        printf("Media_SetVideoDisp error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return 0;
}