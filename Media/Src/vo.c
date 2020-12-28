#include "vo.h"
#include "mipi_tx.h"

MEDIA_SIZE_S enDispSize[LCD_IDX_BUTT] =
{
    {
        1024,
        600
    },

    {
        640,
        480
    },
};

int Media_Vo_InitDev(VO_DEV VoDev, MEDIA_LCD_IDX_E lcdTypeIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VO_PUB_ATTR_S stPubAttr = {0};
    unsigned int dispWidth = 600;
    unsigned int dispHeight = 1024;
    HI_U32 u32Framerate = 60;
    VO_USER_INTFSYNC_INFO_S stUserInfo = {0};

    stPubAttr.enIntfType = VO_INTF_MIPI;
    stPubAttr.enIntfSync = VO_OUTPUT_USER;
    stPubAttr.u32BgColor = COLOR_RGB_BLACK;

    if (lcdTypeIdx >= LCD_IDX_BUTT)
    {
        prtMD("invalid input lcdType = %d\n", lcdTypeIdx);
        return 0;
    }

    dispWidth = enDispSize[lcdTypeIdx].w;
    dispHeight = enDispSize[lcdTypeIdx].h;

    if (lcdTypeIdx == LCD_IDX_7_600X1024_RP)
    {
        stPubAttr.stSyncInfo.bSynm = 0;
        stPubAttr.stSyncInfo.bIop = 1;
        stPubAttr.stSyncInfo.u16Vact = dispHeight;
        stPubAttr.stSyncInfo.u16Vbb = 18;
        stPubAttr.stSyncInfo.u16Vfb = 16;
        stPubAttr.stSyncInfo.u16Hact = dispWidth;
        stPubAttr.stSyncInfo.u16Hbb = 64;
        stPubAttr.stSyncInfo.u16Hfb = 136;
        stPubAttr.stSyncInfo.u16Hpw = 4;
        stPubAttr.stSyncInfo.u16Vpw = 2;
        stPubAttr.stSyncInfo.bIdv = 0;
        stPubAttr.stSyncInfo.bIhs = 0;
        stPubAttr.stSyncInfo.bIvs = 0;
    }
    else if (lcdTypeIdx == LCD_IDX_3_480X640)
    {
        stPubAttr.stSyncInfo.bSynm = 0;
        stPubAttr.stSyncInfo.bIop = 1;

        stPubAttr.stSyncInfo.u16Hmid = 1;
        stPubAttr.stSyncInfo.u16Bvact = 1;
        stPubAttr.stSyncInfo.u16Bvbb = 1;
        stPubAttr.stSyncInfo.u16Bvfb = 1;

        stPubAttr.stSyncInfo.u16Vact = dispHeight;
        stPubAttr.stSyncInfo.u16Vbb = 18;
        stPubAttr.stSyncInfo.u16Vfb = 17;
        stPubAttr.stSyncInfo.u16Hact = dispWidth;
        stPubAttr.stSyncInfo.u16Hbb = 240;
        stPubAttr.stSyncInfo.u16Hfb = 120;
        stPubAttr.stSyncInfo.u16Hpw = 120;
        stPubAttr.stSyncInfo.u16Vpw = 5;

        stPubAttr.stSyncInfo.bIdv = 0;
        stPubAttr.stSyncInfo.bIhs = 0;
        stPubAttr.stSyncInfo.bIvs = 0;

        stPubAttr.u32BgColor = COLOR_RGB_RED;
    }
    else
    {
        prtMD("unsupported lcdTypeIdx! please check lcdTypeIdx = %d!\n", lcdTypeIdx);
        return HI_FAILURE;
    }

    prtMD("dispWidth = %d, dispHeight = %d\n", dispWidth, dispHeight);

    s32Ret = HI_MPI_VO_SetPubAttr(VoDev, &stPubAttr);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    if (lcdTypeIdx == LCD_IDX_7_600X1024_RP)
    {
        stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = 380;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = 0x3F7271;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = 4;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = 7;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = 7;
        stUserInfo.u32DevDiv = 1;
        stUserInfo.u32PreDiv = 1;
    }
    else if (lcdTypeIdx == LCD_IDX_3_480X640)
    {
        stUserInfo.bClkReverse = HI_TRUE;//时钟相位反向
        stUserInfo.u32DevDiv = 1;
        stUserInfo.u32PreDiv = 1;
        stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = 252;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = 0x599999;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = 4;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = 7;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = 7;
    }
    else
    {
        prtMD("unsupported lcdTypeIdx! please check lcdTypeIdx = %d!\n", lcdTypeIdx);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("set user interface sync info failed with %x.\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VO_SetDevFrameRate(VoDev, u32Framerate);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VO_SetDevFrameRate failed with %x.\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VO_Enable(VoDev);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

int Media_Vo_InitVoLayer(VO_LAYER VoLayer, MEDIA_LCD_IDX_E enLcdIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;
    unsigned int dispBufLen = 3;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr = {0};
    VO_CSC_S stVideoCSC = {0};
    unsigned int dispWidth = 0;
    unsigned int dispHeight = 0;

    if (enLcdIdx >= LCD_IDX_BUTT)
    {
        prtMD("invalid input enLcdIdx = %d\n", enLcdIdx);
        return -1;
    }

    s32Ret = HI_MPI_VO_SetDisplayBufLen(VoLayer, dispBufLen);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_SetDisplayBufLen failed with %#x!\n", s32Ret);
        return s32Ret;
    }

#if 0
    VO_PART_MODE_E enPartMode = VO_PART_MODE_SINGLE;

    s32Ret = HI_MPI_VO_SetVideoLayerPartitionMode(VoLayer, enPartMode);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_SetVideoLayerPartitionMode failed with %#x!\n", s32Ret);
        return s32Ret;
    }
#endif

    dispWidth = enDispSize[enLcdIdx].w;
    dispHeight = enDispSize[enLcdIdx].h;

    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stLayerAttr.u32DispFrmRt = 30;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = stLayerAttr.stImageSize.u32Width = dispWidth;
    stLayerAttr.stDispRect.u32Height = stLayerAttr.stImageSize.u32Height = dispHeight;

    stLayerAttr.enDstDynamicRange = DYNAMIC_RANGE_SDR8;

    prtMD("dispWidth = %d, dispHeight = %d\n", dispWidth, dispHeight);

    s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VO_SetVideoLayerAttr failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VO_SetVideoLayerAttr failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VO_GetVideoLayerCSC(VoLayer, &stVideoCSC);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_GetVideoLayerCSC failed with %#x\n", s32Ret);
        return s32Ret;
    }
    stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
    s32Ret = HI_MPI_VO_SetVideoLayerCSC(VoLayer, &stVideoCSC);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_SetVideoLayerCSC failed width %#x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int Media_Vo_InitMipiTx(MEDIA_LCD_IDX_E lcdTypeIdx)
{
    return Media_MipiTx_Init(lcdTypeIdx);
}

int Media_Vo_Init(MEDIA_LCD_IDX_E lcdTypeIdx)
{
    VO_DEV VoDev = 0;
    VO_LAYER VoLayer = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    static unsigned int bInitFlag = 0;

    if (lcdTypeIdx >= LCD_IDX_BUTT)
    {
        prtMD("invalid lcdTypeIdx = %d\n", lcdTypeIdx);
        return 0;
    }

    prtMD("lcdTypeIdx = %d\n", lcdTypeIdx);

    if (bInitFlag == 1)
    {
        prtMD("VO has been inited!\n");
        return 0;
    }

    s32Ret = Media_Vo_InitDev(VoDev, lcdTypeIdx);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_Vo_InitDev error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    s32Ret = Media_Vo_InitVoLayer(VoLayer, lcdTypeIdx);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_Vo_InitVoLayer error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    if (lcdTypeIdx != LCD_IDX_3_480X640)
    {
        s32Ret = Media_Vo_InitMipiTx(lcdTypeIdx);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_Vo_InitMipiTx error! s32Ret = %#x\n", s32Ret);
            return s32Ret;
        }
    }

    bInitFlag = 1;

    return 0;
}

