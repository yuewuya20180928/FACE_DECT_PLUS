#ifndef __MEDIA_API_H__
#define __MEDIA_API_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SENSOR_NUMBER 2

/* 定义码流类型 */
#define STREAM_TYPE_VIDEO 0x0001                /* 视频流 */
#define STREAM_TYPE_AUDIO 0x0002                /* 音频流 */
#define STREAM_TYPE_PRIVT 0x0004                /* 私有流 */

/* 枚举sensor类型 */
typedef enum
{
    MEDIA_SENSOR_RGB = 0,
    MEDIA_SENSOR_IR,
    MEDIA_SENSOR_BUTT,
}MEDIA_SENSOR_E;

typedef struct
{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
}MEDIA_RECT_S;

typedef enum
{
    MEDIA_ROTATION_0 = 0,
    MEDIA_ROTATION_90,
    MEDIA_ROTATION_180,
    MEDIA_ROTATION_270,
}MEDIA_ROTATION_E;

typedef struct
{
    bool bOpen;
    MEDIA_ROTATION_E enRotation;
    MEDIA_RECT_S stRect;
}MEDIA_VIDEO_DISP_S;

/* 枚举视频流类型 */
typedef enum
{
    VIDEO_STREAM_MAIN = 0,          /* 主码流 */
    VIDEO_STREAM_SUB,               /* 子码流 */

    VIDEO_STREAM_BUTT,
}VIDEO_STREAM_E;

typedef enum
{
    ENCODE_TYPE_H264 = 0,           /* H264 */
    ENCODE_TYPE_H265,               /* H265 */
    ENCODE_TYPE_JPEG,               /* JPEG */

    ENCODE_TYPE_BUTT,
}ENCODE_TYPE_E;

typedef struct
{
    bool bOpen;                     /* 开启或关闭 */
    ENCODE_TYPE_E enType;           /* 编码类型 */
    unsigned int w;                 /* 宽 */
    unsigned int h;                 /* 高 */
    unsigned int fps;               /* 帧率 */
    unsigned int gop;               /* 帧组 */
    unsigned int bitRate;           /* 码率,单位:Kb(小b) */
    unsigned int bitType;           /* 码率类型:暂时只支持CBR */
}VIDEO_PARAM_S;

/* 枚举音频采样率 */
typedef enum
{
    SAMPLE_RATE_8K = 0,             /* 8K */
    SAMPLE_RATE_16K,                /* 16K */
    SAMPLE_RATE_32K,                /* 32K */
    SAMPLE_RATE_44_1K,              /* 44.1K */

    SAMPLE_RATE_BUTT,
}SAMPLE_RATE_E;

/* 声道数 */
typedef enum
{
    AUDIO_SOUND_SINGLE = 0,         /* 单声道 */
    AUDIO_SOUND_STEREO,             /* 双声道 */

    AUDIO_SOUND_BUTT,
}AUDIO_SOUND_E;

/* 音频编码类型 */
typedef enum
{
    AUDIO_TYPE_G711A = 0,           /* G711A */
    AUDIO_TYPE_G711U,               /* G711U */
    AUDIO_TYPE_AAC,                 /* AAC */
    AUDIO_TYPE_MP3,                 /* MP3 */

    AUDIO_TYPE_BUTT,
}AUDIO_TYPE_E;

/* 音频参数结构体 */
typedef struct
{
    bool bOpen;                     /* 开启或关闭 */
    AUDIO_SOUND_E enSoundmode;      /* 音频声道数 */
    unsigned int sampleRate;        /* 采样率 */
    unsigned int bitWidth;          /* 位宽 */
    AUDIO_TYPE_E enType;            /* 音频编码类型 */
}AUDIO_PARAM_S;

/* 录像参数结构体 */
typedef struct
{
    int streamType;                 /* 码流类型 */
    VIDEO_PARAM_S stVideoParam;     /* 视频参数 */
    AUDIO_PARAM_S stAudioParam;     /* 音频参数 */
}MEDIA_RECORD_S;

int Media_Init(unsigned int sensorNumber);

int Media_SetVideoDisp(MEDIA_SENSOR_E sensorIdx, MEDIA_VIDEO_DISP_S *pDispParam);

int Media_SetRecord(MEDIA_SENSOR_E sensorIdx, VIDEO_STREAM_E videoStreamType, MEDIA_RECORD_S *pRecord);

#ifdef __cplusplus
}
#endif



#endif

