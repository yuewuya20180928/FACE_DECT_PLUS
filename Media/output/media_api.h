#ifndef __MEDIA_API_H__
#define __MEDIA_API_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SENSOR_NUMBER 2

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

int Media_Init(unsigned int sensorNumber);

int Media_SetVideoDisp(MEDIA_SENSOR_E sensorIdx, MEDIA_VIDEO_DISP_S *pDispParam);


#ifdef __cplusplus
}
#endif



#endif

