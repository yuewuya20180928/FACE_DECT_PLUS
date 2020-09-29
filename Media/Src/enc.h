#ifndef __ENC_H__
#define __ENC_H__

#include "media_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ENC_CHAN 16

/* 定义编码参数结构体 */
typedef struct
{
    bool bOpen;                         /* 是否开启 */
    bool bTskCreat;                     /* 是否已创建线程 */
    unsigned int vpssGroup;             /* VPSS组号 */
    unsigned int vpssChan;              /* VPSS */
    unsigned int vencChan;              /* 通道号 */
    VIDEO_PARAM_S stVideo;              /* 编码参数 */
}ENC_PARAM_S;

int Media_Enc_StartVideo(unsigned int encChan, VIDEO_PARAM_S *pVideoParam);

int Media_Enc_SetAudio(MEDIA_SENSOR_E sensorIdx, VIDEO_STREAM_E videoStreamType, AUDIO_PARAM_S *pAudioParam);

int Media_Enc_StartTsk(unsigned int encChan);


#ifdef __cplusplus
}
#endif




#endif


