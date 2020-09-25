/*
* Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
* Description:
* Author: Hisilicon multimedia software group
* Create: 2011/06/28
*/

#if !defined(__GC2145_CMOS_H_)
#define __GC2145_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "gc2145_cmos_ex.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define GC2145_ID                             2145  //修改20200629

/****************************************************************************
 * global variables                                                            *
 ****************************************************************************/

#define HIGH_8BITS(x)   (((x) & 0xFF00) >> 8)
#define LOW_8BITS(x)    ( (x) & 0x00FF)
#define LOWER_4BITS(x)  ( (x) & 0x000F)
#define HIGHER_4BITS(x) (((x) & 0xF000) >> 12)
#define HIGHER_8BITS(x) (((x) & 0x0FF0) >> 4)

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif

ISP_SNS_STATE_S *g_pastGc2145[ISP_MAX_PIPE_NUM] = {HI_NULL};

#define GC2145_SENSOR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastGc2145[dev])
#define GC2145_SENSOR_SET_CTX(dev, pstCtx)   (g_pastGc2145[dev] = pstCtx)
#define GC2145_SENSOR_RESET_CTX(dev)         (g_pastGc2145[dev] = HI_NULL)

ISP_SNS_COMMBUS_U g_aunGc2145BusInfo[ISP_MAX_PIPE_NUM] = {
    [0] = { .s8I2cDev = 0},
    [1 ... ISP_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

static ISP_FSWDR_MODE_E genFSWDRMode[ISP_MAX_PIPE_NUM] = {
    [0 ... ISP_MAX_PIPE_NUM - 1] = ISP_FSWDR_NORMAL_MODE
};

static HI_U32 gu32MaxTimeGetCnt[ISP_MAX_PIPE_NUM] = {0};
static HI_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static HI_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};

static HI_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static HI_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16InitCCM[ISP_MAX_PIPE_NUM][CCM_MATRIX_SIZE] = {{0}};

/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern const unsigned int gc2145_i2c_addr;
extern unsigned int gc2145_addr_byte;
extern unsigned int gc2145_data_byte;

extern void gc2145_init(VI_PIPE ViPipe);
extern void gc2145_exit(VI_PIPE ViPipe);
extern void gc2145_standby(VI_PIPE ViPipe);
extern void gc2145_restart(VI_PIPE ViPipe);
extern int  gc2145_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  gc2145_read_register(VI_PIPE ViPipe, int addr);
extern void gc2145_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/

#define GC2145_FULL_LINES_MAX                 (0x3FFF)

/* GC2145 Register Address */
#define GC2145_EXPTIME_ADDR_H                 (0x03) // Shutter-time[12:8]//快门时间      无需修改
#define GC2145_EXPTIME_ADDR_L                 (0x04) // Shutter-time[7:0]//20200713

#define GC2145_AGAIN_ADDR_H                   (0xb4)   // Analog-gain[9:8]//模拟增益
#define GC2145_AGAIN_ADDR_L                   (0xb3)   // Analog-gain[7:0]//没有模拟增益啊20200713

#define GC2145_DGAIN_ADDR_H                   (0xb9)   // digital-gain[11:6]//数字增益
#define GC2145_DGAIN_ADDR_L                   (0xb8)   // digital-gain[5:0]//没有数字增益啊20200713

#define GC2145_AUTO_PREGAIN_ADDR_H            (0xb1)   // auto-pregain-gain[9:6]自动前增益       无需修改
#define GC2145_AUTO_PREGAIN_ADDR_L            (0xb2)   // auto-pregain-gain[5:0]//20200713

#define GC2145_VMAX_ADDR_H                    (0x95)    // Vmax[10:8]
#define GC2145_VMAX_ADDR_L                    (0x96)    // Vmax[7:0]
#define GC2145_STATUS_ADDR                    (0x90)    // slow shutter via framerate or exptime//状态地址

#define GC2145_INCREASE_LINES                 (0) /* make real fps less than stand fps because NVR require */

#define GC2145_VMAX_UXGA_30_LINEAR            (1232 + GC2145_INCREASE_LINES)

#define GC2145_VMAX_960P_30_LINEAR            (1005 + GC2145_INCREASE_LINES)

#define GC2145_SENSOR_UXGA_30FPS_LINEAR_MODE  (0)

#define GC2145_SENSOR_960P_30FPS_LINEAR_MODE  (1)


/* sensor fps mode */
#define GC2145_SENSOR_1080P_30FPS_LINEAR_MODE (0)

static HI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstAeSnsDft);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);
    //AE是自动增益  这里先注释掉

    memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));

    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FlickerFreq = 50 * 256;
    pstAeSnsDft->u32FullLinesMax = GC2145_FULL_LINES_MAX;
    pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * 30);

    pstAeSnsDft->u8AERunInterval  = 1;
    pstAeSnsDft->u32InitExposure  = 10;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 1;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 1;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

    pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_0;
    pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_32_0;

    pstAeSnsDft->bAERouteExValid = HI_FALSE;
    pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
    pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;

    if (g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * 30 / 2;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }

    switch (pstSnsState->enWDRMode)
    {
        case WDR_MODE_NONE:
            pstAeSnsDft->au8HistThresh[0] = 0x0D;
            pstAeSnsDft->au8HistThresh[1] = 0x28;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            pstAeSnsDft->u32MaxAgain = 113168;
            pstAeSnsDft->u32MinAgain = 1024;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 512;
            pstAeSnsDft->u32MinDgain = 64;
            pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

            pstAeSnsDft->u8AeCompensation = 0x38;
            pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;

            pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 148859;

            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 3;
            pstAeSnsDft->u32MaxIntTimeTarget = 65535;
            pstAeSnsDft->u32MinIntTimeTarget = 3;

            printf("Func:%s, Line:%d, u32InitExposure = %d\n", __FUNCTION__, __LINE__, pstAeSnsDft->u32InitExposure);

            break;
        default:
            printf("cmos_get_ae_default_Sensor Mode is error!\n");
            break;
    }

    return 0;
}

