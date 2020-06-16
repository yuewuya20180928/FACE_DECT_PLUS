#include "vi.h"
#include "mipi_rx.h"

//int SensorViDev[MAX_SENSOR_NUMBER] = {0,1};

//int SensorenWDRMode[MAX_SENSOR_NUMBER] = {WDR_MODE_2To1_LINE, WDR_MODE_2To1_LINE};

//int SensorViPipe[VI_MAX_PIPE_NUM] = {0, 1, 2, 3};

VI_DEV_ATTR_S DEV_ATTR_IMX327_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    VI_DATA_TYPE_RGB,
    HI_FALSE,
    {1920, 1080},
    {
        {
            {1920 , 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1
};

VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, HI_FALSE, HI_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    HI_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    HI_FALSE,
    { -1, -1}
};

VI_CHN_ATTR_S CHN_ATTR_1920x1080_420_SDR8_LINEAR =
{
    {1920, 1080},
    PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1}
};

int Media_VideoIn_GetConfig(MEDIA_VI_PARAM_S *pViParam, unsigned int sensorNum)
{
    unsigned int i = 0;
    unsigned int idx = 0;

    if (pViParam == NULL)
    {
        prtMD("ivnalid pViParam = %p\n", pViParam);
        return -1;
    }

    if (sensorNum > MAX_SENSOR_NUMBER)
    {
        prtMD("invalid input sensorNum = %d\n", sensorNum);
        return -1;
    }

    pViParam->s32WorkingViNum = sensorNum;

    for (i = 0; i < sensorNum; i++)
    {
        pViParam->s32WorkingViId[i] = i;

        #if 0
        if (SensorenWDRMode[i] == WDR_MODE_2To1_LINE)
        {
            pViParam->stViInfo[i].stSnsInfo.enSnsType = SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1;
        }
        else
        {
            pViParam->stViInfo[i].stSnsInfo.enSnsType = SONY_IMX327_MIPI_2M_30FPS_12BIT;
        }
        #else
        pViParam->stViInfo[i].stSnsInfo.enSnsType = SONY_IMX327_2L_MIPI_2M_30FPS_12BIT;
        #endif

        pViParam->stViInfo[i].stSnsInfo.MipiDev = i;
        pViParam->stViInfo[i].stSnsInfo.s32BusId = i;
        pViParam->stViInfo[i].stSnsInfo.s32SnsId = i;
        pViParam->stViInfo[i].stDevInfo.ViDev = i;                                      //SensorViDev[i];
        pViParam->stViInfo[i].stDevInfo.enWDRMode = WDR_MODE_NONE;                 //SensorenWDRMode[i];

        pViParam->stViInfo[i].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;

        pViParam->stViInfo[i].stPipeInfo.aPipe[0] = idx++;                              //SensorViPipe[idx++];
        if (pViParam->stViInfo[i].stDevInfo.enWDRMode == WDR_MODE_2To1_LINE)
        {
            pViParam->stViInfo[i].stPipeInfo.aPipe[1] = idx++;                          //SensorViPipe[idx++];
        }
        else
        {
            pViParam->stViInfo[i].stPipeInfo.aPipe[1] = -1;
        }

        pViParam->stViInfo[i].stPipeInfo.aPipe[2] = -1;
        pViParam->stViInfo[i].stPipeInfo.aPipe[3] = -1;

        prtMD("i = %d, snsType = %d, enWDRMode = %d, %d %d %d %d\n",
            i,
            pViParam->stViInfo[i].stSnsInfo.enSnsType,
            pViParam->stViInfo[i].stDevInfo.enWDRMode,
            pViParam->stViInfo[i].stPipeInfo.aPipe[0],
            pViParam->stViInfo[i].stPipeInfo.aPipe[1],
            pViParam->stViInfo[i].stPipeInfo.aPipe[2],
            pViParam->stViInfo[i].stPipeInfo.aPipe[3]);

        pViParam->stViInfo[i].stChnInfo.ViChn = 0;
        pViParam->stViInfo[i].stChnInfo.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pViParam->stViInfo[i].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
        pViParam->stViInfo[i].stChnInfo.enVideoFormat = VIDEO_FORMAT_LINEAR;
        pViParam->stViInfo[i].stChnInfo.enCompressMode = COMPRESS_MODE_NONE;
    }

    return HI_SUCCESS;
}

