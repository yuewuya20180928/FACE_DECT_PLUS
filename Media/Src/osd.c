#include "media_com.h"
#include "osd.h"
#include "SDL_ttf.h"
#include "SDL_surface.h"
#include "module.h"

static bool bShowTime = false;      /* 时间显示标志:1-显示时间;0-不显示时间 */
static TTF_Font *pOsdFont = NULL;
OSD_PARAM_S stOsdParam[OSD_TYPE_BUTT] = {0};

int Media_Osd_InitFT(char *pTtfString)
{
    int ret = 0;

    if (NULL == pTtfString)
    {
        prtMD("invalid input pTtfString = %p\n", pTtfString);
        return -1;
    }

    /* Initialize the TTF library */
    if (TTF_Init() < 0)
    {
        prtMD("TTF_Init error!\n");
        return -1;
    }

    /* load ttf */
    pOsdFont = TTF_OpenFont(pTtfString, 48);
    if (pOsdFont == NULL)
    {
        prtMD("TTF_OpenFont error!\n");
        ret = -1;
        goto exit1;
    }

    return ret;

exit1:
    TTF_Quit();

    return 0;
}

void *Media_Osd_Tsk(void *argc)
{
    int ret = 0;
    time_t lastTime = 0;
    time_t newTime = 0;
    struct tm *pTimeNow = NULL;
    char stTimeString[64] = {0};
    unsigned int u32rgb = 0xFF;
    OSD_PIC_INFO_S *pOsdPic = NULL;
    OSD_PARAM_S *pOsdParam = NULL;
    BITMAP_S stBitMap = {0};

    /* 1秒钟更新一下时间 */
    while(1)
    {
        pOsdParam = stOsdParam + OSD_TYPE_TIME;

        pOsdPic = &pOsdParam->stPicInfo;

        time(&newTime);
        if (newTime != lastTime)
        {
            lastTime = newTime;
            pTimeNow = localtime(&lastTime);

            if (pTimeNow != NULL)
            {
                snprintf(stTimeString, 64, "%04d-%02d-%02d %02d:%02d:%02d",
                    pTimeNow->tm_year + 1900,
                    pTimeNow->tm_mon + 1,
                    pTimeNow->tm_mday,
                    pTimeNow->tm_hour,
                    pTimeNow->tm_min,
                    pTimeNow->tm_sec);
            }

            /* 判断是否需要显示时间 */
            if (false == bShowTime)
            {
                usleep(100 * 1000);
                continue;
            }

            /* 使用SDL2_image开源库实现字符串转图片,图片通过region叠加到VO上 */
            ret = Media_Osd_StringToPic(stTimeString, u32rgb, pOsdPic);
            if (0 != ret)
            {
                prtMD("Media_Osd_StringToPic error! ret = %#x\n", ret);
                usleep(100 * 1000);
                continue;
            }

            /* 刷新region 画布数据 */
            stBitMap.pData = pOsdPic->pVirAddr;
            stBitMap.u32Width = pOsdPic->u32Width;
            stBitMap.u32Height = pOsdPic->u32Height;
            stBitMap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
            ret = HI_MPI_RGN_SetBitMap(pOsdParam->rgnHandle, &stBitMap);
            if (HI_SUCCESS != ret)
            {
                prtMD("HI_MPI_RGN_SetBitMap error! ret = %#x\n", ret);
                usleep(100 * 1000);
                continue;
            }
        }

        usleep(100 * 1000);
    }
}

