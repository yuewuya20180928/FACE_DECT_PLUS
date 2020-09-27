#include "media_api.h"
#include "sys_init.h"
#include "vi.h"
#include "isp.h"
#include "vpss.h"
#include "module.h"
#include "vo.h"

MEDIA_SENSOR_E sensorIdx[MAX_SENSOR_NUMBER] =
{
    MEDIA_SENSOR_RGB,
    MEDIA_SENSOR_IR
};

/* 初始化媒体库 */
int Media_Init(unsigned int sensorNumber)
{
    unsigned int i = 0;
    int ret = 0;
    MEDIA_LCD_IDX_E lcdTypeIdx = LCD_IDX_3_480X640;     //屏幕类型根据实际情况进行设定,媒体需要兼容

    MEDIA_VI_PARAM_S stViParam = {0};
    MEDIA_BIND_INFO_S stSrcInfo = {0};

    unsigned int imageWidth = 1920;
    unsigned int imageHeight = 1080;
    unsigned int vb_count = 12;
    MEDIA_POOL_S stPoolCfg = {0};

    ret = Media_VideoIn_GetConfig(&stViParam, sensorNumber);
    if (HI_SUCCESS != ret)
    {
        prtMD("Media_VideoIn_GetConfig error! ret = %#x\n", ret);
        return -1;
    }

    ret = Media_VideoIn_GetSnsWH(stViParam.stViInfo[0].stSnsInfo.enSnsType, &imageWidth, &imageHeight);
    if (HI_SUCCESS != ret)
    {
        prtMD("Media_VideoIn_GetSnsWH error! ret = %#x\n", ret);
        return -1;
    }

    prtMD("imageWidth = %d, imageheight = %d\n", imageWidth, imageHeight);

    stPoolCfg.poolNum = 2;
    stPoolCfg.stVbInfo[0].width = imageWidth;
    stPoolCfg.stVbInfo[0].height = imageHeight;
    stPoolCfg.stVbInfo[0].count = vb_count;

    stPoolCfg.stVbInfo[1].width = imageWidth;
    stPoolCfg.stVbInfo[1].height = imageHeight;
    stPoolCfg.stVbInfo[1].count = vb_count;

    ret = Media_Sys_Init(&stPoolCfg);
    if (HI_SUCCESS != ret)
    {
        prtMD("HI_Media_Sys_Init error! ret = %#x\n", ret);
        return -1;
    }

    /* 初始化VI模块 */
    ret = Media_VideoIn_Init(&stViParam);
    if (HI_SUCCESS != ret)
    {
        prtMD("Media_VideoIn_Init error! ret = %#x\n", ret);
        return -1;
    }

    /* 初始化ISP模块 */
    ret = Media_Isp_Init(&stViParam);
    if (HI_SUCCESS != ret)
    {
        prtMD("Media_Isp_Init error! ret = %#x\n", ret);
        return -1;
    }

    /* 初始化VPSS */
    for (i = 0; i < sensorNumber; i++)
    {
        ret = Media_Vpss_InitGroup(sensorIdx[i], imageWidth, imageHeight);
        if (HI_SUCCESS != ret)
        {
            prtMD("Media_Vpss_InitGroup error! ret = %#x\n", ret);
            return -1;
        }
    }

    /* VI绑定VPSS */
    for (i = 0; i < sensorNumber; i++)
    {
        stSrcInfo.enModule = HI_VI;
        stSrcInfo.devNo = i;
        stSrcInfo.chnNo = 0;
        ret = Media_Vpss_BindVi(sensorIdx[i], &stSrcInfo);
        if (HI_SUCCESS != ret)
        {
            prtMD("Media_Vpss_BindVi error! ret = %#x\n", ret);
            return -1;
        }
    }

    /* 初始化VO */
    ret = Media_Vo_Init(lcdTypeIdx);
    if (HI_SUCCESS != ret)
    {
        prtMD("Media_Vo_Init error! ret = %#x\n", ret);
        return -1;
    }

    return 0;
}

/* 初始化算法 */
int TL_ALG_Init(void)
{
    return 0;
}

/* 设置视频显示状态 */
int Media_SetVideoDisp(MEDIA_SENSOR_E sensorIdx, MEDIA_VIDEO_DISP_S *pDispParam)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VO_LAYER voLayer = 0;
    VO_CHN voChn = 0;
    MEDIA_BIND_INFO_S stDst = {0};

    if (pDispParam == NULL)
    {
        prtMD("invalid input pDispParam = %p\n", pDispParam);
        return -1;
    }

    if (sensorIdx >= MEDIA_SENSOR_BUTT)
    {
        prtMD("invalid input sensorIdx = %d\n", sensorIdx);
        return -1;
    }

    if (pDispParam->bOpen == 1)
    {
        s32Ret = Media_Vpss_StartChn(sensorIdx, VPSS_CHN_FOR_VO, &pDispParam->stRect);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }

        s32Ret = Media_Vo_StartChn(voLayer, voChn, pDispParam);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }

        stDst.enModule = HI_VO;
        stDst.devNo = voLayer;
        stDst.chnNo = voChn;

        s32Ret = Media_Vpss_BindVo(MEDIA_SENSOR_RGB, &stDst);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }
    }
    else if (pDispParam->bOpen == 0)
    {
#if 0
        s32Ret = Media_Vo_StopChn();
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }

        s32Ret = Media_Vpss_StopChn();
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }

        s32Ret = Media_Sys_UnBind();
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vpss_StartChn error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }
#endif
    }
    else
    {
        prtMD("invalid input param! bOpen = %d\n", pDispParam->bOpen);
        return -1;
    }

    return HI_SUCCESS;
}

