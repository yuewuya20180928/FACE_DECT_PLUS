#ifndef __ISP_CONFIGURE_H__
#define __ISP_CONFIGURE_H__

#include "hi_comm_isp.h"
#include "hi_type.h"
#include <stdbool.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define ISP_CFG_FILE_PREFIX "/opt/ispconfig_param_"

#define MAX_NUM_IN_LINE 5120

typedef struct
{
    HI_BOOL bitBypassISPDGain;
    HI_BOOL bitBypassAntiFC;
    HI_BOOL bitBypassCrosstalkR;
    HI_BOOL bitBypassDPC;
    HI_BOOL bitBypassNR;
    HI_BOOL bitBypassDehaze;
    HI_BOOL bitBypassWBGain;
    HI_BOOL bitBypassMeshShading;
    HI_BOOL bitBypassDRC;
    HI_BOOL bitBypassDemosaic;
    HI_BOOL bitBypassColorMatrix;
    HI_BOOL bitBypassGamma;
    HI_BOOL bitBypassFSWDR;
    HI_BOOL bitBypassCA;
    HI_BOOL bitBypassCsConv;
    HI_BOOL bitBypassSharpen;
    HI_BOOL bitBypassLCAC;
    HI_BOOL bitBypassGCAC;
    HI_BOOL bit2ChnSelect;
    HI_BOOL bitBypassLdci;
    HI_BOOL bitBypassPreGamma;
    HI_BOOL bitBypassAEStatFE;
    HI_BOOL bitBypassAEStatBE;
    HI_BOOL bitBypassMGSta;
    HI_BOOL bitBypassDE;
    HI_BOOL bitBypassAFStatBE;
    HI_BOOL bitBypassAWBStat;
    HI_BOOL bitBypassCLUT;
    HI_BOOL bitBypassHLC;
    HI_BOOL bitBypassEdgeMark;
}ISPCFG_BYPASS_PARAM_S;

typedef struct
{
    int enOpType;
    HI_U16 au16BlackLevel[4]; /* RW; Range: [0x0, 0xFFF];Format:12.0;Black level values that correspond to the black levels of the R,Gr, Gb, and B components respectively.*/
}ISPCFG_BLACK_PARAM_S;

//修改加载参数
typedef struct
{
    HI_BOOL bAERouteExValid;
    HI_U8 ExposureOpType;
    HI_U8 u8AERunInterval;
    HI_U32 u32AutoExpTimeMax;
    HI_U32 u32AutoExpTimeMin;
    HI_U32 u32AutoAGainMax;
    HI_U32 u32AutoAGainMin;
    HI_U32 u32AutoDGainMax;
    HI_U32 u32AutoDGainMin;
    HI_U32 u32AutoISPDGainMax;
    HI_U32 u32AutoISPDGainMin;
    HI_U32 u32AutoSysGainMax;
    HI_U32 u32AutoSysGainMin;
    HI_U32 u32AutoGainThreshold;
    HI_U8  u8AutoSpeed;
    HI_U16 u16AutoBlackSpeedBias;
    HI_U8  u8AutoTolerance;
    HI_U8  u8AutostaticCompesation;
    HI_U16 u16AutoEVBias;
    HI_BOOL bAutoAEStrategMode;
    HI_U16 u16AutoHistRatioSlope;
    HI_U8  u8AutoMaxHistOffset;
    HI_U16 u16AutoBlackDelayFrame;
    HI_U16 u16AutoWhiteDelayFrame;  //原先已经存在的
    HI_BOOL enFSWDRMode;
} ISPCFG_AE_PARAM_S;

typedef struct
{
    HI_U8  u8ExpRatioType;
    HI_U32 u32ExpRatioMax;
    HI_U32 u32ExpRatioMin;
    HI_U16 u16Tolerance;
    HI_U16 u16Speed;
    HI_U16 u16RatioBias;
    HI_U32 au32ExpRatio[3];
} ISPCFG_WDR_AE_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_U8  u8CurveSelect;
    HI_U8  u8DRCOpType;
    HI_U8  u8PDStrength;
    HI_U8  u8LocalMixingBrightMax;
    HI_U8  u8LocalMixingBrightMin;
    HI_U8  u8LocalMixingBrightThr;
    HI_S8  s8LocalMixingBrightSlo;
    HI_U8  u8LocalMixingDarkMax;
    HI_U8  u8LocalMixingDarkMin;
    HI_U8  u8LocalMixingDarkThr;
    HI_S8  s8LocalMixingDarkSlo;
    HI_U8  u8BrightGainLmt;
    HI_U8  u8BrightGainLmtStep;
    HI_U8  u8DarkGainLmtY;
    HI_U8  u8DarkGainLmtC;
    HI_U16 au16ToneMappingValue[10][20];
    HI_U8  u8ContrastControl;
    HI_S8  s8DetailAdjustFactor;
    HI_U8  u8SpatialFltCoef;
    HI_U8  u8RangeFltCoef;
    HI_U8  u8RangeAdaMax;
    HI_U8  u8GradRevMax;
    HI_U8  u8GradRevThr;
    int    enOpType;
    HI_U16 u16ManualStrength;
    HI_U16 u16AutoStrength;
    HI_U16 u16AutoStrengthMax;
    HI_U16 u16AutoStrengthMin;
    HI_U8  u8Asymmetry;
    HI_U8  u8SecondPole;
    HI_U8  u8Stretch;
    HI_U8  u8Compress;
} ISPCFG_DRC_PARAM_S;