/* the function of sensor set fps */
static HI_VOID cmos_fps_set(VI_PIPE ViPipe, HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    HI_U32 u32VMAX = GC2145_VMAX_UXGA_30_LINEAR;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    switch (pstSnsState->u8ImgMode)
    {
        case GC2145_SENSOR_UXGA_30FPS_LINEAR_MODE:
            if ((f32Fps <= 30) && (f32Fps > 2.06))
            {
                u32VMAX = GC2145_VMAX_UXGA_30_LINEAR * 30 / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                ISP_ERR_TRACE("Not support Fps: %f\n", f32Fps);
                return;
            }
            pstAeSnsDft->u32LinesPer500ms = GC2145_VMAX_UXGA_30_LINEAR * 15;
            break;

        case GC2145_SENSOR_960P_30FPS_LINEAR_MODE:
            if ((f32Fps <= 30) && (f32Fps > 2.06))
            {
                u32VMAX = GC2145_VMAX_960P_30_LINEAR * 30 / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                ISP_ERR_TRACE("Not support Fps: %f\n", f32Fps);
                return;
            }
            pstAeSnsDft->u32LinesPer500ms = GC2145_VMAX_960P_30_LINEAR * 15;
            break;
        default:
            break;
    }
    u32VMAX = (u32VMAX > GC2145_FULL_LINES_MAX) ? GC2145_FULL_LINES_MAX : u32VMAX;
    pstSnsState->u32FLStd = u32VMAX;
    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = 0x0;
    pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = (u32VMAX & 0xFF);
    pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = ((u32VMAX & 0xFF00) >> 8);

    pstAeSnsDft->f32Fps = f32Fps;
    //pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
    pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * DIV_0_TO_1_FLOAT(f32Fps));

    return;
}

