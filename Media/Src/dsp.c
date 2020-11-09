#include "dsp.h"
#include "common.h"
#include "media_com.h"
#include "media_api.h"

static bool bCmdTskInited = false;
static CMD_STATUS_S *pCmdStatus = NULL;
static void *pDspBuf = NULL;                        /* 命令缓冲数据 */
static DSP_CMD_S stHostCmd[HOST_CMD_MAX_NUM] =
{
    {HOST_CMD_DSP_INIT, 1, 1, sizeof(INIT_PARAM_S), Media_Init},
    {HOST_CMD_DSP_SET_DISP, 1, 1, sizeof(MEDIA_VIDEO_DISP_S), Media_SetVideoDisp},
    {HOST_CMD_DSP_INIT, 1, 1, sizeof(MEDIA_RECORD_S), Media_SetRecord},
    {HOST_CMD_DSP_SHOWTIME, 1, 1, 0, Media_SetTime},
};

int InitDspCmd(void)
{
    int shmid = 0;
    void *pAddr = NULL;

    /* 创建主机命令对应的共享内存对象 */
    shmid = shmget(SHM_CMD_KEY, SHM_CMD_LENTH, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1)
    {
        prtMD("[server] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pAddr = shmat(shmid, NULL, 0);
    if (pAddr == NULL)
    {
        prtMD("[server] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pCmdStatus = (CMD_STATUS_S *)pAddr;

    shmid = shmget(SHM_CMD_BUF_KEY, SHM_CMD_BUF_LEN, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1)
    {
        prtMD("[server] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pAddr = shmat(shmid, NULL, 0);
    if (pAddr == NULL)
    {
        prtMD("[server] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pDspBuf = pAddr;

    return 0;
}

void *Tsk_Cmd(void *argc)
{
    HOST_DSP_CMD_FUNC pfunc = NULL;          /* 回调函数 */
    unsigned int cmdIdx = 0;

    if (pCmdStatus == NULL || pDspBuf == NULL)
    {
        prtMD("invalid input pCmdInfo = %p\n", pCmdStatus);
        return NULL;
    }

    while (1)
    {
        /*
            1. 通过共享内存获取应用传递来的命令索引
            2. 根据索引号获取DSP回调函数,调用回调函数
        */
        if (pCmdStatus->bNewCmd == 0)
        {
            usleep(10 * 1000);
            continue;
        }

        cmdIdx = pCmdStatus->cmdIdx;
        if (cmdIdx >= HOST_CMD_DSP_BUTT)
        {
            prtMD("invalid cmdIdx = %d\n", cmdIdx);
            pCmdStatus->bNewCmd = 0;
            usleep(5 * 1000);
            continue;
        }

        pfunc = stHostCmd[cmdIdx].pFunc;
        if (pfunc)
        {
            pfunc(pCmdStatus->chan, pCmdStatus->param, pDspBuf);
        }

        pCmdStatus->bNewCmd = 0;

        usleep(5 * 1000);
    }
}

int InitCmdTsk(void)
{
    int ret = 0;
    pthread_t pThrd;

    if (bCmdTskInited == true)
    {
        return 0;
    }

    bCmdTskInited = true;

    /* 创建主机命令处理线程 */
    ret = pthread_create(&pThrd, NULL, Tsk_Cmd, NULL);
    if (0 != ret)
    {
        prtMD("creat Tsk_Cmd error! errno = %d\n", errno);
    }

    return ret;
}

int main(int argc, char **argv)
{
    int ret = 0;

    /* 初始化共享内存模块 */
    ret = InitDspCmd();
    if (ret != 0)
    {
        prtMD("InitDspCmd error! ret = %#x\n", ret);
        return -1;
    }

    /* 初始化主机命令服务线程 */
    ret = InitCmdTsk();
    if (0 != ret)
    {
        prtMD("InitCmdTsk error! ret = %#x\n", ret);
        return -1;
    }

    while (1)
    {
        usleep(5000 * 1000);
        continue;
    }

    return 0;
}



