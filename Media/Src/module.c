#include <stdio.h>
#include <string.h>
#include "module.h"

MEDIA_MODULE_POOL_S modulePool;         /* 全局变量用于统计各个硬件模块的使用状况 */

int Media_Module_GetChan(MEDIA_MODULE_E module, int *pChnValue)
{
    static unsigned char bFirstInit = 0;
    unsigned int i = 0;
    unsigned int j = 0;

    if ((module <= HI_MODULE_BASE) || (module >= HI_MODULE_BUTT) || (pChnValue == NULL))
    {
        prtMD("invalid input module = %d >= HI_MODULE_BUTT(%d), pChnValue = %p\n", module, HI_MODULE_BUTT, pChnValue);
        return -1;
    }

    if (bFirstInit == 0)
    {
        /* 第一次调用接口时进行初始化操作 */
        memset(&modulePool, 0, sizeof(modulePool));
        pthread_mutex_init(&modulePool.mutex, NULL);
        bFirstInit = 1;
    }

    pthread_mutex_lock(&modulePool.mutex);

    /* 查找该模块是否有注册过 */
    for (i = 0;i < MAX_HD_MODULE_NUM; i++)
    {
        /* 查到一个空的模块或相等模块时返回 */
        if ((modulePool.poolInfo[i].module == module) || (modulePool.poolInfo[i].module == HI_MODULE_BASE))
        {
            break;
        }
    }

    if (i >= MAX_HD_MODULE_NUM)
    {
        pthread_mutex_unlock(&modulePool.mutex);
        return -1;
    }

    for (j = 0;j < MAX_POOL_NUM; j++)
    {
        if (modulePool.poolInfo[i].bUsed[j] == 0)
        {
            break;
        }
    }

    if (j >= MAX_POOL_NUM)
    {
        pthread_mutex_unlock(&modulePool.mutex);
        return -1;
    }

    modulePool.poolInfo[i].bUsed[j] = 1;
    *pChnValue = j;

    if (i == modulePool.NUM)
    {
        modulePool.poolInfo[i].module = module;
        modulePool.NUM ++;
    }

    pthread_mutex_unlock(&modulePool.mutex);

    return 0;
}

int Media_Module_ReleaseChan(MEDIA_MODULE_E module, int modChan)
{
    unsigned int i = 0;

    if ((module <= HI_MODULE_BASE) || (module >= HI_MODULE_BUTT) || (modChan >= MAX_POOL_NUM))
    {
        prtMD("invalid input module = %d >= HI_MODULE_BUTT(%d), modChan = %u\n", module, HI_MODULE_BUTT, modChan);
        return -1;
    }

    pthread_mutex_lock(&modulePool.mutex);

    /* 释放通道时只在曾经注册的模块内搜索 */
    for (i = 0;i < modulePool.NUM; i++)
    {
        if (modulePool.poolInfo[i].module == module)
        {
            break;
        }
    }

    if (i >= modulePool.NUM)
    {
        pthread_mutex_unlock(&modulePool.mutex);
        return -1;
    }

    modulePool.poolInfo[i].bUsed[modChan] = 0;

    pthread_mutex_unlock(&modulePool.mutex);

    return 0;
}

MOD_ID_E Media_Module_GetType(MEDIA_MODULE_E enModule)
{
    MOD_ID_E enMode = HI_ID_BUTT;
    switch (enModule)
    {
        case HI_DEC:
            enMode = HI_ID_VDEC;
            break;

        case HI_ENC:
            enMode = HI_ID_VENC;
            break;

        case HI_VPSS:
            enMode = HI_ID_VPSS;
            break;

        case HI_VO:
            enMode = HI_ID_VO;
            break;
        
        case HI_VI:
            enMode = HI_ID_VI;
            break;
        
        case HI_RGN:
            enMode = HI_ID_RGN;
            break;
        
        case HI_VGS:
            enMode = HI_ID_VGS;
            break;

        default:
            prtMD("unsupported enModule = %d\n", enModule);
            break;
    }

    return enMode;
}

int Media_Module_Bind(MEDIA_BIND_INFO_S *pSrc, MEDIA_BIND_INFO_S *pDst)
{
    HI_S32 s32Ret = HI_SUCCESS;

    MPP_CHN_S stSrcChn = {0};
    MPP_CHN_S stDestChn = {0};

    if ((pSrc == NULL) || (pDst == NULL))
    {
        prtMD("invalid input pSrc = %p, pDst = %p\n", pSrc, pDst);
        return -1;
    }

    stSrcChn.enModId   = Media_Module_GetType(pSrc->enModule);
    stSrcChn.s32DevId  = pSrc->devNo;
    stSrcChn.s32ChnId  = pSrc->chnNo;

    stDestChn.enModId  = Media_Module_GetType(pDst->enModule);
    stDestChn.s32DevId = pDst->devNo;
    stDestChn.s32ChnId = pDst->chnNo;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_SYS_Bind error! s32Ret = %#x\n", s32Ret);
    }
    
    return HI_SUCCESS;
}

