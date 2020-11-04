#ifndef __COMMON_H__
#define __COMMON_H__



#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>


#define HOST_CMD_MAX_NUM 255            /* 最大的主机命令个数 */


#define SHM_CMD_KEY 0x77777701          /* 主机命令 */
#define SHM_CMD_BUF_KEY 0x77777702      /* 主机命令内存缓冲 */


#define SHM_CMD_LENTH 0x1000            /* 主机命令长度 */
#define SHM_CMD_BUF_LEN 0x100000        /* 主机命令数据缓冲长度 */


typedef int (*HOST_DSP_CMD_FUNC)(unsigned int chan, unsigned int param, void *pBuf);

/* 命令号 */
typedef enum
{
    HOST_CMD_DSP_INIT = 0,              /* 初始化主机命令 */
    HOST_CMD_DSP_SET_DISP,              /* 显示设置 */
    HOST_CMD_DSP_SET_RECORD,            /* 设置录像 */

    HOST_CMD_DSP_BUTT,
}CMD_IDX_E;

/* 命令信息 */
typedef struct
{
    unsigned int bNewCmd;               /* 是否有新的命令 */
    unsigned int cmdIdx;                /* 命令号 */
    unsigned int chan;                  /* 命令通道 */
    unsigned int param;                 /* 命令参数 */
    unsigned int bufLenth;              /* 命令缓冲长度 */
}CMD_STATUS_S;



#ifdef __cplusplus
}
#endif



#endif