static HI_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, HI_U32 u32FullLines,
                                       AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32FullLines = (u32FullLines > GC2145_FULL_LINES_MAX) ? GC2145_FULL_LINES_MAX : u32FullLines;
    pstSnsState->au32FL[0] = u32FullLines;

    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = 0x0;
    pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = (u32FullLines & 0xFF);
    pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = ((u32FullLines & 0xFF00) >> 8);

    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] - 2;
    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
//无需修改
static HI_VOID cmos_inttime_update(VI_PIPE ViPipe, HI_U32 u32IntTime)
{
    HI_U32 u32Val = 0;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32Val = (u32IntTime > GC2145_FULL_LINES_MAX) ? GC2145_FULL_LINES_MAX : u32IntTime;

    pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = (u32Val & 0xFF);
    pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = ((u32Val & 0xFF00) >> 8);  //高位曝光时间只有4bit

    return;
}

static HI_U32 regValTable[29][4] = {
    {0x00, 0x00, 0x01, 0x00}, // 0xb4 0xb3 0xb8 0xb9
    {0x00, 0x10, 0x01, 0x0c},
    {0x00, 0x20, 0x01, 0x1b},
    {0x00, 0x30, 0x01, 0x2c},
    {0x00, 0x40, 0x01, 0x3f},
    {0x00, 0x50, 0x02, 0x16},
    {0x00, 0x60, 0x02, 0x35},
    {0x00, 0x70, 0x03, 0x16},
    {0x00, 0x80, 0x04, 0x02},
    {0x00, 0x90, 0x04, 0x31},
    {0x00, 0xa0, 0x05, 0x32},
    {0x00, 0xb0, 0x06, 0x35},
    {0x00, 0xc0, 0x08, 0x04},
    {0x00, 0x5a, 0x09, 0x19},
    {0x00, 0x83, 0x0b, 0x0f},
    {0x00, 0x93, 0x0d, 0x12},
    {0x00, 0x84, 0x10, 0x00},
    {0x00, 0x94, 0x12, 0x3a},
    {0x01, 0x2c, 0x1a, 0x02},
    {0x01, 0x3c, 0x1b, 0x20},
    {0x00, 0x8c, 0x20, 0x0f},
    {0x00, 0x9c, 0x26, 0x07},
    {0x02, 0x64, 0x36, 0x21},
    {0x02, 0x74, 0x37, 0x3a},
    {0x00, 0xc6, 0x3d, 0x02},
    {0x00, 0xdc, 0x3f, 0x3f},
    {0x02, 0x85, 0x3f, 0x3f},
    {0x02, 0x95, 0x3f, 0x3f},
    {0x00, 0xce, 0x3f, 0x3f},
};

static HI_U32 analog_gain_table[29] = {//逻辑增益表
    1024, 1230, 1440, 1730, 2032, 2380, 2880, 3460, 4080, 4800, 5776,
    6760, 8064, 9500, 11552, 13600, 16132, 18912, 22528, 27036, 32340,
    38256, 45600, 53912, 63768, 76880, 92300, 108904, 123568,
};

static HI_VOID cmos_again_calc_table(VI_PIPE ViPipe, HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
    int again;
    int i;
    static HI_U8 againmax = 28;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);
    again = *pu32AgainLin;

    if (again >= analog_gain_table[againmax])
    {
        *pu32AgainLin = analog_gain_table[28];
        *pu32AgainDb = againmax;
    }
    else
    {
        for (i = 1; i < 29; i++)
        {
            if (again < analog_gain_table[i])
            {
                *pu32AgainLin = analog_gain_table[i - 1];
                *pu32AgainDb = i - 1;
                break;
            }
        }
    }

    return;
}