int Media_Vo_StartChn(VO_LAYER VoLayer, VO_CHN voChn, MEDIA_VIDEO_DISP_S *pDispParam)
{
    VO_VIDEO_LAYER_ATTR_S stLayerAttr = {0};
    VO_CHN_ATTR_S stChnAttr = {0};
    HI_S32 s32Ret = HI_SUCCESS;
    ROTATION_E enRotation = ROTATION_0;

    if (pDispParam == NULL)
    {
        prtMD("invalid input pDispParam = %p\n", pDispParam);
        return -1;
    }

    s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_GetVideoLayerAttr error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    stChnAttr.stRect.s32X = pDispParam->stRect.x;
    stChnAttr.stRect.s32Y = pDispParam->stRect.y;
    stChnAttr.stRect.u32Width = pDispParam->stRect.w;
    stChnAttr.stRect.u32Height = pDispParam->stRect.h;
    stChnAttr.u32Priority = 0;
    stChnAttr.bDeflicker = HI_FALSE;

    s32Ret = HI_MPI_VO_SetChnAttr(VoLayer, voChn, &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_SetChnAttr error! s32Ret = %#X\n", s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VO_EnableChn(VoLayer, voChn);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_EnableChn error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    switch (pDispParam->enRotation)
    {
        case MEDIA_ROTATION_0:
            enRotation = ROTATION_0;
            break;
        case MEDIA_ROTATION_90:
            enRotation = ROTATION_90;
            break;
        case MEDIA_ROTATION_180:
            enRotation = ROTATION_180;
            break;
        case MEDIA_ROTATION_270:
            enRotation = ROTATION_270;
            break;
        default:
            enRotation = ROTATION_0;
            break;
    }

    s32Ret = HI_MPI_VO_SetChnRotation(VoLayer, voChn, enRotation);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VO_EnableChn error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return HI_SUCCESS;
}
