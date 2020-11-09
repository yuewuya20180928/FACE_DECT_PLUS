#ifndef __OSD_H__
#define __OSD_H__



#ifdef __cplusplus
extern "C" {
#endif

/* OSD尺寸 */
typedef enum
{
    OSD_SIZE_16x16 = 0,
    OSD_SIZE_32x32,
    OSD_SIZE_48x48,
    OSD_SIZE_64x64,

    OSD_SIZE_BUTT,
}OSD_SIZE_E;

/* BMP参数信息 */
typedef struct
{
    unsigned int w;                     /* 图片宽 */
    unsigned int stride;                /* 对齐宽度 */
    unsigned int h;                     /* 图片高 */
    unsigned char *pVirAddr;            /* 虚拟地址 */
    unsigned long long phyAAddr;        /* 物理地址 */
}OSD_PIC_PARAM_S;


int Media_Osd_Char2Pic_TEST(char *pString);
int Media_Osd_Char2Pic(const char *pStrings, OSD_SIZE_E enOsdSize, OSD_PIC_PARAM_S *pPicParam);
int Media_Osd_InitTsk(void);
int Media_Osd_StartTime(void);
int Media_Osd_StopTime(void);

#ifdef __cplusplus
}
#endif



#endif

