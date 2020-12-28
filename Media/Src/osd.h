#ifndef __OSD_H__
#define __OSD_H__



#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OSD_WIDTH 640               /* OSD最大宽度 */
#define MAX_OSD_HEIGHT 64               /* OSD最大高度 */

/* OSD类型枚举 */
typedef enum
{
    OSD_TYPE_TIME = 0,                  /* 时间OSD */

    OSD_TYPE_BUTT,                      /* 无效值 */
}OSD_TYPE_E;

/* 区域叠加属性结构体 */
typedef struct
{
    unsigned char *pVirAddr;            /* 数据缓冲首(虚拟)地址 */
    unsigned int u32Width;              /* 有效宽度 */
    unsigned int u32Height;             /* 有效高度 */
    unsigned int u32stride;             /* 存储跨度 */
    unsigned int startX;                /* 数据添加起点X坐标 */
    unsigned int starty;                /* 数据添加起点Y坐标 */
}OSD_PIC_INFO_S;

/* OSD参数结构体 */
typedef struct
{
    OSD_TYPE_E enType;                  /* OSD类型 */
    OSD_PIC_INFO_S stPicInfo;           /* 图像信息 */
    unsigned int rgnHandle;             /* region handle */
}OSD_PARAM_S;

/* OSD尺寸 */
typedef enum
{
    OSD_SIZE_16x16 = 0,
    OSD_SIZE_32x32,
    OSD_SIZE_48x48,
    OSD_SIZE_64x64,

    OSD_SIZE_BUTT,
}OSD_SIZE_E;


int Media_Osd_InitFT(char *pTtfString);
void *Media_Osd_Tsk(void *argc);
int Media_Osd_InitRgn(OSD_TYPE_E enType);
int Media_Osd_Init(void);
int Media_Osd_InitTsk(void);
int Media_Osd_StartTime(void);
int Media_Osd_StopTime(void);
int Media_Osd_StringToPic(char *pStrings, unsigned int RGB, OSD_PIC_INFO_S *pRgnInfo);


#ifdef __cplusplus
}
#endif



#endif

