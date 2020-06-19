#ifndef __VI_COMM_H__
#define __VI_COMM_H__

#include "media_com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WDR_MAX_PIPE_NUM        4

typedef enum
{
    SONY_IMX327_MIPI_2M_30FPS_12BIT = 0,
    SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX327_2L_MIPI_2M_30FPS_12BIT,
    SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX327_2L_MIPI_720P_30FPS_12BIT,
	
    VI_SNS_TYPE_BUTT,
} VI_SNS_TYPE_E;

typedef struct
{
    VI_SNS_TYPE_E   enSnsType;
    HI_S32              s32SnsId;
    HI_S32              s32BusId;
    combo_dev_t           MipiDev;
} VI_SENSOR_INFO_S;

typedef struct
{
    VI_DEV      ViDev;
    WDR_MODE_E  enWDRMode;
} VI_DEV_INFO_S;

typedef struct
{
    VI_PIPE         aPipe[WDR_MAX_PIPE_NUM];
    VI_VPSS_MODE_E  enMastPipeMode;
    HI_BOOL         bMultiPipe;
    HI_BOOL         bVcNumCfged;
    HI_BOOL         bIspBypass;
    PIXEL_FORMAT_E  enPixFmt;
    HI_U32          u32VCNum[WDR_MAX_PIPE_NUM];
} VI_PIPE_INFO_S;

typedef struct
{
    VI_CHN              ViChn;
    PIXEL_FORMAT_E      enPixFormat;
    DYNAMIC_RANGE_E     enDynamicRange;
    VIDEO_FORMAT_E      enVideoFormat;
    COMPRESS_MODE_E     enCompressMode;
} VI_CHN_INFO_S;

typedef struct
{
    HI_BOOL  bSnap;
    HI_BOOL  bDoublePipe;
    VI_PIPE    VideoPipe;
    VI_PIPE    SnapPipe;
    VI_VPSS_MODE_E  enVideoPipeMode;
    VI_VPSS_MODE_E  enSnapPipeMode;
} VI_SNAP_INFO_S;

typedef struct
{
    VI_SENSOR_INFO_S    stSnsInfo;
    VI_DEV_INFO_S       stDevInfo;
    VI_PIPE_INFO_S      stPipeInfo;
    VI_CHN_INFO_S       stChnInfo;
    VI_SNAP_INFO_S      stSnapInfo;
} MEDIA_VI_INFO_S;

typedef struct
{
    MEDIA_VI_INFO_S     stViInfo[VI_MAX_DEV_NUM];
    HI_S32              s32WorkingViId[VI_MAX_DEV_NUM];
    HI_S32              s32WorkingViNum;
} MEDIA_VI_PARAM_S;






#ifdef __cplusplus
}
#endif



#endif

