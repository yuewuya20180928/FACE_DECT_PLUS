#include "media_com.h"
#include "osd.h"
#include "SDL_ttf.h"
#include "SDL_surface.h"

static bool bShowTime = false;      /* 时间显示标志:1-显示时间;0-不显示时间 */

int Media_Osd_Char2Pic_TEST(char *pString)
{
    TTF_Font *pFont = NULL;
    SDL_Surface *pTextSurface = NULL;
    SDL_Surface *pTempSurface = NULL;
    SDL_PixelFormat *pFormat = NULL;
    SDL_Color foreColor= {0xFF, 0xFF, 0xFF, 0xFF};
    int ret = 0;

    if (NULL == pString)
    {
        prtMD("invalid input pString = %p\n", pString);
        return -1;
    }

    /* Initialize the TTF library */
    if (TTF_Init() < 0)
    {
        prtMD("TTF_Init error!\n");
        return -1;
    }

    /* load ttf */
    pFont = TTF_OpenFont("/opt/monaco.ttf", 48);
    if (pFont == NULL)
    {
        prtMD("TTF_OpenFont error!\n");
        ret = -1;
        goto exit1;
    }

    /* 字符串转图片 */
    pTextSurface = TTF_RenderUTF8_Solid(pFont, pString, foreColor);
    if (NULL == pTextSurface)
    {
        prtMD("TTF_RenderUTF8_Solid error!\n");
        ret = -1;
        goto exit2;
    }

    pFormat = (SDL_PixelFormat*)malloc(sizeof(SDL_PixelFormat));
    if (NULL == pFormat)
    {
        prtMD("malloc error! errno = %d\n", errno);
        ret = -1;
        goto exit3;
    }

    memset(pFormat,0,sizeof(SDL_PixelFormat));
    pFormat->BitsPerPixel = 16;
    pFormat->BytesPerPixel = 2;
    pFormat->Rmask = 0x00ff0000;//0x00FF0000
    pFormat->Gmask = 0x0000ff00;//0x0000FF00
    pFormat->Bmask = 0x000000ff;//0x000000FF
    pFormat->Amask = 0xff000000;

    pTempSurface = SDL_ConvertSurface(pTextSurface, pFormat, 0);
    if (NULL == pTempSurface)
    {
        prtMD("SDL_ConvertSurface error!\n");
        ret = -1;
        goto exit4;
    }
    else
    {
        ret = SDL_SaveBMP(pTempSurface, "/opt/sdl_osd.bmp");
        if (0 != ret)
        {
            prtMD("SDL_SaveBMP error! ret = %#x\n", ret);
            ret = -1;
            goto exit5;
        }
    }

exit5:
    if (NULL != pTempSurface)
    {
        SDL_FreeSurface(pTempSurface);
    }

exit4:
    if (NULL != pFormat)
    {
        free(pFormat);
        pFormat = NULL;
    }

exit3:
    if (NULL != pTextSurface)
    {
        SDL_FreeSurface(pTextSurface);
    }

exit2:
    if (NULL != pFont)
    {
        TTF_CloseFont(pFont);
    }

exit1:
    TTF_Quit();

    return ret;
}

int Media_Osd_Char2Pic(const char *pStrings, OSD_SIZE_E enOsdSize, OSD_PIC_PARAM_S *pPicParam)
{
    if ((pStrings == NULL) || (pPicParam == NULL))
    {
        return -1;
    }

    return 0;
}

void *Tsk_TimeOsd(void *argc)
{
    time_t lastTime = 0;
    time_t newTime = 0;
    struct tm *pTimeNow = NULL;
    char stTimeString[64] = {0};
    unsigned int testFlag = 0;
    int ret = 0;

    /* 1秒钟更新一下时间 */
    while(1)
    {
        time(&newTime);
        if (newTime != lastTime)
        {
            lastTime = newTime;
            pTimeNow = localtime(&lastTime);

            if (pTimeNow != NULL)
            {
                snprintf(stTimeString,64, "[%d-%d-%d] %d:%d:%d\n",
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

            /* 使用SDL2_image开源库实现字符串转图片,图片通过region叠加到VPSS上 */
            /* TODO */


            /* 存图测试 */
            if (testFlag == 0)
            {
                testFlag = 1;
                ret = Media_Osd_Char2Pic_TEST(stTimeString);
                if (ret != 0)
                {
                    prtMD("Media_Osd_Char2Pic_TEST error! ret = %#x\n", ret);
                }
            }
        }

        usleep(100 * 1000);
    }
}

int Media_Osd_InitTsk(void)
{
    int ret = 0;
    pthread_t pThrd;

    /* 创建时间任务主线程 */
    ret = pthread_create(&pThrd, NULL, Tsk_TimeOsd, NULL);
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

