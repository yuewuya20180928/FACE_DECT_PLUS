#ifndef __MIPI_RX_H__
#define __MIPI_RX_H__

#include "media_com.h"
#include "vi_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIPI_DEV_NODE       "/dev/hi_mipi"

int Media_MipiRx_Start(MEDIA_VI_PARAM_S *pMediaViConfig);






#ifdef __cplusplus
}
#endif







#endif


