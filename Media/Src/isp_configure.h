#ifndef __ISP_CONFIGURE_H__
#define __ISP_CONFIGURE_H__

#include "hi_comm_isp.h"
#include "hi_type.h"

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
    HI_U8  u8AutostaticCompesation[16];
    HI_U16 u16AutoEVBias;
    HI_BOOL bAutoAEStrategMode;
    HI_U16 u16AutoHistRatioSlope;
    HI_U8  u8AutoMaxHistOffset;
    HI_U16 u16AutoBlackDelayFrame;
    HI_U16 u16AutoWhiteDelayFrame;  //原先已经存在的
    HI_BOOL enFSWDRMode;
} ISPCFG_STATIC_AE_S;

typedef struct
{
    HI_U8  u8ExpRatioType;
    HI_U32 u32ExpRatioMax;
    HI_U32 u32ExpRatioMin;
    HI_U16 u16Tolerance;
    HI_U16 u16Speed;
    HI_U16 u16RatioBias;
    HI_U32 au32ExpRatio[3];
} ISPCFG_STATIC_WDR_AE_S;

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
} ISPCFG_STATIC_DRC_S;

typedef struct
{
    HI_U8    CcmOpType;
    HI_BOOL  bISOActEn;
    HI_BOOL  bTempActEn;
    HI_U16   u16CCMTabNum;
    ISP_COLORMATRIX_PARAM_S astCCMTab[7];
}ISPCFG_STATIC_CCM_S;

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
} ISPCFG_STATIC_DEHAZE_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_U8   u8LDCIOpType;
    HI_U16  u16ManualBlcCtrl;
    HI_U16  u16AutoBlcCtrl[16];
} ISPCFG_STATIC_LDCI_S;

typedef struct
{
    HI_U8 au8AEWeight[AE_ZONE_ROW][AE_ZONE_COLUMN];
} ISPCFG_STATIC_STATISTICSCFG_S;

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
} ISPCFG_STATIC_AWB_S;

typedef struct
{
    HI_BOOL bEnable;
    COLOR_GAMUT_E enColorGamut;  //色域属性
    HI_U8 u8Hue;
    HI_U8 u8Luma;
    HI_U8 u8Contr;
    HI_U8 u8Satu;
} ISPCFG_STATIC_CSC_S;

typedef struct
{
    HI_BOOL bEnable;
} ISPCFG_STATIC_SHADING_S;

typedef struct
{
    HI_BOOL bOpType;
    HI_U8   u8ManualSat;
    HI_U8   au8AutoSat[ISP_AUTO_ISO_STRENGTH_NUM];
} ISPCFG_STATIC_SATURATION_S;

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
    HI_U8   AutoLumaWgt[16][32];
    HI_U16  au16TextureStr[16][32];
    HI_U16  au16EdgeStr[16][32];
} ISPCFG_STATIC_SHARPEN_S;

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
} ISPCFG_STATIC_DEMOSAIC_S;

typedef struct
{
    HI_S32 IES0, IES1, IES2, IES3;
    HI_S32 IEDZ;
} ISPCFG_VI_3DNR_IE_S;

typedef struct
{
    HI_S32 SPN6, SFR;
    HI_S32 SBN6, PBR6;
    HI_S32 SRT0, SRT1, JMODE, DeIdx;
    HI_S32 DeRate, SFR6[3];

    HI_S32 SFS1, SFT1, SBR1;
    HI_S32 SFS2, SFT2, SBR2;
    HI_S32 SFS4, SFT4, SBR4;

    HI_S32 STH1,  SFN1, NRyEn, SFN0;
    HI_S32 STH2,  SFN2, BWSF4, kMode;
    HI_S32 STH3,  SFN3, TriTh;
} ISPCFG_VI_3DNR_SF_S;

typedef struct
{
    ISPCFG_VI_3DNR_IE_S IEy;
    ISPCFG_VI_3DNR_SF_S SFy;
} ISPCFG_VI_3DNR_S;

