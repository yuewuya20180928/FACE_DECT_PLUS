#include<pthread.h>

#include "isp.h"
#include "vi.h"
#include "isp_configure.h"

pthread_t g_IspPid[MAX_ISP_NUM];

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX327_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX327_720P_30FPS =
{
    {0, 0, 1280, 720},
    {1280, 720},
    30,
    BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX327_2M_30FPS_WDR2TO1 =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    BAYER_RGGB,
    WDR_MODE_2To1_LINE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2145_UXGA_30FPS =
{
    {0, 0, 1600, 1200},
    {1600, 1200},
    30,
    BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2145_960P_30FPS =
{
    {0, 0, 1280, 960},
    {1280, 960},
    30,
    BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

int Media_Isp_GetAttr(VI_SNS_TYPE_E enSnsType, ISP_PUB_ATTR_S *pPubAttr)
{
    if (pPubAttr == NULL)
    {
        prtMD("invalid input pPubAttr = %p\n", pPubAttr);
        return -1;
    }

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
            memcpy(pPubAttr, &ISP_PUB_ATTR_IMX327_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
            break;

        case SONY_IMX327_2L_MIPI_720P_30FPS_12BIT:
            memcpy(pPubAttr, &ISP_PUB_ATTR_IMX327_720P_30FPS, sizeof(ISP_PUB_ATTR_S));
            break;

        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            memcpy(pPubAttr, &ISP_PUB_ATTR_IMX327_2M_30FPS_WDR2TO1, sizeof(ISP_PUB_ATTR_S));
            break;

        case GALAXYCORE_GC2145_MIPI_UXGA_30FPS_10BIT:
            memcpy(pPubAttr, &ISP_PUB_ATTR_GC2145_UXGA_30FPS, sizeof(ISP_PUB_ATTR_S));
            break;

        case GALAXYCORE_GC2145_MIPI_960P_30FPS_10BIT:
            memcpy(pPubAttr, &ISP_PUB_ATTR_GC2145_960P_30FPS, sizeof(ISP_PUB_ATTR_S));
            break;

        default:
            memcpy(pPubAttr, &ISP_PUB_ATTR_IMX327_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
            break;
    }

    return HI_SUCCESS;
}

ISP_SNS_OBJ_S* Media_Isp_GetSnsObj(VI_SNS_TYPE_E enSnsType)
{
    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            return &stSnsImx327Obj;

        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_720P_30FPS_12BIT:
        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            return &stSnsImx327_2l_Obj;

        case GALAXYCORE_GC2145_MIPI_UXGA_30FPS_10BIT:
        case GALAXYCORE_GC2145_MIPI_960P_30FPS_10BIT:
            return &stSnsGc2145Obj;

        default:
            prtMD("unsupported! to be finished! enSnsType = %d\n", enSnsType);
            return HI_NULL;
    }
}

int Media_Isp_RegiterSensorCallback(ISP_DEV IspDev, VI_SNS_TYPE_E enSnsType)
{
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    const ISP_SNS_OBJ_S* pstSnsObj;
    HI_S32 s32Ret = HI_SUCCESS;

    pstSnsObj = Media_Isp_GetSnsObj(enSnsType);
    if (HI_NULL == pstSnsObj)
    {
        prtMD("sensor %d unsupported!\n", enSnsType);
        return HI_FAILURE;
    }

    stAeLib.s32Id = IspDev;
    stAwbLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));

    if (pstSnsObj->pfnRegisterCallback != HI_NULL)
    {
        prtMD("pfnRegisterCallback IspDev = %d\n", IspDev);
        s32Ret = pstSnsObj->pfnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("pfnRegisterCallback failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        prtMD("pfnRegisterCallback failed with HI_NULL!\n");
    }

    return HI_SUCCESS;
}

int Media_Isp_BindSensor(ISP_DEV IspDev, VI_SNS_TYPE_E enSnsType, HI_S8 s8SnsDev)
{
    ISP_SNS_COMMBUS_U uSnsBusInfo;
    const ISP_SNS_OBJ_S *pstSnsObj;
    HI_S32 s32Ret = HI_SUCCESS;

    pstSnsObj = Media_Isp_GetSnsObj(enSnsType);
    if (HI_NULL == pstSnsObj)
    {
        prtMD("sensor %d not exist!\n", enSnsType);
        return HI_FAILURE;
    }

    uSnsBusInfo.s8I2cDev = s8SnsDev;

    if (HI_NULL != pstSnsObj->pfnSetBusInfo)
    {
        prtMD("pfnSetBusInfo IspDev = %d\n", IspDev);
        s32Ret = pstSnsObj->pfnSetBusInfo(IspDev, uSnsBusInfo);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("set sensor bus info failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        prtMD("not support set sensor bus info!\n");
        return HI_FAILURE;
    }

    return s32Ret;
}

int Media_Isp_Aelib_Callback(ISP_DEV IspDev)
{
    ALG_LIB_S stAeLib;
    HI_S32 s32Ret = HI_SUCCESS;

    stAeLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));

    prtMD("HI_MPI_AE_Register IspDev = %d\n", IspDev);
    s32Ret = HI_MPI_AE_Register(IspDev, &stAeLib);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_AE_Register error! s32Ret = %#x\n", s32Ret);
    }

    return s32Ret;
}