static HI_VOID cmos_gains_update(VI_PIPE ViPipe, HI_U32 u32Again, HI_U32 u32Dgain)
{
    HI_U8 u8DgainHigh, u8DgainLow;

    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);
    u8DgainHigh = (u32Dgain >> 6) & 0x0f;
    u8DgainLow = (u32Dgain & 0x3f) << 2;

    pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = regValTable[u32Again][0];
    pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = regValTable[u32Again][1];
    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = regValTable[u32Again][2];
    pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = regValTable[u32Again][3];

    pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = u8DgainLow;
    pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = u8DgainHigh;

    return;
}

static HI_VOID cmos_get_inttime_max(VI_PIPE ViPipe, HI_U16 u16ManRatioEnable, HI_U32 *au32Ratio, HI_U32 *au32IntTimeMax, HI_U32 *au32IntTimeMin, HI_U32 *pu32LFMaxIntTime)
{
    return;
}

/* Only used in LINE_WDR mode */
static HI_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    CMOS_CHECK_POINTER_VOID(pstAeFSWDRAttr);

    genFSWDRMode[ViPipe] = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;

    return;
}
//ae经验函数  不需要修改
static HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));
    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = HI_NULL;
    pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
    pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

    return HI_SUCCESS;
}


/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN                          0
#define GOLDEN_BGAIN                          0
//暂时不修改
static HI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    HI_S32 i;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstAwbSnsDft);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);
//自动增益暂时注释掉

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
    pstAwbSnsDft->u16WbRefTemp = 4908;

    pstAwbSnsDft->au16GainOffset[0] = 388; // R
    pstAwbSnsDft->au16GainOffset[1] = 256; // G
    pstAwbSnsDft->au16GainOffset[2] = 256; // G
    pstAwbSnsDft->au16GainOffset[3] = 519; // B

    pstAwbSnsDft->as32WbPara[0] = 39;
    pstAwbSnsDft->as32WbPara[1] = 152;
    pstAwbSnsDft->as32WbPara[2] = -65;
    pstAwbSnsDft->as32WbPara[3] = 201612;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -147924;
    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;
    switch (pstSnsState->enWDRMode) {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
            memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
            break;
    }
    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];
    pstAwbSnsDft->u16InitRgain = g_au16InitWBGain[ViPipe][0];
    pstAwbSnsDft->u16InitGgain = g_au16InitWBGain[ViPipe][1];
    pstAwbSnsDft->u16InitBgain = g_au16InitWBGain[ViPipe][2];
    pstAwbSnsDft->u8AWBRunInterval = 4;

    for (i = 0; i < CCM_MATRIX_SIZE; i++) {
        pstAwbSnsDft->au16InitCCM[i] = g_au16InitCCM[ViPipe][i];
    }

    return HI_SUCCESS;
}
//awb经验公式
static HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));
    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return HI_SUCCESS;
}

//暂时不修改
static ISP_CMOS_DNG_COLORPARAM_S g_stDngColorParam = {
    {378, 256, 430},
    {439, 256, 439}
};
//暂时不修改
static HI_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstDef);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
#ifdef CONFIG_HI_ISP_CA_SUPPORT
    pstDef->unKey.bit1Ca       = 1;
    pstDef->pstCa              = &g_stIspCA;
#endif
    pstDef->unKey.bit1Clut    = 1;
    pstDef->pstClut           = &g_stIspCLUT;
    pstDef->unKey.bit1Dpc      = 1;
    pstDef->pstDpc             = &g_stCmosDpc;
    pstDef->unKey.bit1Wdr      = 1;
    pstDef->pstWdr             = &g_stIspWDR;
#ifdef CONFIG_HI_ISP_HLC_SUPPORT
    pstDef->unKey.bit1Hlc      = 0;
    pstDef->pstHlc             = &g_stIspHlc;
#endif
    pstDef->unKey.bit1Lsc      = 1;
    pstDef->pstLsc             = &g_stCmosLsc;
#ifdef CONFIG_HI_ISP_EDGEMARK_SUPPORT
    pstDef->unKey.bit1EdgeMark = 0;
    pstDef->pstEdgeMark        = &g_stIspEdgeMark;