typedef struct
{
    HI_S32 IES0, IES1, IES2, IES3;
    HI_S32 IEDZ;
} ISPCFG_VPSS_3DNR_IE_S;

typedef struct
{
    HI_S32 GMEMode;
} ISPCFG_VPSS_3DNR_GMC_S;

typedef struct
{
    HI_S32 SPN6, SFR;
    HI_S32 SBN6, PBR6;
    HI_S32 SRT0, SRT1, JMODE, DeIdx;
    HI_S32 DeRate, SFR6[3];

    HI_S32 SFS1, SFT1, SBR1;
    HI_S32 SFS2, SFT2, SBR2;
    HI_S32 SFS4, SFT4, SBR4;

    HI_S32 STH1,  SFN1, SFN0, NRyEn;
    HI_S32 STH2,  SFN2, BWSF4, kMode;
    HI_S32 STH3,  SFN3, TriTh;

    HI_S32 SBSk[32], SDSk[32];
} ISPCFG_VPSS_3DNR_SF_S;

typedef struct
{
    HI_S32 MADZ0, MAI00, MAI01, MAI02, biPath;
    HI_S32 MADZ1, MAI10, MAI11, MAI12;
    HI_S32 MABR0, MABR1;

    HI_S32 MATH0, MATE0, MATW;
    HI_S32 MATH1, MATE1;
    HI_S32 MASW;
    HI_S32 MABW0, MABW1;
} ISPCFG_VPSS_3DNR_MD_S;

typedef struct
{
    HI_S32 advMATH, RFDZ;
    HI_S32 RFUI, RFSLP, bRFU;
} ISPCFG_VPSS_3DNR_RF_S;

typedef struct
{
    HI_S32 TFS0, TDZ0, TDX0;
    HI_S32 TFS1, TDZ1, TDX1;
    HI_S32 SDZ0, STR0, DZMode0;
    HI_S32 SDZ1, STR1, DZMode1;

    HI_S32 TFR0[7], TSS0, TSI0;
    HI_S32 TFR1[7], TSS1, TSI1;

    HI_S32 RFI, tEdge;
    HI_S32 bRef;
} ISPCFG_VPSS_3DNR_TF_S;

typedef struct
{
    HI_S32 SFC, TFC;
    HI_S32 CTFS;
} ISPCFG_3DNR_VPSS_PNRC_S;

typedef struct
{
    ISPCFG_VPSS_3DNR_IE_S IEy;
    ISPCFG_VPSS_3DNR_SF_S SFy;
    ISPCFG_VPSS_3DNR_TF_S TFy;
    ISPCFG_VPSS_3DNR_MD_S MDy;
} ISPCFG_VPSS_3DNR_NRC_S;

typedef struct
{
    ISPCFG_VPSS_3DNR_IE_S  IEy[3];
    ISPCFG_VPSS_3DNR_GMC_S  GMC;
    ISPCFG_VPSS_3DNR_SF_S  SFy[3];
    ISPCFG_VPSS_3DNR_MD_S  MDy[2];
    ISPCFG_VPSS_3DNR_RF_S  RFs;
    ISPCFG_VPSS_3DNR_TF_S  TFy[2];
    ISPCFG_3DNR_VPSS_PNRC_S pNRc;
    ISPCFG_VPSS_3DNR_NRC_S  NRc;
} ISPCFG_VPSS_3DNR_S;

typedef struct
{
    HI_U32 u32ThreeDNRCount;
    HI_U16 u16VI_3DNRStartPoint;
    HI_U32 au32ThreeDNRIso[15];
    ISPCFG_VI_3DNR_S astThreeDNRVIValue[15];
    ISPCFG_VPSS_3DNR_S astThreeDNRVPSSValue[15];
} ISPCFG_DYNAMIC_THREEDNR_S;

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
    HI_U16 u16Table[25][41];   //25*41=1025
    int enCurveType;
} ISPCFG_DYNAMIC_GAMMA_S;

