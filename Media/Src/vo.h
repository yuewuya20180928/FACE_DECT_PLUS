#ifndef __VO_H__
#define __VO_H__

#include "media_com.h"
#include "media_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LCD_IDX_7_600X1024_RP = 0,
    LCD_IDX_BUTT,
}MEDIA_LCD_IDX_E;

typedef struct
{
    unsigned int w;
    unsigned int h;
}MEDIA_SIZE_S;

#define COLOR_RGB_RED      0xFF0000
#define COLOR_RGB_GREEN    0x00FF00
#define COLOR_RGB_BLUE     0x0000FF
#define COLOR_RGB_BLACK    0x000000
#define COLOR_RGB_YELLOW   0xFFFF00
#define COLOR_RGB_CYN      0x00ffff
#define COLOR_RGB_WHITE    0xffffff

int Media_Vo_Init(void);

int Media_Vo_InitDev(VO_DEV VoDev);

int Media_Vo_InitVoLayer(VO_LAYER VoLayer, MEDIA_LCD_IDX_E enLcdIdx);

int Media_Vo_InitMipiTx(void);

int Media_Vo_StartChn(VO_LAYER VoLayer, VO_CHN voChn, MEDIA_VIDEO_DISP_S *pDispParam);

#ifdef __cplusplus
}
#endif



#endif

