#ifndef __MIPI_TX_H__
#define __MIPI_TX_H__

#include "media_com.h"
#include "media_api.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MIPI_TX_DEV "/dev/hi_mipi_tx"

int Media_MipiTx_Init(MEDIA_LCD_IDX_E lcdTypeIdx);






#ifdef __cplusplus
}
#endif



#endif

