#ifndef __ISP_H__
#define __ISP_H__

#include "media_com.h"
#include "vi_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ISP_NUM 2

int Media_Isp_GetAttr(VI_SNS_TYPE_E enSnsType, ISP_PUB_ATTR_S *pPubAttr);

ISP_SNS_OBJ_S* Media_Isp_GetSnsObj(VI_SNS_TYPE_E enSnsType);

int Media_Isp_RegiterSensorCallback(ISP_DEV IspDev, VI_SNS_TYPE_E enSnsType);

int Media_Isp_BindSensor(ISP_DEV IspDev, VI_SNS_TYPE_E enSnsType, HI_S8 s8SnsDev);

int Media_Isp_Aelib_Callback(ISP_DEV IspDev);

int Media_Isp_Awblib_Callback(ISP_DEV IspDev);

void* Media_Isp_Thread(void* param);

int Media_Isp_Run(ISP_DEV IspDev);

int Media_VideoIn_StartIsp(MEDIA_VI_INFO_S* pViInfo);

int Media_Isp_Init(MEDIA_VI_PARAM_S *pViParam);







#ifdef __cplusplus
}
#endif



#endif

