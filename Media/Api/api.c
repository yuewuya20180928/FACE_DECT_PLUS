#include "api.h"
#include "common.h"
#include "media_api.h"
#include <string.h>

static unsigned int bApiFirstInited = 0;
static CMD_STATUS_S *pApiCmd = NULL;
static void *pApiBuf = NULL;

int DSP_API_Init(void)
{
    void *pAddr = NULL;
    int shmid = -1;

    if (bApiFirstInited == 1)
    {
        return 0;
    }

    shmid = shmget(SHM_CMD_KEY, SHM_CMD_LENTH, IPC_CREAT);
    if (shmid == -1)
    {
        printf("[client] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pAddr = shmat(shmid, NULL, 0);
    if (pAddr == NULL)
    {
        printf("[client] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pApiCmd = (CMD_STATUS_S *)pAddr;

    shmid = shmget(SHM_CMD_BUF_KEY, SHM_CMD_BUF_LEN, IPC_CREAT);
    if (shmid == -1)
    {
        printf("[client] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pAddr = shmat(shmid, NULL, 0);
    if (pAddr == NULL)
    {
        printf("[client] Func:%s, Line:%d, errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    pApiBuf = pAddr;

    bApiFirstInited = 1;

    printf("DSP_API_Init OK! %d\n", pApiCmd->bNewCmd);

    return 0;
}

int DSP_API_SendCmd(HOST_CMD_S *pHostCmd)
{
    unsigned int waitCount = 0;

    if (pHostCmd == NULL)
    {
        printf("invalid input pHostCmd = %p\n", pHostCmd);
        return -1;
    }

    while (pApiCmd->bNewCmd == 1)
    {
        usleep(50 * 1000);

        waitCount ++;

        if (waitCount >= 100)
        {
            return -1;
        }
    }

    pApiCmd->cmdIdx = pHostCmd->cmdIdx;
    pApiCmd->chan = pHostCmd->chan;
    pApiCmd->param = pHostCmd->param;
    pApiCmd->bufLenth = pHostCmd->bufLenth;
    memcpy(pApiBuf, pHostCmd->pBuf, pHostCmd->bufLenth);

    pApiCmd->bNewCmd = 1;

    return 0;
}

int DSP_Init(void)
{
    return DSP_API_Init();
}

int DSP_SetMedia(INIT_PARAM_S *pMediaParam)
{
    int ret = 0;
    HOST_CMD_S stCmdSend = {0};

    /* 参数有效性校验 */
    if (pMediaParam == NULL)
    {
        printf("invalid input pMediaParam = %p\n", pMediaParam);
        return -1;
    }

    /* 对命令结构体参数赋值 */
    stCmdSend.cmdIdx = HOST_CMD_DSP_INIT;
    stCmdSend.bHaveBuf = 1;
    stCmdSend.bReturn = 1;
    stCmdSend.bufLenth = sizeof(INIT_PARAM_S);
    stCmdSend.pBuf = (void *)pMediaParam;

    /* 赋值完成后调用发送接口发送主机命令 */
    ret = DSP_API_SendCmd(&stCmdSend);
    if (0 != ret)
    {
        printf("DSP_API_SendCmd error! ret = %#x\n", ret);
        return ret;
    }

    return 0;
}

int DSP_SetVideoDisp(SENSOR_TYPE_E sensorIdx, MEDIA_VIDEO_DISP_S *pDispParam)
{
    int ret = 0;
    HOST_CMD_S stCmdSend = {0};

    if (pDispParam == NULL)
    {
        printf("invalid input pDispParam = %p\n", pDispParam);
        return -1;
    }

    /* 对命令结构体参数赋值 */
    stCmdSend.cmdIdx = HOST_CMD_DSP_SET_DISP;
    stCmdSend.bHaveBuf = 1;
    stCmdSend.chan = sensorIdx;
    stCmdSend.bReturn = 1;
    stCmdSend.bufLenth = sizeof(MEDIA_VIDEO_DISP_S);
    stCmdSend.pBuf = (void *)pDispParam;

    /* 赋值完成后调用发送接口发送主机命令 */
    ret = DSP_API_SendCmd(&stCmdSend);
    if (0 != ret)
    {
        printf("DSP_API_SendCmd error! ret = %#x\n", ret);
        return ret;
    }

    return 0;
}

int DSP_SetRecord(SENSOR_TYPE_E sensorIdx, VIDEO_STREAM_E videoStreamType, MEDIA_RECORD_S *pRecord)
{
    int ret = 0;
    HOST_CMD_S stCmdSend = {0};

    if (pRecord == NULL)
    {
        printf("invalid input pRecord = %p\n", pRecord);
        return -1;
    }

    /* 对命令结构体参数赋值 */
    stCmdSend.cmdIdx = HOST_CMD_DSP_SET_RECORD;
    stCmdSend.bHaveBuf = 1;
    stCmdSend.chan = sensorIdx;
    stCmdSend.param = videoStreamType;
    stCmdSend.bReturn = 1;
    stCmdSend.bufLenth = sizeof(MEDIA_RECORD_S);
    stCmdSend.pBuf = (void *)pRecord;

    /* 赋值完成后调用发送接口发送主机命令 */
    ret = DSP_API_SendCmd(&stCmdSend);
    if (0 != ret)
    {
        printf("DSP_API_SendCmd error! ret = %#x\n", ret);
        return ret;
    }

    return 0;
}

int DSP_SetTime(unsigned int bDispTime)
{
    int ret = 0;
    HOST_CMD_S stCmdSend = {0};

    /* 对命令结构体参数赋值 */
    stCmdSend.cmdIdx = HOST_CMD_DSP_SHOWTIME;
    stCmdSend.bHaveBuf = 0;
    stCmdSend.chan = 0;
    stCmdSend.param = bDispTime;
    stCmdSend.bReturn = 0;
    stCmdSend.bufLenth = 0;
    stCmdSend.pBuf = NULL;

    /* 赋值完成后调用发送接口发送主机命令 */
    ret = DSP_API_SendCmd(&stCmdSend);
    if (0 != ret)
    {
        printf("DSP_API_SendCmd error! ret = %#x\n", ret);
        return ret;
    }

    return 0;
}