typedef struct
{
    HI_U8    CcmOpType;
    HI_BOOL  bISOActEn;
    HI_BOOL  bTempActEn;
    HI_U16   u16CCMTabNum;
    ISP_COLORMATRIX_PARAM_S astCCMTab[7];
}ISPCFG_CCM_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_U8   u8DehazeOpType;
    HI_BOOL bUserLutEnable;
    HI_U8   u8ManualStrength;
    HI_U8   u8Autostrength;
    HI_U16  u16prfltIncrCoef;
    HI_U16  u16prfltDecrCoef;
    HI_U8   au8DehazeLut[256];
} ISPCFG_DEHAZE_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_U8   u8LDCIOpType;
    HI_U16  u16ManualBlcCtrl;
    HI_U16  u16AutoBlcCtrl[16];
} ISPCFG_LDCI_PARAM_S;

typedef struct
{
    HI_U8 au8AEWeight[AE_ZONE_ROW][AE_ZONE_COLUMN];
} ISPCFG_STATISTICS_PARAM_S;

//add awb para
typedef struct
{
    HI_U8   AWBOpType;
    HI_BOOL AWBEnable;
    HI_U16  u16AutoRefColorTemp;
    HI_U16  au16AutoStaticWB[4];
    HI_S32  as32AutoCurvePara[6];
    HI_U8   u8AutoRGStrength;
    HI_U8   u8AutoBGStrength;
    HI_U16  u16AutoSpeed;
    HI_U16  u16AutoZoneSel;
    HI_U16  u16AutoHighColorTemp;
    HI_U16  u16AutoLowColorTemp;
} ISPCFG_AWB_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    COLOR_GAMUT_E enColorGamut;  //色域属性
    HI_U8 u8Hue;
    HI_U8 u8Luma;
    HI_U8 u8Contr;
    HI_U8 u8Satu;
} ISPCFG_CSC_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
} ISPCFG_SHADING_PARAM_S;

