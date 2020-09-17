#ifndef __MIPI_TX_H__
#define __MIPI_TX_H__

#include "media_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LCD_IDX_7_600X1024_RP = 0,
    LCD_IDX_3_480X640,

    LCD_IDX_BUTT,
}MEDIA_LCD_IDX_E;

#define MIPI_TX_DEV "/dev/hi_mipi_tx"

int Media_MipiTx_Init(MEDIA_LCD_IDX_E lcdTypeIdx);






#ifdef __cplusplus
}
#endif



#endif