int Media_Isp_Awblib_Callback(ISP_DEV IspDev)
{
    ALG_LIB_S stAwbLib;
    HI_S32 s32Ret = HI_SUCCESS;

    stAwbLib.s32Id = IspDev;
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));

    prtMD("HI_MPI_AWB_Register IspDev = %d\n", IspDev);
    s32Ret = HI_MPI_AWB_Register(IspDev, &stAwbLib);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_AWB_Register error! s32Ret = %#x\n", s32Ret);
    }

    return s32Ret;
}

/* 根据ViPipe的值去 */
int Media_Isp_LoadParam(VI_PIPE viPipe)
{
    char fileNames[512] = {0};
    int s32Ret = 0;
    ISPCFG_PARAM_S stIspParam = {0};

    if (viPipe > VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input viPipe = %d\n", viPipe);
        return -1;
    }

    snprintf(fileNames, 512, "%s%d", ISP_CFG_FILE_PREFIX, viPipe);
    prtMD("fileNames[%d]: %s\n", viPipe, fileNames);

    /* 加载配置文件 */
    s32Ret = ISPCFG_LoadFile(viPipe, fileNames);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_LoadFile error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 从配置文件中获取ISP参数 */
    s32Ret = ISPCFG_GetParam(viPipe, &stIspParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetParam error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 将配置文件中的参数设置到海思驱动中去 */
    s32Ret = ISPCFG_SetParam(viPipe, &stIspParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetParam error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 卸载配置文件 */
    s32Ret = ISPCFG_UnloadFile(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_UnloadFile error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return 0;
}

void* Media_Isp_Thread(void* param)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;;
    HI_CHAR szThreadName[20];

    IspDev = (ISP_DEV)param;

    snprintf(szThreadName, 20, "ISP%d_RUN", IspDev);
    prctl(PR_SET_NAME, szThreadName, 0,0,0);

    prtMD("ISP Dev %d running !\n", IspDev);
    s32Ret = HI_MPI_ISP_Run(IspDev);                    /* 这个函数内部有while(1),要在线程中调用,否者会卡死 */
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_ISP_Run failed with %#x!\n", s32Ret);
    }

    return NULL;
}

int Media_Isp_Run(ISP_DEV IspDev)
{
    HI_S32 s32Ret = 0;

    s32Ret = pthread_create(&g_IspPid[IspDev], NULL, Media_Isp_Thread, (HI_VOID*)IspDev);
    if (0 != s32Ret)
    {
        prtMD("create Media_Isp_Thread failed!, error: %d, %s\r\n", s32Ret, strerror(s32Ret));
    }

    return s32Ret;
}

int Media_VideoIn_StartIsp(MEDIA_VI_INFO_S* pViInfo)
{
    HI_S32 i = 0;
    HI_BOOL bNeedPipe = 0;
    HI_S32  s32Ret = HI_SUCCESS;
    VI_PIPE ViPipe = -1;
    HI_U32 u32SnsId = 0;
    ISP_PUB_ATTR_S stPubAttr = {0};
    VI_PIPE_ATTR_S stPipeAttr = {0};

    if (pViInfo == NULL)
    {
        prtMD("invalid input pViInfo = %p\n", pViInfo);
        return -1;
    }

    s32Ret = Media_VideoIn_GetPipeAttr(pViInfo->stSnsInfo.enSnsType, &stPipeAttr);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_VideoIn_GetPipeAttr error! s32Ret = %#x\n", s32Ret);
    }

    if (VI_PIPE_BYPASS_BE == stPipeAttr.enPipeBypassMode)
    {
        return HI_SUCCESS;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pViInfo->stPipeInfo.aPipe[i] >= 0  && pViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe      = pViInfo->stPipeInfo.aPipe[i];
            u32SnsId    = pViInfo->stSnsInfo.s32SnsId;

            s32Ret = Media_Isp_GetAttr(pViInfo->stSnsInfo.enSnsType, &stPubAttr);
            if (HI_SUCCESS != s32Ret)
            {
                prtMD("Media_Isp_GetAttr error! s32Ret = %#x\n", s32Ret);
            }

            stPubAttr.enWDRMode = pViInfo->stDevInfo.enWDRMode;

            if (WDR_MODE_NONE == pViInfo->stDevInfo.enWDRMode)
            {
                bNeedPipe = HI_TRUE;
            }
            else
            {
                bNeedPipe = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (HI_TRUE != bNeedPipe)
            {
                continue;
            }

            s32Ret = Media_Isp_RegiterSensorCallback(ViPipe, pViInfo->stSnsInfo.enSnsType);
            if (HI_SUCCESS != s32Ret)
            {
                prtMD("register sensor %d to ISP %d failed\n", u32SnsId, ViPipe);
                return HI_FAILURE;
            }

            if (((pViInfo->stSnapInfo.bDoublePipe) && (pViInfo->stSnapInfo.SnapPipe == ViPipe)) || (pViInfo->stPipeInfo.bMultiPipe && i > 0))
            {
                s32Ret = Media_Isp_BindSensor(ViPipe, pViInfo->stSnsInfo.enSnsType, -1);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("register sensor %d bus id %d failed\n", u32SnsId, pViInfo->stSnsInfo.s32BusId);
                    return HI_FAILURE;
                }
            }
            else
            {
                s32Ret = Media_Isp_BindSensor(ViPipe, pViInfo->stSnsInfo.enSnsType, pViInfo->stSnsInfo.s32BusId);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("register sensor %d bus id %d failed\n", u32SnsId, pViInfo->stSnsInfo.s32BusId);
                    return HI_FAILURE;
                }
            }

            s32Ret = Media_Isp_Aelib_Callback(ViPipe);
            if (HI_SUCCESS != s32Ret)
            {
                prtMD("Media_Isp_Aelib_Callback error! s32Ret = %#x\n", s32Ret);
                return HI_FAILURE;
            }

            s32Ret = Media_Isp_Awblib_Callback(ViPipe);
            if (HI_SUCCESS != s32Ret)
            {
                prtMD("Media_Isp_Awblib_Callback failed\n");
                return HI_FAILURE;
            }

            prtMD("HI_MPI_ISP_MemInit IspDev = %d\n", ViPipe);
            s32Ret = HI_MPI_ISP_MemInit(ViPipe);
            if (s32Ret != HI_SUCCESS)
            {
                prtMD("Init Ext memory failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }

            prtMD("HI_MPI_ISP_SetPubAttr IspDev = %d\n", ViPipe);
            s32Ret = HI_MPI_ISP_SetPubAttr(ViPipe, &stPubAttr);
            if (s32Ret != HI_SUCCESS)
            {
                prtMD("SetPubAttr failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }

            prtMD("HI_MPI_ISP_Init IspDev = %d\n", ViPipe);
            s32Ret = HI_MPI_ISP_Init(ViPipe);
            if (s32Ret != HI_SUCCESS)
            {
                prtMD("ISP Init failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }

            prtMD("Media_Isp_Run IspDev = %d\n", ViPipe);
            s32Ret = Media_Isp_Run(ViPipe);
            if (s32Ret != HI_SUCCESS)
            {
                prtMD("ISP Run failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }
        }
    }

    return s32Ret;
}

void *Media_Isp_Tsk(void *arg)
{
    MEDIA_VI_PARAM_S *pViParam = NULL;
    unsigned int i = 0;
    unsigned int j = 0;
    int viIdx = 0;
    VI_PIPE ViPipe = -1;
    MEDIA_VI_INFO_S *pViInfo = NULL;
    ISP_PUB_ATTR_S stPubAttr = {0};
    HI_S32 s32Ret = HI_SUCCESS;
    HI_BOOL bNeedPipe = HI_FALSE;

    pViParam = (MEDIA_VI_PARAM_S *)arg;

    /* 每个VI通道加载默认的ISP配置参数 */
    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        viIdx  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[viIdx];
        for (j = 0; j < WDR_MAX_PIPE_NUM; j++)
        {
            if (pViInfo->stPipeInfo.aPipe[j] >= 0  && pViInfo->stPipeInfo.aPipe[j] < VI_MAX_PIPE_NUM)
            {
                ViPipe = pViInfo->stPipeInfo.aPipe[j];

                s32Ret = Media_Isp_GetAttr(pViInfo->stSnsInfo.enSnsType, &stPubAttr);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("Media_Isp_GetAttr error! s32Ret = %#x\n", s32Ret);
                }

                stPubAttr.enWDRMode = pViInfo->stDevInfo.enWDRMode;

                if (WDR_MODE_NONE == pViInfo->stDevInfo.enWDRMode)
                {
                    bNeedPipe = HI_TRUE;
                }
                else
                {
                    bNeedPipe = (j > 0) ? HI_FALSE : HI_TRUE;
                }

                if (HI_TRUE != bNeedPipe)
                {
                    continue;
                }

                /* 从ISP配置文件中加载预先设计好的ISP参数 */
                s32Ret = Media_Isp_LoadParam(ViPipe);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("Media_Isp_LoadParam error! s32Ret = %#x\n", s32Ret);
                }
            }
        }
    }

    while (1)
    {
        usleep(50 * 1000);
        continue;
    }

    return NULL;
}

int Media_Isp_Init(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32 i = 0;
    HI_S32 s32ViNum = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    MEDIA_VI_INFO_S *pViInfo = HI_NULL;
    pthread_t ptd;

    if (!pViParam)
    {
        prtMD("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        s32Ret = Media_VideoIn_StartIsp(pViInfo);
        if (s32Ret != HI_SUCCESS)
        {
            prtMD("Media_VideoIn_StartIsp failed !\n");
            return HI_FAILURE;
        }
    }

    /* 创建ISP处理线程，实时的检测图像质量,并进行调节 */
    prtMD("Media_Isp_Init start!\n");
    pthread_create(&ptd, NULL, Media_Isp_Tsk, (void *)pViInfo);
    prtMD("Media_Isp_Init end!\n");

    return HI_SUCCESS;
}

