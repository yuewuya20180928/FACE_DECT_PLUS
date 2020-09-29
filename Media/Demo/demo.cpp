#include "media_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned int sensorNumber = 1;

int main()
{
    int s32Ret = 0;
    MEDIA_RECORD_S stRecord = {0};
    VIDEO_PARAM_S *pVideoParam = &stRecord.stVideoParam;

    /* 显示区域位置设置 */
    unsigned int dispX = 0;
    unsigned int dispY = 0;
    unsigned int dispW = 640;   //1024;
    unsigned int dispH = 480;   //600;

    MEDIA_VIDEO_DISP_S stVideoDisp = {0};

    s32Ret = Media_Init(sensorNumber);
    if (0 != s32Ret)
    {
        printf("Media_Init error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    #if 1
    stVideoDisp.bOpen = 1;
    #else
    stVideoDisp.bOpen = 0;
    #endif

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

    /* 视频流 */
    stRecord.streamType = STREAM_TYPE_VIDEO;
    pVideoParam->enType = ENCODE_TYPE_H265;
    pVideoParam->w = 1600;
    pVideoParam->h = 1200;
    pVideoParam->fps = 1;
    pVideoParam->gop = 50;
    pVideoParam->bOpen = true;
    pVideoParam->bitRate = 4096;

    /* 开启RGB sensor的录像功能 */
    //s32Ret = Media_SetRecord(MEDIA_SENSOR_RGB, VIDEO_STREAM_MAIN, &stRecord);
    if (0 != s32Ret)
    {
        printf("Media_SetRecord error! s32Ret = %#x\n", s32Ret);
    }

    while (1)
    {
        usleep(100 * 1000);

        printf("goto sleep!\n");
        /* TODO:打印只打印一次,确认原因 */

        continue;
    }

    return 0;
}