#endif
#ifdef CONFIG_HI_ISP_PREGAMMA_SUPPORT
    pstDef->unKey.bit1PreGamma = 0;
    pstDef->pstPreGamma        = &g_stPreGamma;
#endif
    switch (pstSnsState->enWDRMode) {
        default:
        case WDR_MODE_NONE:
            pstDef->unKey.bit1Demosaic       = 1;
            pstDef->pstDemosaic              = &g_stIspDemosaic;
            pstDef->unKey.bit1Sharpen        = 1;
            pstDef->pstSharpen               = &g_stIspYuvSharpen;
            pstDef->unKey.bit1Drc            = 1;
            pstDef->pstDrc                   = &g_stIspDRC;
            pstDef->unKey.bit1BayerNr        = 1;
            pstDef->pstBayerNr               = &g_stIspBayerNr;
            pstDef->unKey.bit1AntiFalseColor = 1;
            pstDef->pstAntiFalseColor        = &g_stIspAntiFalseColor;
            pstDef->unKey.bit1Ldci           = 1;
            pstDef->pstLdci                  = &g_stIspLdci;
            pstDef->unKey.bit1Gamma          = 1;
            pstDef->pstGamma                 = &g_stIspGamma;
            pstDef->unKey.bit1Detail         = 1;
            pstDef->pstDetail                = &g_stIspDetail;
#ifdef CONFIG_HI_ISP_CR_SUPPORT
            pstDef->unKey.bit1Ge             = 1;
            pstDef->pstGe                    = &g_stIspGe;
#endif
            pstDef->unKey.bit1Dehaze         = 1;
            pstDef->pstDehaze                = &g_stIspDehaze;

            memcpy(&pstDef->stNoiseCalibration,   &g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));
            break;
    }

    pstDef->stSensorMode.u32SensorID = GC2145_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;


    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    switch (pstSnsState->u8ImgMode) {
        default:
        case GC2145_SENSOR_UXGA_30FPS_LINEAR_MODE:
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 10;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel = 1023;
            break;
    }

    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout = CFALAYOUT_TYPE_RECTANGULAR;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstDef->stSensorMode.bValidDngRawFormat = HI_TRUE;

    return HI_SUCCESS;
}

//不需要修改
static HI_S32 cmos_get_isp_black_level(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstBlackLevel);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstBlackLevel->bUpdate = HI_FALSE;
    pstBlackLevel->au16BlackLevel[0] = 256;
    pstBlackLevel->au16BlackLevel[1] = 256;
    pstBlackLevel->au16BlackLevel[2] = 256;
    pstBlackLevel->au16BlackLevel[3] = 256;
    return HI_SUCCESS;

}

//无需修改
static HI_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, HI_BOOL bEnable)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    return;
}

//无需修改
static HI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, HI_U8 u8Mode)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);
    pstSnsState->bSyncInit = HI_FALSE;

    switch (u8Mode)
    {
        case WDR_MODE_NONE:
            pstSnsState->enWDRMode = WDR_MODE_NONE;
            break;

        default:
            ISP_ERR_TRACE("cmos_set_wdr_mode_NOT support this mode!\n");
            return HI_FAILURE;
    }

    printf("File:%s, Func:%s, Line:%d, u8Mode = %d\n", __FILE__, __FUNCTION__, __LINE__, u8Mode);

    memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));

    return HI_SUCCESS;
}