typedef struct
{
    HI_BOOL bEnable;
    HI_BOOL bNrLscEnable;
    HI_U8   u8BnrMaxGain;
    HI_U16  u16BnrLscCmpStrength;
    HI_U8   u8BnrOptype;
    HI_U8   au8ChromaStr[4][16];
    HI_U8   au8FineStr[16];
    HI_U16  au16CoringWgt[16];
    HI_U16  au16CoarseStr[4][16];
    HI_U8   au8WDRFrameStr[4];
}ISPCFG_STATIC_BNR_S;

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
    HI_U8   au8AutoMdThrLowGain[16];
    HI_U8   au8AutoMdThrHigGain[16];
    HI_U16  u16FusionFusionThr[4];
} ISPCFG_STATIC_WDRFS_S;

typedef struct
{
    ISPCFG_BYPASS_PARAM_S stModuleState;
    ISPCFG_BLACK_PARAM_S  stStaticBlackLevel;
    ISPCFG_STATIC_AE_S     stStaticAe;
    ISPCFG_STATIC_WDR_AE_S stStaticWdrExposure;
    ISPCFG_STATIC_DRC_S  stStaticDrc;
    ISPCFG_STATIC_CCM_S  stStaticCCMAttr;
    ISPCFG_STATIC_DEHAZE_S stStaticDehaze;
    ISPCFG_STATIC_LDCI_S stStaticLdci;
    ISPCFG_STATIC_STATISTICSCFG_S stStatistics;
    ISPCFG_STATIC_AWB_S stStaticAwb;
    ISPCFG_STATIC_CSC_S stStaticCsc;
    ISPCFG_STATIC_SHADING_S stStaticShading;
    ISPCFG_STATIC_SATURATION_S  stStaticSaturation;
    ISPCFG_STATIC_SHARPEN_S  stStaticSharpen;
    ISPCFG_STATIC_DEMOSAIC_S  stStaticDemosaic;
    ISPCFG_DYNAMIC_THREEDNR_S stDynamicThreeDNR;
    ISPCFG_DYNAMIC_GAMMA_S stDynamicGamma;
    ISPCFG_STATIC_WDRFS_S stStaticWDRFS;
    ISPCFG_STATIC_BNR_S   stStaticBNRS;
} ISPCFG_PARAM_S;

int ISPCFG_LoadFile(VI_PIPE viPipe, char *pFileName);
int ISPCFG_UnloadFile(VI_PIPE viPipe);
unsigned int ISPCFG_GetValue(const VI_PIPE viPipe, const char *pString);
int ISPCFG_SetValue(const VI_PIPE viPipe, const char *pKeyString,char *pValue);
int ISPCFG_GetString(const VI_PIPE viPipe, char **pString, const char *pKey);
int ISPCFG_SetLongFrameAE(VI_PIPE ViPipe);
int ISPCFG_SetStatistics(VI_PIPE ViPipe);
int ISPCFG_SetDemosaic(VI_PIPE ViPipe);
int ISPCFG_SetPipeParam(VI_PIPE ViPipe, VPSS_GRP VpssGrp);
int ISPCFG_SetNormalStatictics(VI_PIPE ViPipe);
int ISPCFG_GetParam(VI_PIPE ViPipe, ISPCFG_PARAM_S *pIspParam);
int ISPCFG_SetParam(VI_PIPE ViPipe, ISPCFG_PARAM_S *pIspParam);
int ISPCFG_SetLDCI(VI_PIPE ViPipe);
int ISPCFG_GetStaticFSWDR(VI_PIPE viPipe, ISPCFG_STATIC_WDRFS_S *pStaticWDRFs);
int ISPCFG_SetBypass(VI_PIPE ViPipe);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __LOAD_BIN_FILE_H__ */



