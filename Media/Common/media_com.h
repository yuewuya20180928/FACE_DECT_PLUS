/**********************************************************************************************************************
*   media_com.h
*
*   THOULION tech
*
*   Description:
*       - 硬件说明:
*
*       - 程序结构体说明:
*
*       - 使用说明: 媒体模块公共头文件
*
*       - 局限性说明:
*
*       - 其他说明:
*
*   Modification:
*       - Data    : 2020-6-2
*       - Revision: V1.0.0
*       - Author  : fanyaofeng
*       - Contents: creat
*
*********************************************************************************************************************/

#ifndef __MEDIA_COM_H__
#define __MEDIA_COM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include "hi_common.h"
#include "hi_buffer.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vpss.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_comm_ive.h"
#include "hi_defines.h"
#include "hi_comm_hdmi.h"
#include "hi_mipi.h"
#include "hi_comm_vgs.h"
#include "hi_md.h"
#include "hi_mipi_tx.h"
#include "hi_tde_type.h"
#include "hi_tde_api.h"
#include "mpi_ive.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_region.h"
#include "mpi_audio.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "hi_math.h"
#include "hi_sns_ctrl.h"
#include "mpi_hdmi.h"
#include "mpi_vgs.h"
#include "hi_ive.h"
#include "ivs_md.h"
#include "acodec.h"

#ifdef __cplusplus
extern "C" {
#endif


#define prtMD(fmt...)   \
    do {\
        printf("[MEDIA] file:[%s][%d]: ", __FILE__, __LINE__);\
        printf(fmt);\
    }while(0)

#define CHECK_NULL_PTR(ptr)\
    do{\
        if(NULL == ptr)\
        {\
            printf("func:%s,line:%d, NULL pointer\n",__FUNCTION__,__LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

#ifdef __cplusplus
}
#endif



#endif