static HI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    HI_S32 i;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstSnsRegsInfo);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    if ((pstSnsState->bSyncInit == HI_FALSE) || (pstSnsRegsInfo->bConfig == HI_FALSE))
    {
        pstSnsState->astRegsInfo[0].enSnsType = ISP_SNS_I2C_TYPE;
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev = g_aunGc2145BusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        pstSnsState->astRegsInfo[0].u32RegNum = 7;
        for (i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
        {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr = gc2145_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = gc2145_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = gc2145_data_byte;
        }
        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr  = GC2145_EXPTIME_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr  = GC2145_EXPTIME_ADDR_H;

        //pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum  = 1;
        //pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr  = GC2145_AGAIN_ADDR_H;
        //pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum  = 1;
        //pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr  = GC2145_AGAIN_ADDR_L;

        //pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum  = 1;
        //pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr  = GC2145_DGAIN_ADDR_L;
        //pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum  = 1;
        //pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr  = GC2145_DGAIN_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr  = GC2145_AUTO_PREGAIN_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr  = GC2145_AUTO_PREGAIN_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr = GC2145_STATUS_ADDR;

        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum  = 0;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr  = GC2145_VMAX_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum  = 0;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr  = GC2145_VMAX_ADDR_H;

        pstSnsState->bSyncInit = HI_TRUE;
    }
    else
    {
        for (i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum - 2; i++)
        {    // AEC/AGC_Update
            if (pstSnsState->astRegsInfo[0].astI2cData[i].u32Data == pstSnsState->astRegsInfo[1].astI2cData[i].u32Data) {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_FALSE;
            } else {

                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            }
        }

        if ((pstSnsState->astRegsInfo[0].astI2cData[6].bUpdate == HI_TRUE) || (pstSnsState->astRegsInfo[0].astI2cData[7].bUpdate == HI_TRUE)) {
            pstSnsState->astRegsInfo[0].astI2cData[6].bUpdate = HI_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[7].bUpdate = HI_TRUE;
        }

        pstSnsState->astRegsInfo[0].astI2cData[9].bUpdate = HI_TRUE;
        pstSnsState->astRegsInfo[0].astI2cData[10].bUpdate = HI_TRUE;
    }

    pstSnsRegsInfo->bConfig = HI_FALSE;
    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return HI_SUCCESS;
}

static HI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode = 0;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstSensorImageMode);
    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    u8SensorImageMode = pstSnsState->u8ImgMode;
    pstSnsState->bSyncInit = HI_FALSE;

    if ((pstSensorImageMode->u16Width <= 1280) && (pstSensorImageMode->u16Height <= 960))
    {
        if (pstSensorImageMode->f32Fps <= 30)
        {
            u8SensorImageMode     = GC2145_SENSOR_960P_30FPS_LINEAR_MODE;
            pstSnsState->u32FLStd = GC2145_VMAX_960P_30_LINEAR;
        }
        else
        {
            ISP_ERR_TRACE("Not support! Width:%d, Height:%d, Fps:%f\n",
                      pstSensorImageMode->u16Width,
                      pstSensorImageMode->u16Height,
                      pstSensorImageMode->f32Fps);
            return HI_FAILURE;
        }
    }
    else if ((pstSensorImageMode->u16Width <= 1600) && (pstSensorImageMode->u16Height <= 1200))
    {
        if (pstSensorImageMode->f32Fps <= 30)
        {
            u8SensorImageMode     = GC2145_SENSOR_UXGA_30FPS_LINEAR_MODE;
            pstSnsState->u32FLStd = GC2145_VMAX_UXGA_30_LINEAR;
        }
        else
        {
            ISP_ERR_TRACE("Not support! Width:%d, Height:%d, Fps:%f\n",
                      pstSensorImageMode->u16Width,
                      pstSensorImageMode->u16Height,
                      pstSensorImageMode->f32Fps);
            return HI_FAILURE;
        }
    }
    else
    {
        ISP_ERR_TRACE("Not support! Width:%d, Height:%d, Fps:%f\n",
                  pstSensorImageMode->u16Width,
                  pstSensorImageMode->u16Height,
                  pstSensorImageMode->f32Fps);
        return HI_FAILURE;
    }

    printf("Func:%s, Line:%d, w:%d, h:%d, fps:%f\n",
        __FUNCTION__,
        __LINE__,
        pstSensorImageMode->u16Width,
        pstSensorImageMode->u16Height,
        pstSensorImageMode->f32Fps);

    if ((pstSnsState->bInit == HI_TRUE) && (u8SensorImageMode == pstSnsState->u8ImgMode))
    {
        return ISP_DO_NOT_NEED_SWITCH_IMAGEMODE;
    }

    pstSnsState->u8ImgMode = u8SensorImageMode;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return HI_SUCCESS;
}