int Media_VideoIn_GetSnsWH(VI_SNS_TYPE_E enSnsType, unsigned int *pWidth, unsigned int *pHeight)
{
    if ((pWidth == NULL) || (pHeight == NULL))
    {
        prtMD("invalid input pWidth = %p, pHeight = %p\n", pWidth, pHeight);
        return -1;
    }

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            *pWidth = 1920;
            *pHeight = 1080;
            break;

        default:
            *pWidth = 1920;
            *pHeight = 1080;
    }

    return HI_SUCCESS;
}

int Media_VideoIn_StartMipiRx(MEDIA_VI_PARAM_S *pViParam)
{
    if (pViParam == NULL)
    {
        prtMD("invalid input pViParam = %p\n", pViParam);
        return -1;
    }

    return Media_MipiRx_Start(pViParam);
}

int Media_VideoIn_SetPipeMode(void)
{
    HI_S32              i = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_VPSS_MODE_S      stVIVPSSMode;

    memset(&stVIVPSSMode, 0, sizeof(stVIVPSSMode));

    s32Ret = HI_MPI_SYS_GetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_SYS_GetVIVPSSMode failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /* 默认采用离线模式 */
    for (i = 0; i < VI_MAX_PIPE_NUM; i++)
    {
        stVIVPSSMode.aenMode[i] = VI_OFFLINE_VPSS_OFFLINE;
    }

    s32Ret = HI_MPI_SYS_SetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_SYS_SetVIVPSSMode failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int Media_Vi_GetDevAttr(VI_SNS_TYPE_E enSnsType, VI_DEV_ATTR_S* pstViDevAttr)
{
    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            hi_memcpy(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX327_2M_BASE, sizeof(VI_DEV_ATTR_S));
            break;

        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            hi_memcpy(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX327_2M_BASE, sizeof(VI_DEV_ATTR_S));
            pstViDevAttr->au32ComponentMask[0] = 0xFFC00000;
            break;

        default:
            hi_memcpy(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX327_2M_BASE, sizeof(VI_DEV_ATTR_S));
    }

    return HI_SUCCESS;
}

int Media_Vi_StartDev(MEDIA_VI_INFO_S *pViInfo)
{
    HI_S32              s32Ret = HI_SUCCESS;
    VI_DEV              ViDev = 0;
    VI_SNS_TYPE_E       enSnsType = VI_SNS_TYPE_BUTT;
    VI_DEV_ATTR_S       stViDevAttr = {0};

    ViDev = pViInfo->stDevInfo.ViDev;
    enSnsType = pViInfo->stSnsInfo.enSnsType;

    Media_Vi_GetDevAttr(enSnsType, &stViDevAttr);
    stViDevAttr.stWDRAttr.enWDRMode = pViInfo->stDevInfo.enWDRMode;
    if (VI_PARALLEL_VPSS_OFFLINE == pViInfo->stPipeInfo.enMastPipeMode || VI_PARALLEL_VPSS_PARALLEL == pViInfo->stPipeInfo.enMastPipeMode)
    {
        stViDevAttr.enDataRate = DATA_RATE_X2;
    }

    s32Ret = HI_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_SetDevAttr error! s32Ret = %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_EnableDev error! s32Ret = %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int Media_Vi_BindPipe(MEDIA_VI_INFO_S *pViInfo)
{
    HI_S32              i = 0;
    HI_S32              s32PipeCnt = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_DEV_BIND_PIPE_S  stDevBindPipe = {0};

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            stDevBindPipe.PipeId[s32PipeCnt] = pViInfo->stPipeInfo.aPipe[i];
            s32PipeCnt++;
            stDevBindPipe.u32Num = s32PipeCnt;
        }
    }

    s32Ret = HI_MPI_VI_SetDevBindPipe(pViInfo->stDevInfo.ViDev, &stDevBindPipe);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_SetDevBindPipe failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