/* 初始化region模块 */
int Media_Osd_InitRgn(OSD_TYPE_E enType)
{
    int ret = 0;
    unsigned int rgnHandle = 0;
    unsigned int regionWidth = MAX_OSD_WIDTH;
    unsigned int regionHeight = MAX_OSD_HEIGHT;
    unsigned int startX = 32;
    unsigned int startY = 32;
    MPP_CHN_S stChn = {0};
    RGN_CHN_ATTR_S stChnAttr = {0};
    RGN_ATTR_S stRgnAttr = {0};
    OSD_PARAM_S *pOsdParam = NULL;

    /* 创建region模块 */
    if (enType >= OSD_TYPE_BUTT)
    {
        prtMD("invalid input enType = %d\n", enType);
        return -1;
    }

    pOsdParam = stOsdParam + enType;

    /* 获取有效region handle */
    ret = Media_Module_GetChan(HI_RGN, &rgnHandle);
    if (0 != ret)
    {
        prtMD("Media_Module_GetChan error! ret = %#x\n", ret);
        return -1;
    }

    pOsdParam->enType = enType;
    pOsdParam->rgnHandle = rgnHandle;

    pOsdParam->stPicInfo.u32Width = regionWidth;
    pOsdParam->stPicInfo.u32Height = regionHeight;
    pOsdParam->stPicInfo.u32stride = regionWidth * 2;
    pOsdParam->stPicInfo.startX = startX;
    pOsdParam->stPicInfo.starty = startY;
    pOsdParam->stPicInfo.pVirAddr = (unsigned char *)malloc(regionWidth * regionHeight * 2);
    if (pOsdParam->stPicInfo.pVirAddr == NULL)
    {
        prtMD("malloc pOsdParam->stPicInfo.pVirAddr error!\n");
        ret = Media_Module_ReleaseChan(HI_RGN, rgnHandle);
        if (0 != ret)
        {
            prtMD("Media_Module_ReleaseChan error! ret = %#x\n", ret);
            return -1;
        }

        return -1;
    }

#if 0
    /* 创建region模块 */
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width = regionWidth;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = regionHeight;
    stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
    stRgnAttr.unAttr.stOverlay.u32CanvasNum = 1;
    ret = HI_MPI_RGN_Create(rgnHandle, &stRgnAttr);
    if (ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_RGN_Create error! s32Ret = %#x\n", ret);
        return ret;
    }

    /* 设置RGN绑定通道的信息 */
    stChn.enModId = HI_ID_VO;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;

    /* 设置RGN绑定通道的属性 */
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;

    /*起点位置需要16字节对齐*/
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = ALIGN_UP(startX, 16);
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = ALIGN_UP(startY, 16);
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    stChnAttr.unChnAttr.stOverlayChn.u32Layer = 2;  // 层次

    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp = 0;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;

    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 80;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;  // HI_TRUE：使能 OSD 反色
    stChnAttr.unChnAttr.stOverlayChn.enAttachDest = ATTACH_JPEG_MAIN;
#else
    /* 创建region模块 */
    stRgnAttr.enType = OVERLAYEX_RGN;
    stRgnAttr.unAttr.stOverlayEx.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Width = regionWidth;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Height = regionHeight;
    stRgnAttr.unAttr.stOverlayEx.u32BgColor = 0x0;
    stRgnAttr.unAttr.stOverlayEx.u32CanvasNum = 1;
    ret = HI_MPI_RGN_Create(rgnHandle, &stRgnAttr);
    if (ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_RGN_Create error! s32Ret = %#x\n", ret);
        return ret;
    }

    /* 设置RGN绑定通道的信息 */
    stChn.enModId = HI_ID_VO;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;

    /* 设置RGN绑定通道的属性 */
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAYEX_RGN;

    /*起点位置需要16字节对齐*/
    stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = ALIGN_UP(startX, 16);
    stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = ALIGN_UP(startY, 16);
    stChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 0;      /* ARGB1555:A为0时有效 */
    stChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 255;    /* ARGB1555:A为1时有效 */
    stChnAttr.unChnAttr.stOverlayExChn.u32Layer = 0;            //ex的必须为0
#endif

    /* 将RGN模块绑定到通道上 */
    ret = HI_MPI_RGN_AttachToChn(rgnHandle, &stChn, &stChnAttr);
    if (ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_RGN_AttachToChn failed! s32Ret: 0x%x.\n", ret);
        return ret;
    }

    return ret;
}