static HI_VOID sensor_global_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    pstSnsState->bInit     = HI_FALSE;
    pstSnsState->bSyncInit = HI_FALSE;
    pstSnsState->u8ImgMode = GC2145_SENSOR_UXGA_30FPS_LINEAR_MODE;
    pstSnsState->enWDRMode = WDR_MODE_NONE;
    pstSnsState->u32FLStd = GC2145_VMAX_UXGA_30_LINEAR;
    pstSnsState->au32FL[0] = GC2145_VMAX_UXGA_30_LINEAR;
    pstSnsState->au32FL[1] = GC2145_VMAX_UXGA_30_LINEAR;

    memset(&pstSnsState->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&pstSnsState->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));

    return;
}

static HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));
    pstSensorExpFunc->pfn_cmos_sensor_init = gc2145_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit = gc2145_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    return HI_SUCCESS;
}

/****************************************************************************
 * callback structure    //不需要修改                                                   *
 ****************************************************************************/
static HI_S32 gc2145_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunGc2145BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return HI_SUCCESS;
}

//不需要修改
static HI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

    if (pastSnsStateCtx == HI_NULL)
    {
        pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
        if (pastSnsStateCtx == HI_NULL)
        {
            ISP_ERR_TRACE("Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
            return HI_ERR_ISP_NOMEM;
        }
    }

    memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

    GC2145_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

    return HI_SUCCESS;
}

//不需要修改
static HI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = HI_NULL;

    GC2145_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    GC2145_SENSOR_RESET_CTX(ViPipe);

    return;
}

//不需要修改
static HI_S32 sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = sensor_ctx_init(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    stSnsAttrInfo.eSensorId = GC2145_ID;

    s32Ret  = cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret |= HI_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor register callback function failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret |= HI_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret |= HI_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

//不需要修改
static HI_S32 sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(ViPipe, GC2145_ID);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, GC2145_ID);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, GC2145_ID);
    if (s32Ret != HI_SUCCESS)
    {
        ISP_ERR_TRACE("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    sensor_ctx_exit(ViPipe);

    return HI_SUCCESS;
}

//不需要修改
static HI_S32 sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    HI_S32 i;
    CMOS_CHECK_POINTER(pstInitAttr);

    g_au32InitExposure[ViPipe]  = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe]   = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe]   = pstInitAttr->u16SampleBgain;

    for (i = 0; i < CCM_MATRIX_SIZE; i++)
    {
        g_au16InitCCM[ViPipe][i] = pstInitAttr->au16CCM[i];
    }

    printf("File:%s, Func:%s, Line:%d, sensor_set_init\n", __FILE__, __FUNCTION__, __LINE__);
    printf("ViPipe  = %d, u32Exposure = %d\n", ViPipe, pstInitAttr->u32Exposure);

    return HI_SUCCESS;
}

ISP_SNS_OBJ_S stSnsGc2145Obj = {
    .pfnRegisterCallback    = sensor_register_callback,
    .pfnUnRegisterCallback  = sensor_unregister_callback,
    .pfnStandby             = gc2145_standby,
    .pfnRestart             = gc2145_restart,
    .pfnMirrorFlip          = gc2145_mirror_flip,
    .pfnWriteReg            = gc2145_write_register,
    .pfnReadReg             = gc2145_read_register,
    .pfnSetBusInfo          = gc2145_set_bus_info,
    .pfnSetInit             = sensor_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif
#endif /* __GC2145_CMOS_H_ */