typedef struct
{
    HI_BOOL bOpType;
    HI_U8   u8ManualSat;
    HI_U8   au8AutoSat[ISP_AUTO_ISO_STRENGTH_NUM];
} ISPCFG_SATURATION_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    int     enOpType;
    HI_U16  ManualTextureFreq;
    HI_U16  ManualEdgeFreq;
    HI_U8   ManualOvershoot;
    HI_U8   ManualUnderShoot;
    HI_U8   ManualSupStr;
    HI_U8   ManualSupAdj;
    HI_U8   ManualDetailCtrl;
    HI_U8   ManualDetailCtrlThr;
    HI_U8   ManualEdgeFiltStr;
    HI_U16  AutoTextureFreq[16];
    HI_U16  AutoEdgeFreq[16];
    HI_U8   au8OverShoot[16];
    HI_U8   au8UnderShoot[16];
    HI_U8   AutoSupStr[16];
    HI_U8   AutoSupAdj[16];
    HI_U8   AutoDetailCtrl[16];
    HI_U8   AutoDetailCtrlThr[16];
    HI_U8   AutoEdgeFiltStr[16];
    HI_U8   AutoEdgeFiltMaxCap[16];
    HI_U8   AutoRGain[16];
    HI_U8   AutoGGain[16];
    HI_U8   AutoBGain[16];
    HI_U8   AutoSkinGain[16];
    HI_U8   AutoMaxSharpGain[16];
    HI_U8   AutoLumaWgt[ISP_SHARPEN_LUMA_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16TextureStr[ISP_SHARPEN_GAIN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16EdgeStr[ISP_SHARPEN_GAIN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
} ISPCFG_SHARPEN_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    int     enOpType;
    HI_U8   u8ManualNonDirStr;
    HI_U8   u8ManualNonDirMFDetailEhcStr;
    HI_U8   u8ManualNonDirHFDetailEhcStr;
    HI_U16  u16ManualDetailSmoothStr;
    HI_U8   u8AutoNonDirStr[16];
    HI_U8   u8AutoNonDirMFDetailEhcStr[16];
    HI_U8   u8AutoNonDirHFDetailEhcStr[16];
    HI_U16  u16AutoDetailSmoothRange[16];
    HI_U8   u8AutoColorNoiseThdF[16];
    HI_U8   u8AutoColorNoiseStrF[16];
    HI_U8   u8AutoColorNoiseThdY[16];
    HI_U8   u8AutoColorNoiseStrY[16];
} ISPCFG_DEMOSAIC_PARAM_S;

typedef struct
{
    #if 0
    HI_U32 u32InterVal;
    HI_U32 u32TotalNum;
    HI_U64 au64ExpThreshLtoH[10];
    HI_U64 au64ExpThreshHtoL[10];
    HI_U16 au16Table[10][1025];
    #endif
    HI_BOOL bEnable;
    HI_U16 u16Table[GAMMA_NODE_NUM];
    int enCurveType;
} ISPCFG_GAMMA_PARAM_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_BOOL bNrLscEnable;
    HI_U8   u8BnrMaxGain;
    HI_U16  u16BnrLscCmpStrength;
    HI_U8   u8BnrOptype;
    HI_U8   au8ChromaStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8   au8FineStr[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16CoringWgt[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16CoarseStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8   au8WDRFrameStr[ISP_BAYER_CHN_NUM];
}ISPCFG_BNR_PARAM_S;

typedef struct
{
    HI_U8   enWDRMergeMode;
    HI_BOOL bCombineMotionComp;
    HI_U16  u16CombineShortThr;
    HI_U16  u16CombineLongThr;
    HI_BOOL bCombineForceLong;
    HI_U16  u16CombineForceLongLowThr;
    HI_U16  u16CombineForceLongHigThr;
    HI_BOOL bWDRMdtShortExpoChk;
    HI_U16  u16WDRMdtShortCheckThd;
    HI_BOOL bWDRMdtMDRefFlicker;
    HI_U8   u8WDRMdtMdtStillThd;
    HI_U8   u8WDRMdtMdtLongBlend;
    HI_U8   u8WDRMdtOpType;
    HI_U8   au8AutoMdThrLowGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8   au8AutoMdThrHigGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  u16FusionFusionThr[4];
} ISPCFG_WDRFS_PARAM_S;

typedef struct
{
    bool bOpen;
    VI_PIPE_NRX_PARAM_S stNrParam;
}ISPCFG_VI_NR_PARAM_S;

typedef struct
{
    bool bOpen;
    VPSS_GRP_NRX_PARAM_S stNrParam;
}ISPCFG_VPSS_NR_PARAM_S;

typedef struct
{
    ISPCFG_BYPASS_PARAM_S stBypassParam;
    ISPCFG_BLACK_PARAM_S stBlackParam;
    ISPCFG_AE_PARAM_S stAeParam;
    ISPCFG_WDR_AE_PARAM_S stWdrAeParam;
    ISPCFG_DRC_PARAM_S stDrcParam;
    ISPCFG_CCM_PARAM_S stCcmParam;
    ISPCFG_DEHAZE_PARAM_S stDehazeParam;
    ISPCFG_LDCI_PARAM_S stLdciParam;
    ISPCFG_STATISTICS_PARAM_S stStatisticsParam;
    ISPCFG_AWB_PARAM_S stAwbParam;
    ISPCFG_CSC_PARAM_S stCscParam;
    ISPCFG_SHADING_PARAM_S stShadingParam;
    ISPCFG_SATURATION_PARAM_S stSaturationParam;
    ISPCFG_SHARPEN_PARAM_S stSharpenParam;
    ISPCFG_DEMOSAIC_PARAM_S stDemosaicParam;
    ISPCFG_VI_NR_PARAM_S stViNrxParam;
    ISPCFG_VPSS_NR_PARAM_S stVpssNrxParam;
    ISPCFG_GAMMA_PARAM_S stGammaParam;
    ISPCFG_WDRFS_PARAM_S stWdrfsParam;
    ISPCFG_BNR_PARAM_S stBnrParam;
} ISPCFG_PARAM_S;

int ISPCFG_LoadFile(VI_PIPE viPipe, char *pFileName);
int ISPCFG_UnloadFile(VI_PIPE viPipe);
int ISPCFG_GetValue(const VI_PIPE viPipe, const char *pString);
int ISPCFG_SetValue(const VI_PIPE viPipe, const char *pKeyString,char *pValue);
int ISPCFG_GetString(const VI_PIPE viPipe, char **pString, const char *pKey);
int ISPCFG_SetLongFrameAE(VI_PIPE ViPipe);
int ISPCFG_SetStatistics(VI_PIPE ViPipe);
int ISPCFG_SetDemosaic(VI_PIPE ViPipe);
int ISPCFG_GetParam(VI_PIPE ViPipe, ISPCFG_PARAM_S *pIspParam);
int ISPCFG_SetParam(VI_PIPE ViPipe, ISPCFG_PARAM_S *pIspParam);
int ISPCFG_SetLDCI(VI_PIPE ViPipe);
int ISPCFG_GetFSWDR(VI_PIPE viPipe, ISPCFG_WDRFS_PARAM_S *pStaticWDRFs);
int ISPCFG_SetBypass(VI_PIPE ViPipe);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __LOAD_BIN_FILE_H__ */



