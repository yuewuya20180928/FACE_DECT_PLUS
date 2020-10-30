#include "media_api.h"
#include "sys_init.h"
#include "vi.h"
#include "isp.h"
#include "vpss.h"
#include "module.h"
#include "vo.h"
#include "enc.h"

SENSOR_TYPE_E sensorIdx[MAX_SENSOR_NUMBER] =
{
    SENSOR_TYPE_RGB,
    SENSOR_TYPE_IR
};

/* 初始化媒体库 */
int Media_Init(INIT_PARAM_S *pSensorParam)
{
    unsigned int i = 0;
    int ret = 0;
    unsigned int sensorNumber = 0;

    if (pSensorParam == NULL)
    {
        prtMD("invalid input pSensorParam = %p\n", pSensorParam);
        return -1;
    }

    //屏幕类型后继由应用应用,媒体需要兼容所有产品支持的屏幕类型
    MEDIA_LCD_IDX_E lcdTypeIdx = pSensorParam->enLcdIdx;

    MEDIA_VI_PARAM_S stViParam = {0};
    MEDIA_BIND_INFO_S stSrcInfo = {0};

    unsigned int imageWidth = 1920;
    unsigned int imageHeight = 1080;
    unsigned int vb_count = 12;
    MEDIA_POOL_S stPoolCfg = {0};

    prtMD("Date:%s, Time:%s\n", __DATE__, __TIME__);

    ret = Media_VideoIn_GetConfig(&stViParam, &pSensorParam->stSensorParam);
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

    sensorNumber = pSensorParam->stSensorParam.sensorNumber;

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
int Media_SetVideoDisp(SENSOR_TYPE_E sensorIdx, MEDIA_VIDEO_DISP_S *pDispParam)
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

    if (sensorIdx >= SENSOR_TYPE_BUTT)
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

        s32Ret = Media_Vpss_BindVo(SENSOR_TYPE_RGB, &stDst);
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

/* 设置录像参数 */
int Media_SetRecord(SENSOR_TYPE_E sensorIdx, VIDEO_STREAM_E videoStreamType, MEDIA_RECORD_S *pRecord)
{
    int ret = 0;
    MEDIA_RECT_S stRect = {0};
    VPSS_CHN vpssChan = 0;
    VPSS_GRP vpssGrp = 0;
    unsigned int vencChan = 0;
    VIDEO_PARAM_S *pVideoParam = NULL;
    MPP_CHN_S stSrcChn = {0};
    MPP_CHN_S stDstChn = {0};

    if (sensorIdx >= MAX_SENSOR_NUMBER)
    {
        prtMD("invalid input sensorIdx = %d\n", sensorIdx);
        return -1;
    }

    if (videoStreamType != VIDEO_STREAM_MAIN && videoStreamType != VIDEO_STREAM_SUB)
    {
        prtMD("invalid input videoStreamType = %d\n", videoStreamType);
        return -1;
    }

    if (pRecord == NULL)
    {
        prtMD("invalid input pRecord = %p\n", pRecord);
        return -1;
    }

    if (sensorIdx == SENSOR_TYPE_IR)
    {
        vpssGrp = 1;
    }
    else
    {
        vpssGrp = 0;
    }

    /* 视频参数设置 */
    if (pRecord->streamType | STREAM_TYPE_VIDEO)
    {
        pVideoParam = &pRecord->stVideoParam;

        /* 设置VPSS通道号 */
        if (videoStreamType == VIDEO_STREAM_SUB)
        {
            vpssChan = VPSS_CHN_FOR_ENC_SUB;
        }
        else
        {
            vpssChan = VPSS_CHN_FOR_ENC_MAIN;
        }

        prtMD("vpssChan = %d\n", vpssChan);

        if (pVideoParam->bOpen == true)
        {
            /* 开启VPSS通道 */
            stRect.x = 0;
            stRect.y = 0;
            stRect.w = pVideoParam->w;
            stRect.h = pVideoParam->h;
            ret = Media_Vpss_StartChn(sensorIdx, vpssChan, &stRect);
            if (ret != 0)
            {
                prtMD("Media_Vpss_StartChn error! ret = %#x\n", ret);
            }

            ret = Media_Module_GetChan(HI_ENC, &vencChan);
            if (0 != ret)
            {
                prtMD("Media_Module_GetChan HI_ENC error!\n");
                return -1;
            }

            prtMD("vencChan = %d\n", vencChan);

            /* 设置编码模块 */
            ret = Media_Enc_StartVideo(vencChan, pVideoParam);
            if (ret != 0)
            {
                prtMD("Media_Enc_SetVideo error! ret = %#x\n", ret);
            }

            /* VPSS绑定VENC */
            stSrcChn.enModId = HI_ID_VPSS;
            stSrcChn.s32DevId = vpssGrp;
            stSrcChn.s32ChnId = vpssChan;

            stDstChn.enModId = HI_ID_VENC;
            stDstChn.s32DevId = 0;
            stDstChn.s32ChnId = vencChan;

            ret = HI_MPI_SYS_Bind(&stSrcChn, &stDstChn);
            if (0 != ret)
            {
                prtMD("HI_MPI_SYS_Bind error! ret = %#x\n", ret);
            }

            ret = Media_Enc_StartTsk(vencChan);
            if (0 != ret)
            {
                prtMD("Media_Enc_StartTsk error! ret = %#x\n", ret);
            }
        }
        else if (pVideoParam->bOpen == false)
        {
            /* 1.解绑VPSS->VENC */

            /* 2.关闭VENC */

            /* 3.关闭VPSS */

        }
    }

    /* 音频参数设置 */
    if (pRecord->streamType | STREAM_TYPE_AUDIO)
    {
        ret = Media_Enc_SetAudio(sensorIdx, videoStreamType, &pRecord->stAudioParam);
        if (ret != 0)
        {
            prtMD("Media_Enc_SetAudio error! ret = %#x\n", ret);
        }
    }

    return 0;
}