int Media_Vi_StartPipe(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32          i = 0, j = 0;
    HI_S32          s32Ret = HI_SUCCESS;
    VI_PIPE         ViPipe = 0;
    VI_PIPE_ATTR_S  stPipeAttr = {0};

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            Media_VideoIn_GetPipeAttr(pViInfo->stSnsInfo.enSnsType, &stPipeAttr);
            if (HI_TRUE == pViInfo->stPipeInfo.bIspBypass)
            {
                stPipeAttr.bIspBypass = HI_TRUE;
                stPipeAttr.enPixFmt   = pViInfo->stPipeInfo.enPixFmt;
                stPipeAttr.enBitWidth = DATA_BITWIDTH_8;
            }

            if ((2 == ViPipe) || (3 == ViPipe))
            {
                stPipeAttr.enCompressMode = COMPRESS_MODE_NONE;
            }

            if ((pViInfo->stSnapInfo.bSnap) && (pViInfo->stSnapInfo.bDoublePipe) && (ViPipe == pViInfo->stSnapInfo.SnapPipe))
            {
                s32Ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != HI_SUCCESS)
                {
                    prtMD("HI_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    goto EXIT;
                }
            }
            else
            {
                s32Ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != HI_SUCCESS)
                {
                    prtMD("HI_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    return HI_FAILURE;
                }

                if (HI_TRUE == pViInfo->stPipeInfo.bVcNumCfged)
                {
                    s32Ret = HI_MPI_VI_SetPipeVCNumber(ViPipe, pViInfo->stPipeInfo.u32VCNum[i]);
                    if (s32Ret != HI_SUCCESS)
                    {
                        HI_MPI_VI_DestroyPipe(ViPipe);
                        prtMD("HI_MPI_VI_SetPipeVCNumber failed with %#x!\n", s32Ret);
                        return HI_FAILURE;
                    }
                }

                s32Ret = HI_MPI_VI_StartPipe(ViPipe);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VI_DestroyPipe(ViPipe);
                    prtMD("HI_MPI_VI_StartPipe failed with %#x!\n", s32Ret);
                    return HI_FAILURE;
                }
            }

        }
    }

    return s32Ret;

EXIT:

    for (j = 0; j < i; j++)
    {
        ViPipe = j;
        Media_VideoIn_StopSingleViPipe(ViPipe);
    }

    return s32Ret;
}

