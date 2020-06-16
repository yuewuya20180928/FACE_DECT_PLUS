#include "sys_init.h"



int Media_Sys_Init(MEDIA_POOL_S *pPool)
{
    unsigned int i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    MEDIA_VB_S *pVbInfo = NULL;
    VB_CONFIG_S stVbConfig = {0};

    if (pPool == NULL)
    {
        prtMD("invalid input pPool = %p\n", pPool);
        return -1;
    }

    if (pPool->poolNum >= MAX_POOL_COUNT)
    {
        prtMD("invalid poolNum = %d\n", pPool->poolNum);
        return -1;
    }

    for (i = 0; i < pPool->poolNum; i++)
    {
        pVbInfo = pPool->stVbInfo + i;
        if (pVbInfo == NULL)
        {
            prtMD("invalid pVbInfo = %p, i = %d\n", pVbInfo, i);
            return -1;
        }

        if ((pVbInfo->count > MAX_VB_COUNT) || (pVbInfo->width > MAX_VB_WIDTH) || (pVbInfo->height > MAX_VB_HEIGHT))
        {
            prtMD("invalid param! i = %d, w = %d, h = %d, count = %d\n", i, pVbInfo->width, pVbInfo->height, pVbInfo->count);
            return -1;
        }

        stVbConfig.astCommPool[i].u64BlkSize = ALIGN_UP(pVbInfo->width, 16) * ALIGN_UP(pVbInfo->height, 16) * 3 >> 1;
        stVbConfig.astCommPool[i].u32BlkCnt = pVbInfo->count;
        stVbConfig.u32MaxPoolCnt ++;
    }

    /* 先去初始化海思系统模块 */
    HI_MPI_SYS_Exit();

    /* 去初始化VB */
    HI_MPI_VB_Exit();

    /* 设置VB配置 */
    s32Ret = HI_MPI_VB_SetConfig(&stVbConfig);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VB_SetConfig error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    /* 初始化VB */
    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VB_Init error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    /* 初始化系统 */
    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_SYS_Init error! s32Ret = %#x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

