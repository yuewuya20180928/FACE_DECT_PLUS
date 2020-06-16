#include "media_api.h"
#include "vpss.h"
#include "module.h"

VPSS_PARAM_S stVpssParam[MAX_SENSOR_NUMBER];

int Media_Vpss_InitModule(void)
{
    memset(stVpssParam, 0, sizeof(VPSS_PARAM_S) * MAX_SENSOR_NUMBER);

    return 0;
}

int Media_Vpss_InitGroup(MEDIA_SENSOR_E enSensorIdx, unsigned int w, unsigned int h)
{
    VPSS_PARAM_S *pVpssParam = NULL;
    VPSS_GRP vpssGrp = -1;
    int ret = 0;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_GRP_HDL_S *pHandle = NULL;

    if (enSensorIdx >= MEDIA_SENSOR_BUTT)
    {
        prtMD("invalid input enSensorIdx = %d\n", enSensorIdx);
        return -1;
    }

    if (w > MAX_VB_WIDTH || h > MAX_VB_HEIGHT)
    {
        prtMD("invalid input w = %d, h = %d\n", w, h);
        //return -1;
    }

    memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));

    pVpssParam = stVpssParam + enSensorIdx;
    if (pHandle == NULL)
    {
        pHandle = (VPSS_GRP_HDL_S *)malloc(sizeof(VPSS_GRP_HDL_S));
        if (pHandle == NULL)
        {
            prtMD("malloc vpss group hdl error!\n");
            return -1;
        }
    }

    ret = Media_Module_GetChan(HI_VPSS, &vpssGrp);
    if (0 != ret)
    {
        prtMD("Media_Module_GetChan error!\n");
        return -1;
    }

    pHandle->groupIdx = vpssGrp;
    pHandle->groupWidth = w;

    pVpssParam->pHandle = (void *)pHandle;

    /* 设置并创建vpss组 */
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate           = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate           = -1;
    stVpssGrpAttr.enDynamicRange                        = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                         = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stVpssGrpAttr.u32MaxW                               = w;
    stVpssGrpAttr.u32MaxH                               = h;
    stVpssGrpAttr.bNrEn                                 = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode               = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode               = NR_MOTION_MODE_NORMAL;

    ret = HI_MPI_VPSS_CreateGrp(vpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != ret)
    {
        prtMD("HI_MPI_VPSS_CreateGrp error! ret = %#x\n", ret);
        return -1;
    }

    ret = HI_MPI_VPSS_StartGrp(vpssGrp);
    if (HI_SUCCESS != ret)
    {
        prtMD("HI_MPI_VPSS_CreateGrp error! ret = %#x\n", ret);
        return -1;
    }

    return ret;
}

int Media_Vpss_StartChn(MEDIA_SENSOR_E sensorIdx, VPSS_CHN VpssChn, MEDIA_RECT_S *pRect)
{
    VPSS_PARAM_S *pVpssParam = NULL;
    VPSS_GRP_HDL_S *pVpssGrpHdl = NULL;
    VPSS_GRP VpssGrp = -1;
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    unsigned int width = 0;
    unsigned int height = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    if (pRect == NULL || sensorIdx >= MEDIA_SENSOR_BUTT)
    {
        prtMD("invalid input pRect = %p, sensorIdx = %d\n", pRect, sensorIdx);
        return -1;
    }

    pVpssParam = stVpssParam + sensorIdx;

    pVpssGrpHdl = (VPSS_GRP_HDL_S *)pVpssParam->pHandle;

    VpssGrp = pVpssGrpHdl->groupIdx;

    width = ALIGN_DOWN(pRect->w, 2);
    height = ALIGN_DOWN(pRect->h, 2);

    stVpssChnAttr.u32Width                    = width;
    stVpssChnAttr.u32Height                   = height;
    stVpssChnAttr.enChnMode                   = VPSS_CHN_MODE_USER;
    stVpssChnAttr.enCompressMode              = COMPRESS_MODE_NONE;
    stVpssChnAttr.enDynamicRange              = DYNAMIC_RANGE_SDR8;
    stVpssChnAttr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
    stVpssChnAttr.enPixelFormat               = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stVpssChnAttr.stFrameRate.s32SrcFrameRate = 30;
    stVpssChnAttr.stFrameRate.s32DstFrameRate = 30;
    stVpssChnAttr.u32Depth                    = 0;
    stVpssChnAttr.bMirror                     = HI_FALSE;
    stVpssChnAttr.bFlip                       = HI_FALSE;
    stVpssChnAttr.stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);

    if (s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int Media_Vpss_BindVo(MEDIA_SENSOR_E sensorIdx, MEDIA_BIND_INFO_S *pDst)
{
    VPSS_PARAM_S *pVpssParam = NULL;
    VPSS_GRP_HDL_S *pVpssGrpHdl = NULL;
    VPSS_GRP VpssGrp = -1;
    MEDIA_BIND_INFO_S stSrc = {0};
    int s32Ret = 0;

    if (pDst == NULL)
    {
        prtMD("invalid input pDst = %p\n", pDst);
        return -1;
    }

    pVpssParam = stVpssParam + sensorIdx;

    pVpssGrpHdl = (VPSS_GRP_HDL_S *)pVpssParam->pHandle;

    VpssGrp = pVpssGrpHdl->groupIdx;

    stSrc.enModule = HI_VPSS;
    stSrc.devNo = VpssGrp;
    stSrc.chnNo = VPSS_CHN_FOR_VO;

    s32Ret = Media_Module_Bind(&stSrc, pDst);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_Module_Bind error! s32REet = %#x\n", s32Ret);        
    }

    return s32Ret;
}

int Media_Vpss_BindVi(MEDIA_SENSOR_E sensorIdx, MEDIA_BIND_INFO_S *pSrc)
{
    VPSS_PARAM_S *pVpssParam = NULL;
    VPSS_GRP_HDL_S *pVpssGrpHdl = NULL;
    VPSS_GRP VpssGrp = -1;
    MEDIA_BIND_INFO_S stDst = {0};
    int s32Ret = 0;

    if (pSrc == NULL)
    {
        prtMD("invalid input pSrc = %p\n", pSrc);
        return -1;
    }

    pVpssParam = stVpssParam + sensorIdx;

    pVpssGrpHdl = (VPSS_GRP_HDL_S *)pVpssParam->pHandle;

    VpssGrp = pVpssGrpHdl->groupIdx;

    stDst.enModule = HI_VPSS;
    stDst.devNo = VpssGrp;
    stDst.chnNo = VPSS_CHN_FOR_VO;

    s32Ret = Media_Module_Bind(pSrc, &stDst);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_Module_Bind error! s32REet = %#x\n", s32Ret);        
    }

    return s32Ret;
}