int Media_VideoIn_StopSingleViPipe(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_VI_StopPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_StopPipe failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_DestroyPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_DestroyPipe failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

int Media_VideoIn_StopDev(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32 s32Ret;
    VI_DEV ViDev;

    ViDev   = pViInfo->stDevInfo.ViDev;
    s32Ret  = HI_MPI_VI_DisableDev(ViDev);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_DisableDev failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int Media_VideoIn_GetPipeAttr(VI_SNS_TYPE_E enSnsType, VI_PIPE_ATTR_S *pPipeAttr)
{
    if (pPipeAttr == NULL)
    {
        prtMD("invalid input pPipeAttr = %p\n", pPipeAttr);
        return -1;
    }

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            memcpy_s(pPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
            break;

        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            memcpy_s(pPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
            pPipeAttr->enPixFmt = PIXEL_FORMAT_RGB_BAYER_10BPP;
            pPipeAttr->enBitWidth = DATA_BITWIDTH_10;
            break;

        default:
            memcpy_s(pPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
    }

    return HI_SUCCESS;
}

int Media_VideoIn_StopSinglePipe(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_VI_StopPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_StopPipe failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_DestroyPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_DestroyPipe failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

int Media_VideoIn_StartPipe(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32          i = 0;
    HI_S32          j = 0;
    HI_S32          s32Ret = HI_SUCCESS;
    VI_PIPE         ViPipe = 0;
    VI_PIPE_ATTR_S  stPipeAttr = {0};

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            Media_VideoIn_GetPipeAttr(pViInfo->stSnsInfo.enSnsType, &stPipeAttr);
            if (HI_TRUE == pViInfo->stPipeInfo.bIspBypass)
            {
                stPipeAttr.bIspBypass = HI_TRUE;
                stPipeAttr.enPixFmt   = pViInfo->stPipeInfo.enPixFmt;
                stPipeAttr.enBitWidth = DATA_BITWIDTH_8;
            }

            if((2 == ViPipe) || (3 == ViPipe))
            {
                stPipeAttr.enCompressMode = COMPRESS_MODE_NONE;
            }

            if ((pViInfo->stSnapInfo.bSnap) && (pViInfo->stSnapInfo.bDoublePipe) && (ViPipe == pViInfo->stSnapInfo.SnapPipe))
            {
                s32Ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != HI_SUCCESS)
                {
                    prtMD("HI_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    goto EXIT;
                }
            }
            else
            {
                s32Ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != HI_SUCCESS)
                {
                    prtMD("HI_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    return HI_FAILURE;
                }

                if (HI_TRUE == pViInfo->stPipeInfo.bVcNumCfged)
                {
                    s32Ret = HI_MPI_VI_SetPipeVCNumber(ViPipe, pViInfo->stPipeInfo.u32VCNum[i]);
                    if (s32Ret != HI_SUCCESS)
                    {
                        HI_MPI_VI_DestroyPipe(ViPipe);
                        prtMD("HI_MPI_VI_SetPipeVCNumber failed with %#x!\n", s32Ret);
                        return HI_FAILURE;
                    }
                }

                s32Ret = HI_MPI_VI_StartPipe(ViPipe);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VI_DestroyPipe(ViPipe);
                    prtMD("HI_MPI_VI_StartPipe failed with %#x!\n", s32Ret);
                    return HI_FAILURE;
                }
            }

        }
    }

    return s32Ret;

EXIT:

    for (j = 0; j < i; j++)
    {
        ViPipe = j;
        Media_VideoIn_StopSinglePipe(ViPipe);
    }

    return s32Ret;
}

int Media_VideoIn_GetChnAttr(VI_SNS_TYPE_E enSnsType, VI_CHN_ATTR_S* pstChnAttr)
{
    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
            break;

        default:
            memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
    }

    return HI_SUCCESS;
}

int Media_VideoIn_StartChn(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32              i = 0;
    HI_BOOL             bNeedChn = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_PIPE             ViPipe = 0;
    VI_CHN              ViChn = 0;
    VI_CHN_ATTR_S       stChnAttr = {0};
    VI_VPSS_MODE_E      enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            ViChn  = pViInfo->stChnInfo.ViChn;

            Media_VideoIn_GetChnAttr(pViInfo->stSnsInfo.enSnsType, &stChnAttr);
            stChnAttr.enDynamicRange = pViInfo->stChnInfo.enDynamicRange;
            stChnAttr.enVideoFormat  = pViInfo->stChnInfo.enVideoFormat;
            stChnAttr.enPixelFormat  = pViInfo->stChnInfo.enPixFormat;
            stChnAttr.enCompressMode = pViInfo->stChnInfo.enCompressMode;

            if (WDR_MODE_NONE == pViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = HI_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (bNeedChn)
            {
                s32Ret = HI_MPI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);
                if (s32Ret != HI_SUCCESS)
                {
                    prtMD("HI_MPI_VI_SetChnAttr failed with %#x!\n", s32Ret);
                    return HI_FAILURE;
                }

                enMastPipeMode = pViInfo->stPipeInfo.enMastPipeMode;

                if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode)
                {
                    s32Ret = HI_MPI_VI_EnableChn(ViPipe, ViChn);
                    if (s32Ret != HI_SUCCESS)
                    {
                        prtMD("HI_MPI_VI_EnableChn failed with %#x!\n", s32Ret);
                        return HI_FAILURE;
                    }
                }
            }
        }
    }

    return s32Ret;
}

int Media_VideoIn_CreateVi(MEDIA_VI_INFO_S *pViInfo)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    s32Ret = Media_Vi_StartDev(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("Media_Vi_StartDev failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    //we should bind pipe,then creat pipe
    s32Ret = Media_Vi_BindPipe(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("SAMPLE_COMM_VI_BindPipeDev failed with %#x!\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = Media_VideoIn_StartPipe(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("Media_VideoIn_StartPipe failed with %#x!\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = Media_VideoIn_StartChn(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("Media_VideoIn_StartChn failed with %#x!\n", s32Ret);
        goto EXIT2;
    }

    return HI_SUCCESS;

EXIT2:
    s32Ret = Media_VideoIn_StopPipe(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("Media_VideoIn_StopPipe failed with %#x!\n", s32Ret);
    }

EXIT1:
    s32Ret = Media_VideoIn_StopDev(pViInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("Media_VideoIn_StopDev failed with %#x!\n", s32Ret);
    }

    return s32Ret;
}

int Media_VideoIn_StopChn(MEDIA_VI_INFO_S *pViInfo)
{
    HI_S32              i = 0;
    HI_BOOL             bNeedChn = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_PIPE             ViPipe = 0;
    VI_CHN              ViChn = 0;
    VI_VPSS_MODE_E      enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;

    for (i = 0; i < 4; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            ViChn  = pViInfo->stChnInfo.ViChn;

            if (WDR_MODE_NONE == pViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = HI_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (bNeedChn)
            {
                enMastPipeMode = pViInfo->stPipeInfo.enMastPipeMode;

                if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode)
                {
                    s32Ret = HI_MPI_VI_DisableChn(ViPipe, ViChn);

                    if (s32Ret != HI_SUCCESS)
                    {
                        prtMD("HI_MPI_VI_DisableChn failed with %#x!\n", s32Ret);
                        return HI_FAILURE;
                    }
                }
            }
        }
    }

    return s32Ret;
}

int Media_VideoIn_StopPipe(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32  i = 0;
    HI_S32  s32Ret = HI_SUCCESS;
    VI_PIPE ViPipe = 0;

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            s32Ret = Media_VideoIn_StopSingleViPipe(ViPipe);
            if (HI_SUCCESS != s32Ret)
            {
                prtMD("Media_VideoIn_StopSingleViPipe error! s32Ret = %#x\n", s32Ret);
            }
        }
    }

    return HI_SUCCESS;
}

