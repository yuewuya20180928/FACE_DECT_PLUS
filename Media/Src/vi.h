#ifndef __VI_H__
#define __VI_H__

#include "media_com.h"
#include "vi_comm.h"
#include "media_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int Media_VideoIn_GetConfig(MEDIA_VI_PARAM_S *pViParam, unsigned int sensorNum);

int Media_VideoIn_StartMipiRx(MEDIA_VI_PARAM_S *pViParam);

int Media_VideoIn_SetPipeMode(void);

int Media_Vi_GetDevAttr(VI_SNS_TYPE_E enSnsType, VI_DEV_ATTR_S* pstViDevAttr);

int Media_Vi_StartDev(MEDIA_VI_INFO_S *pViInfo);

int Media_Vi_BindPipe(MEDIA_VI_INFO_S *pViInfo);

int Media_Vi_StartPipe(MEDIA_VI_INFO_S* pViInfo);

int Media_VideoIn_StopSingleViPipe(VI_PIPE ViPipe);

int Media_VideoIn_StopDev(MEDIA_VI_INFO_S* pViInfo);

int Media_VideoIn_GetPipeAttr(VI_SNS_TYPE_E enSnsType, VI_PIPE_ATTR_S *pPipeAttr);

int Media_VideoIn_StopSinglePipe(VI_PIPE ViPipe);

int Media_VideoIn_StartPipe(MEDIA_VI_INFO_S* pViInfo);

int Media_VideoIn_GetChnAttr(VI_SNS_TYPE_E enSnsType, VI_CHN_ATTR_S* pstChnAttr);

int Media_VideoIn_StartChn(MEDIA_VI_INFO_S* pViInfo);

int Media_VideoIn_CreateVi(MEDIA_VI_INFO_S *pViInfo);

int Media_VideoIn_StopChn(MEDIA_VI_INFO_S *pViInfo);

int Media_VideoIn_StopPipe(MEDIA_VI_INFO_S* pViInfo);

int Media_VideoIn_DestroySingleVi(MEDIA_VI_INFO_S *pViInfo);

int Media_VideoIn_StartVi(MEDIA_VI_PARAM_S *pViParam);

int Media_VideoIn_Rotate(MEDIA_VI_INFO_S *pViInfo, ROTATION_E enRotation);

int Media_VideoIn_SetRotate(MEDIA_VI_PARAM_S *pViParam, ROTATION_E enRotation);

int Media_VideoIn_Init(MEDIA_VI_PARAM_S *pViParam);

int Media_VideoIn_GetSnsWH(VI_SNS_TYPE_E enSnsType, unsigned int *pWidth, unsigned int *pHeight);

#ifdef __cplusplus
}
#endif




#endif