int Media_Osd_Init(void)
{
    int ret = 0;

    /* 初始化ft和ttf */
    ret = Media_Osd_InitFT("/opt/monaco.ttf");
    if (0 != ret)
    {
        prtMD("Media_Osd_InitFT error! ret = %#x\n", ret);
        return ret;
    }

    /* 初始化时间OSD对应的region模块 */
    ret = Media_Osd_InitRgn(OSD_TYPE_TIME);
    if (0 != ret)
    {
        prtMD("Media_Osd_InitRgn error! ret = %#x\n", ret);
        return ret;
    }

    /* 初始化OSD主线程 */
    ret = Media_Osd_InitTsk();
    if (0 != ret)
    {
        prtMD("Media_Osd_InitTsk error! ret = %#x\n", ret);
        return ret;
    }

    return ret;
}

int Media_Osd_InitTsk(void)
{
    int ret = 0;
    pthread_t pThrd;

    /* 创建时间任务主线程 */
    ret = pthread_create(&pThrd, NULL, Media_Osd_Tsk, NULL);
    if (0 != ret)
    {
        prtMD("creat Tsk_Cmd error! errno = %d\n", errno);
    }

    return ret;
}

int Media_Osd_StartTime(void)
{
    bShowTime = true;
    return 0;
}

int Media_Osd_StopTime(void)
{
    bShowTime = false;
    return 0;
}

int Media_Osd_StringToPic(char *pStrings, unsigned int RGB, OSD_PIC_INFO_S *pRgnInfo)
{
    SDL_PixelFormat stFormat = {0};
    SDL_Color stColor = {0};
    SDL_Surface *pText = NULL;
    SDL_Surface *pTemp = NULL;
    unsigned int i = 0;

    /* 字符串宽高 */
    unsigned int w = 0;
    unsigned int h = 0;

    /* 有效数据宽高 */
    unsigned int validW = 0;
    unsigned int validH = 0;
    unsigned int stride = 0;

    if ((pStrings == NULL) || (pRgnInfo == NULL))
    {
        prtMD("invalid input param! pStrings = %p, pRgnInfo = %p\n", pStrings, pRgnInfo);
        return -1;
    }

    if (pRgnInfo->pVirAddr == NULL)
    {
        prtMD("invalid input pVirAddr = %p\n", pRgnInfo->pVirAddr);
        return -1;
    }

    stColor.a = 0xFF;
    stColor.r = 0xFF;   //((RGB >> 16) & 0xFF);
    stColor.g = 0xFF;   //((RGB >> 16) & 0xFF);
    stColor.b = 0xFF;   //((RGB >> 16) & 0xFF);

    pText = TTF_RenderUTF8_Solid(pOsdFont, pStrings, stColor);
    if (pText == NULL)
    {
        prtMD("TTF_RenderUTF8_Solid error!\n");
        return -1;
    }

    stFormat.BitsPerPixel = 16;
    stFormat.BytesPerPixel = 2;
    stFormat.format = SDL_PIXELFORMAT_ABGR1555;

    pTemp = SDL_ConvertSurface(pText, &stFormat, 0);
    if (pTemp == NULL)
    {
        prtMD("SDL_ConvertSurface error!\n");
    }

    stride = pRgnInfo->u32stride;

    w = pTemp->w;
    h = pTemp->h;

    /* 计算拷贝有效区域 */
    validW = pRgnInfo->u32Width > w ? w : pRgnInfo->u32Width;
    validH = pRgnInfo->u32Height > h ? h : pRgnInfo->u32Height;

    /* 拷贝 */
    for (i = 0; i < validH; i++)
    {
        memcpy(pRgnInfo->pVirAddr + i * stride, pTemp->pixels + i * pTemp->pitch, validW * stFormat.BytesPerPixel);
    }

    SDL_FreeSurface(pText);
    SDL_FreeSurface(pTemp);

    return 0;
}