int Media_VideoIn_DestroySingleVi(MEDIA_VI_INFO_S *pViInfo)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    s32Ret = Media_VideoIn_StopChn(pViInfo);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StopChn error! s32Ret = %#x\n", s32Ret);
    }

    s32Ret = Media_VideoIn_StopPipe(pViInfo);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StopChn error! s32Ret = %#x\n", s32Ret);
    }

    s32Ret = Media_VideoIn_StopDev(pViInfo);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StopChn error! s32Ret = %#x\n", s32Ret);
    }

    return HI_SUCCESS;
}

int Media_VideoIn_StartVi(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32              i = 0, j = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;

    if (pViParam == NULL)
    {
        prtMD("invalid input pViParam = %p\n", pViParam);
        return -1;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        s32Ret = Media_VideoIn_CreateVi(pViInfo);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("Media_VideoIn_CreateVi error! s32Ret = %#x\n", s32Ret);
            goto EXIT;
        }
    }

    return HI_SUCCESS;

EXIT:
    for (j = 0; j < i; j++)
    {
        s32ViNum  = pViParam->s32WorkingViId[j];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        s32Ret = Media_VideoIn_DestroySingleVi(pViInfo);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("Media_VideoIn_DestroySingleVi error! s32Ret = %#x!\n", s32Ret);
        }

    }

    return s32Ret;
}

int Media_VideoIn_Rotate(MEDIA_VI_INFO_S *pViInfo, ROTATION_E enRotation)
{
    int i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    VI_PIPE ViPipe = -1;
    VI_CHN ViChn = 0;
    HI_BOOL bNeedChn = HI_FALSE;

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        bNeedChn = HI_FALSE;
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pViInfo->stPipeInfo.aPipe[i];
            if (WDR_MODE_NONE == pViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = HI_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (bNeedChn)
            {
                s32Ret = HI_MPI_VI_SetChnRotation(ViPipe, ViChn, enRotation);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("HI_MPI_VI_SetChnCrop error! s32Ret = %#x\n", s32Ret);
                }
            }
        }
    }

    return s32Ret;
}

int Media_VideoIn_SetRotate(MEDIA_VI_PARAM_S *pViParam, ROTATION_E enRotation)
{
    HI_S32              i = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;

    if (pViParam == NULL)
    {
        prtMD("invalid input pViParam = %p\n", pViParam);
        return -1;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        s32Ret = Media_VideoIn_Rotate(pViInfo, enRotation);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("Media_VideoIn_CreateVi error! s32Ret = %#x\n", s32Ret);
        }
    }

    return s32Ret;
}

int Media_VideoIn_Init(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ROTATION_E enRotation = ROTATION_270;

    if (pViParam == NULL)
    {
        prtMD("invalid input pViParam = %p\n", pViParam);
        return -1;
    }

    /* 设置MIPI_RX */
    s32Ret = Media_VideoIn_StartMipiRx(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StartMipiRx error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 设置PIPE工作模式 */
    s32Ret = Media_VideoIn_SetPipeMode();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_SetPipeMode error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 设置VI */
    s32Ret = Media_VideoIn_StartVi(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StartVi error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* set the retation of vi chan */
    s32Ret = Media_VideoIn_SetRotate(pViParam, enRotation);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_StartVi error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return HI_SUCCESS;
}
