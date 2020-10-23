#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <iconv.h>
#include <math.h>
#include "hi_type.h"

#include "mpi_isp.h"
#include "mpi_sys.h"
#include "hi_defines.h"
#include "iniparser.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"
#include "hi_comm_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"

#include "isp_configure.h"
#include "media_com.h"

static dictionary *ISPCFG_Fd[VI_MAX_PIPE_NUM];
static unsigned long long ISPCFG_LineValue[MAX_NUM_IN_LINE];

unsigned int ISPCFG_IsoValue[VI_MAX_PIPE_NUM] = {100,100,100,100};

ISPCFG_PARAM_S stISPCfgPipeParam[VI_MAX_PIPE_NUM] = {0};

/* 加载配置文件 */
int ISPCFG_LoadFile(VI_PIPE viPipe, char *pFileName)
{
    /*打开 ini file */
    if ((viPipe >= VI_MAX_PIPE_NUM) || (pFileName == NULL))
    {
        prtMD("invalid input viPipe = %d or pFileName = %p\n", viPipe, pFileName);
        return -1;
    }

    ISPCFG_Fd[viPipe] = iniparser_load(pFileName);
    if (ISPCFG_Fd[viPipe] == NULL)
    {
        prtMD("stone:iniparser_load error!\n");
        return -1;
    }

    return 0;
}

/* 卸载配置文件 */
int ISPCFG_UnloadFile(VI_PIPE viPipe)
{
    if (viPipe > VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input viPipe = %d\n", viPipe);
        return -1;
    }

    iniparser_freedict(ISPCFG_Fd[viPipe]);

    return 0;
}

/**函数：
***说明：获取pString对应key字符串项的值
***/
int ISPCFG_GetValue(const VI_PIPE viPipe, const char *pString)
{
    int keyvalue = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pString == NULL))
    {
        prtMD("invalid input viPipe = %d, pString = %p\n", viPipe, pString);
        return keyvalue;
    }

    keyvalue = atoi(iniparser_getstring(ISPCFG_Fd[viPipe], pString, "null"));

    return keyvalue;
}

/**函数：
***说明：写入参数到对应的ini文件中
***/
int ISPCFG_SetValue(const VI_PIPE viPipe, const char *pKeyString, char *pValue)
{
    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pKeyString) || (NULL == pValue))
    {
        prtMD("invalid input viPipe = %d or pKeyString = %p or pValue = %p\n", viPipe, pKeyString, pValue);
        return -1;
    }

    iniparser_set(ISPCFG_Fd[viPipe], pKeyString, pValue);

    return 0;
}

/**函数：
***说明：获取ini文件中的字符串信息
***/
int ISPCFG_GetString(const VI_PIPE viPipe, char **pString, const char *pKey)
{
    if ((viPipe >= VI_MAX_PIPE_NUM) || (pString == NULL) || (pKey == NULL))
    {
        prtMD("invalid input viPipe = %d or pString = %p or pKey = %p\n", viPipe, pString, pKey);
        return -1;
    }

    *pString = (char *)iniparser_getstring(ISPCFG_Fd[viPipe], pKey, "null");
    if (*pString == NULL)
    {
        prtMD("get string is error!\n");
        return -1;
    }

    return 0;
}


/**函数：
***说明：解析字符串,返回字符串中有效数据的个数,数据存储到ISPCFG_LineValue数组中
***/
static int ISPCFG_GetNumInLine(char *pInLine)
{
    char *pszVRBegin = NULL;
    char *pszVREnd = NULL;
    unsigned int u32PartCount = 0;
    unsigned int u32WholeCount = 0;
    unsigned long long u64Hex = 0;
    char szPart[20] = {0};
    int s32Length = 0;
    int i = 0;

    HI_BOOL bHex = HI_FALSE;

    if (pInLine == NULL)
    {
        prtMD("invalid input pInLine = %p\n", pInLine);
        return -1;
    }

    pszVRBegin = pInLine;
    pszVREnd = pszVRBegin;
    s32Length = strlen(pInLine);

    memset(ISPCFG_LineValue, 0 , sizeof(long long) * 5000);
    while ((pszVREnd != NULL))
    {
        if ((u32WholeCount > s32Length) || (u32WholeCount == s32Length))
        {
            break;
        }

        while ((*pszVREnd != '|') && (*pszVREnd != '\0') && (*pszVREnd != ','))
        {
            if (*pszVREnd == 'x')
            {
                bHex = HI_TRUE;
            }
            pszVREnd++;
            u32PartCount++;
            u32WholeCount++;
        }

        memcpy(szPart, pszVRBegin, u32PartCount);

        if (bHex == HI_TRUE)
        {
            char *pszstr;
            u64Hex = (unsigned long long)strtoll(szPart + 2, &pszstr, 16);
            ISPCFG_LineValue[i] = u64Hex;
        }
        else
        {
            ISPCFG_LineValue[i] = atoll(szPart);
        }

        memset(szPart, 0, 20);
        u32PartCount = 0;
        pszVREnd++;
        pszVRBegin = pszVREnd;
        u32WholeCount++;
        i++;
        if (i >= MAX_NUM_IN_LINE)
        {
            prtMD("too many values in one line! i = %d\n", i);
            return i;
        }
    }

    return i;
}

HI_U32 ISPCFG_GetThreshValue(HI_U32 u32Value, HI_U32 u32Count, HI_U32* pThresh)
{
    HI_U32 u32Level = 0;

    for (u32Level = u32Count; u32Level > 0; u32Level--)
    {
        if (u32Value > pThresh[u32Level - 1])
        {
            break;
        }
    }

    if (u32Level > 0)
    {
        u32Level = u32Level - 1;
    }

    return u32Level;
}

static int ISPCFG_GetBypass(VI_PIPE viPipe, ISPCFG_BYPASS_PARAM_S *pBypassParam)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pBypassParam == NULL))
    {
        prtMD("invalid input viPipe = %d, pBypassParam = %p\n", viPipe, pBypassParam);
        return -1;
    }

    /* bitBypassISPDGain */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassISPDGain");
    pBypassParam->bitBypassISPDGain = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassISPDGain = %d\n", pBypassParam->bitBypassISPDGain);

    /* bitBypassAntiFC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassAntiFC");
    pBypassParam->bitBypassAntiFC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAntiFC = %d\n", pBypassParam->bitBypassAntiFC);

    /* bitBypassCrosstalkR */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassCrosstalkR");
    pBypassParam->bitBypassCrosstalkR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCrosstalkR = %d\n", pBypassParam->bitBypassCrosstalkR);

    /* bitBypassDPC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassDPC");
    pBypassParam->bitBypassDPC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDPC = %d\n", pBypassParam->bitBypassDPC);

    /* bitBypassNR */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassNR");
    pBypassParam->bitBypassNR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassNR = %d\n", pBypassParam->bitBypassNR);

    /* bitBypassDehaze */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassDehaze");
    pBypassParam->bitBypassDehaze = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDehaze = %d\n", pBypassParam->bitBypassDehaze);

    /* bitBypassWBGain */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassWBGain");
    pBypassParam->bitBypassWBGain = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassWBGain = %d\n", pBypassParam->bitBypassWBGain);

    /* bitBypassMeshShading */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassMeshShading");
    pBypassParam->bitBypassMeshShading = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassMeshShading = %d\n", pBypassParam->bitBypassMeshShading);

    /* bitBypassDRC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassDRC");
    pBypassParam->bitBypassDRC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDRC = %d\n", pBypassParam->bitBypassDRC);

    /* bitBypassDemosaic */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassDemosaic");
    pBypassParam->bitBypassDemosaic = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDemosaic = %d\n", pBypassParam->bitBypassDemosaic);

    /* bitBypassColorMatrix */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassColorMatrix");
    pBypassParam->bitBypassColorMatrix = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassColorMatrix = %d\n", pBypassParam->bitBypassColorMatrix);

    /* bitBypassGamma */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassGamma");
    pBypassParam->bitBypassGamma = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassGamma = %d\n", pBypassParam->bitBypassGamma);

    /* bitBypassFSWDR */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassFSWDR");
    pBypassParam->bitBypassFSWDR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassFSWDR = %d\n", pBypassParam->bitBypassFSWDR);

    /* bitBypassCA */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassCA");
    pBypassParam->bitBypassCA = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCA = %d\n", pBypassParam->bitBypassCA);

    /* bitBypassCsConv */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassCsConv");
    pBypassParam->bitBypassCsConv = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCsConv = %d\n", pBypassParam->bitBypassCsConv);

    /* bitBypassSharpen */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassSharpen");
    pBypassParam->bitBypassSharpen = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassSharpen = %d\n", pBypassParam->bitBypassSharpen);

    /* bitBypassLCAC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassLCAC");
    pBypassParam->bitBypassLCAC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassLCAC = %d\n", pBypassParam->bitBypassLCAC);

    /* bitBypassGCAC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassGCAC");
    pBypassParam->bitBypassGCAC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassGCAC = %d\n", pBypassParam->bitBypassGCAC);

    /* bit2ChnSelect */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bit2ChnSelect");
    pBypassParam->bit2ChnSelect = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bit2ChnSelect = %d\n", pBypassParam->bit2ChnSelect);

    /* bitBypassLdci */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassLdci");
    pBypassParam->bitBypassLdci = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassLdci = %d\n", pBypassParam->bitBypassLdci);

    /* bitBypassPreGamma */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassPreGamma");
    pBypassParam->bitBypassPreGamma = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassPreGamma = %d\n", pBypassParam->bitBypassPreGamma);

    /* bitBypassAEStatFE */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassAEStatFE");
    pBypassParam->bitBypassAEStatFE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAEStatFE = %d\n", pBypassParam->bitBypassAEStatFE);

    /* bitBypassAEStatBE */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassAEStatBE");
    pBypassParam->bitBypassAEStatBE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAEStatBE = %d\n", pBypassParam->bitBypassAEStatBE);

    /* bitBypassMGSta */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassMGSta");
    pBypassParam->bitBypassMGSta = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassMGSta = %d\n", pBypassParam->bitBypassMGSta);

    /* bitBypassDE */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassDE");
    pBypassParam->bitBypassDE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDE = %d\n", pBypassParam->bitBypassDE);

    /* bitBypassAFStatBE */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassAFStatBE");
    pBypassParam->bitBypassAFStatBE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAFStatBE = %d\n", pBypassParam->bitBypassAFStatBE);

    /* bitBypassAWBStat */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassAWBStat");
    pBypassParam->bitBypassAWBStat = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAWBStat = %d\n", pBypassParam->bitBypassAWBStat);

    /* bitBypassCLUT */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassCLUT");
    pBypassParam->bitBypassCLUT = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCLUT = %d\n", pBypassParam->bitBypassCLUT);

    /* bitBypassHLC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassHLC");
    pBypassParam->bitBypassHLC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassHLC = %d\n", pBypassParam->bitBypassHLC);

    /* bitBypassEdgeMark */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_MODULE_CTRL_U:bitBypassEdgeMark");
    pBypassParam->bitBypassEdgeMark = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassEdgeMark = %d\n", pBypassParam->bitBypassEdgeMark);

    return 0;
}

//黑电平属性
static int ISPCFG_GetBlackValue(VI_PIPE viPipe, ISPCFG_BLACK_PARAM_S *pBlackParam)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    int ret = 0;
    int valueNumber = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pBlackParam))
    {
        prtMD("invalid input viPipe = %d or pBlackParam = %p\n", viPipe, pBlackParam);
        return -1;
    }

    /* enOptype */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_BLACK_LEVEL_S:enOptype");
    pBlackParam->enOpType = (HI_S32)u32Value;

    /* AutoStaticWb 包含了4个参数 */
    ret = ISPCFG_GetString(viPipe, &pString,"hiISP_BLACK_LEVEL_S:au16BlackLevel");
    if (0 != ret)
    {
        prtMD("ISPCFG_GetString error! viPipe = %d, ret = %#x\n", viPipe, ret);
    }

    valueNumber = ISPCFG_GetNumInLine(pString);
    prtMD("type:%d, valueNumber = %d\n", pBlackParam->enOpType, valueNumber);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pBlackParam->au16BlackLevel[u32IdxM] = (HI_U16)u32Value;
    }

    return 0;
}

//TODO 函数名待优化
static int ISPCFG_GetAE(VI_PIPE viPipe, ISPCFG_AE_PARAM_S *pStaticAe)
{
    unsigned int u32Value = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticAe == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticAe = %p\n", viPipe, pStaticAe);
        return -1;
    }

    /* AERouteExValid */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_EXPOSURE_ATTR_S:bAERouteExValid");
    pStaticAe->bAERouteExValid = (HI_BOOL)u32Value;

    /* ExposureOpType */  //2019-08-29 add
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_EXPOSURE_ATTR_S:enOpType");
    pStaticAe->ExposureOpType = (HI_BOOL)u32Value;

    /* AERunInterval */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_EXPOSURE_ATTR_S:u8AERunInterval");
    pStaticAe->u8AERunInterval = (HI_U8)u32Value;
    //prtMD("1-------u32Value = %d\n",pStaticAe->u8AERunInterval);

    /* AutoExpTimeMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stExpTimeRange.u32Max");
    pStaticAe->u32AutoExpTimeMax = u32Value;

    /* AutoExpTimeMix */  //2019-08-29 add
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stExpTimeRange.u32Min");
    pStaticAe->u32AutoExpTimeMin = u32Value;

    prtMD("[AE] valid:%d, type:%d, RunInterval:%d, MaxExpTime:%d, MinExpTime:%d\n",
        pStaticAe->bAERouteExValid,
        pStaticAe->ExposureOpType,
        pStaticAe->u8AERunInterval,
        pStaticAe->u32AutoExpTimeMax,
        pStaticAe->u32AutoExpTimeMin);

    /* AutoAGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stAGainRange.u32Max");
    pStaticAe->u32AutoAGainMax = u32Value;

    /* AutoAGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stAGainRange.u32Min");
    pStaticAe->u32AutoAGainMin = u32Value;

    /* AutoDGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stDGainRange.u32Max");
    pStaticAe->u32AutoDGainMax = u32Value;

    /* AutoDGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stDGainRange.u32Min");
    pStaticAe->u32AutoDGainMin = u32Value;

    /* AutoISPDGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stISPDGainRange.u32Max");
    pStaticAe->u32AutoISPDGainMax = u32Value;

    /* AutoISPDGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stISPDGainRange.u32Min");
    pStaticAe->u32AutoISPDGainMin = u32Value;

    /* AutoSysGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stSysGainRange.u32Max");
    pStaticAe->u32AutoSysGainMax = u32Value;

    /* AutoSysGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stSysGainRange.u32Min");
    pStaticAe->u32AutoSysGainMin = u32Value;

    /* AutoGainThreshold */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u32GainThreshold");
    pStaticAe->u32AutoGainThreshold = u32Value;

    /* AutoSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u8Speed");
    pStaticAe->u8AutoSpeed = (HI_U8)u32Value;

    /* AutoBlackSpeedBias */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u16BlackSpeedBias");
    pStaticAe->u16AutoBlackSpeedBias = (HI_U16)u32Value;

    /* AutoTolerance */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u8Tolerance");
    pStaticAe->u8AutoTolerance = (HI_U8)u32Value;

    /* AutostaticCompesation */
    u32Value = ISPCFG_GetString(viPipe, &pString, "hiISP_AE_ATTR_S:u8Compensation");
    pStaticAe->u8AutostaticCompesation = (HI_U8)u32Value;

    /* AutoEVBias */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u16EVBias");
    pStaticAe->u16AutoEVBias = (HI_U16)u32Value;

    /* AutoAEStrategMode */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:enAEStrategyMode");
    pStaticAe->bAutoAEStrategMode = (HI_BOOL)u32Value;

    /* AutoHistRatioSlope */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u16HistRatioSlope");
    pStaticAe->u16AutoHistRatioSlope = (HI_U16)u32Value;

    /* AutoMaxHistOffset */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:u8MaxHistOffset");
    pStaticAe->u8AutoMaxHistOffset = (HI_U8)u32Value;

    /* AutoBlackDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stAEDelayAttr.u16BlackDelayFrame");
    pStaticAe->u16AutoBlackDelayFrame = (HI_U16)u32Value;

    /* AutoWhiteDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AE_ATTR_S:stAEDelayAttr.u16WhiteDelayFrame");
    pStaticAe->u16AutoWhiteDelayFrame = (HI_U16)u32Value;

    return 0;
}

static int ISPCFG_GetAWB(VI_PIPE viPipe, ISPCFG_AWB_PARAM_S *pStaticAwb)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticAwb == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticAwb = %p\n", viPipe, pStaticAwb);
        return -1;
    }

    /* AutoWhiteDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u16RefColorTemp");
    pStaticAwb->u16AutoRefColorTemp = (HI_U16)u32Value;

    /* AutoStaticWb 包含了4个参数 */
    ISPCFG_GetString(viPipe, &pString,"hiISP_AWB_ATTR_S:au16StaticWB");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticAwb->au16AutoStaticWB[u32IdxM] = (HI_U16)u32Value;
    }

    /* AutoCurvePara */
    ISPCFG_GetString(viPipe, &pString,"hiISP_AWB_ATTR_S:as32CurvePara");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 6; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticAwb->as32AutoCurvePara[u32IdxM] = (HI_S32)u32Value;
    }

    /* AutoRGStrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u8RGStrength");
    pStaticAwb->u8AutoRGStrength = (HI_U8)u32Value;

    /* AutoBGStrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u8BGStrength");
    pStaticAwb->u8AutoBGStrength = (HI_U8)u32Value;

    /* AutoAWBSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u16Speed");
    pStaticAwb->u16AutoSpeed = (HI_U16)u32Value;

    /* AutoZoneSel */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u16ZoneSel");
    pStaticAwb->u16AutoZoneSel = (HI_U16)u32Value;

    /* AutoHightColorTemp */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u16HighColorTemp");
    pStaticAwb->u16AutoHighColorTemp = (HI_U16)u32Value;


    /* AutoLowColorTemp */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_AWB_ATTR_S:u16LowColorTemp");
    pStaticAwb->u16AutoLowColorTemp = (HI_U16)u32Value;
    return 0;
}

static int ISPCFG_GetWdrExposure(VI_PIPE viPipe, ISPCFG_WDR_AE_PARAM_S *pStaticWdrExposure)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticWdrExposure == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticWdrExposure = %p\n", viPipe, pStaticWdrExposure);
        return -1;
    }

    /* ExpRatioType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:enExpRatioType");
    pStaticWdrExposure->u8ExpRatioType = (HI_U8)u32Value;

    /* ExpRatioMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:u32ExpRatioMax");
    pStaticWdrExposure->u32ExpRatioMax = (HI_U32)u32Value;

    /* ExpRatioMin */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:u32ExpRatioMin");
    pStaticWdrExposure->u32ExpRatioMin = (HI_U32)u32Value;

    /* Tolerance */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:u16Tolerance");
    pStaticWdrExposure->u16Tolerance = (HI_U16)u32Value;

    /* WDRSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:u16Speed");
    pStaticWdrExposure->u16Speed = (HI_U16)u32Value;

    /* RatioBias */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_EXPOSURE_ATTR_S:u16RatioBias");
    pStaticWdrExposure->u16RatioBias = (HI_U16)u32Value;

    /* ExpRatio */
    ISPCFG_GetString(viPipe, &pString, "hiISP_WDR_EXPOSURE_ATTR_S:au32ExpRatio");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 3; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWdrExposure->au32ExpRatio[u32IdxM] = (HI_U32)u32Value;
    }

    return 0;
}

static int ISPCFG_GetSaturation(VI_PIPE viPipe, ISPCFG_SATURATION_PARAM_S *pStaticSaturation)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    int s32Ret = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticSaturation == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticSaturation = %p\n", viPipe, pStaticSaturation);
        return -1;
    }

    /* OpType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SATURATION_ATTR_S:enOpType");
    pStaticSaturation->bOpType = (HI_BOOL)u32Value;

    /* ManualSat */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SATURATION_ATTR_S:stManual.u8Saturation");
    pStaticSaturation->u8ManualSat = (HI_U8)u32Value;

    /* AutoSat */
    s32Ret = ISPCFG_GetString(viPipe, &pString, "hiISP_SATURATION_ATTR_S:stAuto.au8Sat");
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetString error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = ISPCFG_GetNumInLine(pString);

    prtMD("Manual.Saturation[%d]:", s32Ret);

    for (u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSaturation->au8AutoSat[u32IdxM] = (HI_U8)u32Value;
        printf(" %d", u32Value);
    }
    printf("\n");

    return 0;
}

static int ISPCFG_GetDRC(VI_PIPE viPipe, ISPCFG_DRC_PARAM_S *pStaticDrc)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticDrc == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticDrc = %p\n", viPipe, pStaticDrc);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:bEnable");
    pStaticDrc->bEnable = (HI_BOOL)u32Value;

    /* CurveSelect */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:enCurveSelect");
    pStaticDrc->u8CurveSelect = (HI_U8)u32Value;

    /* PDStrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8PDStrength");
    pStaticDrc->u8PDStrength = (HI_U8)u32Value;

    /* LocalMixingBrightMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingBrightMax");
    pStaticDrc->u8LocalMixingBrightMax = (HI_U8)u32Value;

    /* LocalMixingBrightMin */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingBrightMin");
    pStaticDrc->u8LocalMixingBrightMin = (HI_U8)u32Value;

    /* LocalMixingBrightThr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingBrightThr");
    pStaticDrc->u8LocalMixingBrightThr = (HI_U8)u32Value;

    /* LocalMixingBrightSlo */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:s8LocalMixingBrightSlo");
    pStaticDrc->s8LocalMixingBrightSlo = (HI_S8)u32Value;

    /* LocalMixingDarkMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingDarkMax");
    pStaticDrc->u8LocalMixingDarkMax = (HI_U8)u32Value;

    /* LocalMixingDarkMin */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingDarkMin");
    pStaticDrc->u8LocalMixingDarkMin = (HI_U8)u32Value;

    /* LocalMixingDarkThr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8LocalMixingDarkThr");
    pStaticDrc->u8LocalMixingDarkThr = (HI_U8)u32Value;

    /* LocalMixingDarkSlo */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:s8LocalMixingDarkSlo");
    pStaticDrc->s8LocalMixingDarkSlo = (HI_S8)u32Value;

    /* BrightGainLmt */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8BrightGainLmt");
    pStaticDrc->u8BrightGainLmt = (HI_U8)u32Value;

    /* BrightGainLmtStep */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8BrightGainLmtStep");
    pStaticDrc->u8BrightGainLmtStep = (HI_BOOL)u32Value;

    /* DarkGainLmtY */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8DarkGainLmtY");
    pStaticDrc->u8DarkGainLmtY = (HI_U8)u32Value;

    /* DarkGainLmtC */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8DarkGainLmtC");
    pStaticDrc->u8DarkGainLmtC = (HI_U8)u32Value;

    /* ContrastControl */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8ContrastControl");
    pStaticDrc->u8ContrastControl = (HI_U8)u32Value;

    /* DetailAdjustFactor */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:s8DetailAdjustFactor");
    pStaticDrc->s8DetailAdjustFactor = (HI_S8)u32Value;

    /* SpatialFltCoef */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8SpatialFltCoef");
    pStaticDrc->u8SpatialFltCoef = (HI_U8)u32Value;

    /* RangeFltCoef */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8RangeFltCoef");
    pStaticDrc->u8RangeFltCoef = (HI_U8)u32Value;

    /* RangeAdaMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8RangeAdaMax");
    pStaticDrc->u8RangeAdaMax = (HI_U8)u32Value;

    /* GradRevMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8GradRevMax");
    pStaticDrc->u8GradRevMax = (HI_U8)u32Value;

    /* GradRevThr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:u8GradRevThr");
    pStaticDrc->u8GradRevThr = (HI_U8)u32Value;

    /* DRCOpType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:enOpType");
    pStaticDrc->u8DRCOpType = (HI_U8)u32Value;

    /* ManualStrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stManual.u16Strength");
    pStaticDrc->u16ManualStrength = (HI_U16)u32Value;

    /* AutoStrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAuto.u16Strength");
    pStaticDrc->u16AutoStrength = (HI_U16)u32Value;

    /* AutoStrengthMax */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAuto.u16StrengthMax");
    pStaticDrc->u16AutoStrengthMax = (HI_U16)u32Value;

    /* AutoStrengthMin */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAuto.u16StrengthMin");
    pStaticDrc->u16AutoStrengthMin = (HI_U16)u32Value;

    /* Asy_u8Asymmetry */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAsymmetryCurve.u8Asymmetry");
    pStaticDrc->u8Asymmetry = (HI_U16)u32Value;

    /* Asy_u8SecondPole */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAsymmetryCurve.u8SecondPole");
    pStaticDrc->u8SecondPole = (HI_U16)u32Value;

    /* Asy_u8Stretch */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAsymmetryCurve.u8Stretch");
    pStaticDrc->u8Stretch = (HI_U16)u32Value;

    /* Asy_u8Compress */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DRC_ATTR_S:stAsymmetryCurve.u8Compress");
    pStaticDrc->u8Compress = (HI_U16)u32Value;

#if 0
    /* DRCToneMappingAsymmetry */
    for(i = 0;i < 10;i++)
    {
        snprintf(filename,128,"static_drc:DRCToneMappingAsymmetry%d",i);
        ISPCFG_GetString(viPipe, &pString, filename);
        ISPCFG_GetNumInLine(pString);

        for(j = 0; j < 20; j++)
        {
            u32Value = ISPCFG_LineValue[j];
            pStaticDrc->au16ToneMappingValue[i][j] = (HI_U16)u32Value;
        }
    }

    /* DRCToneMappingValue */
    ISPCFG_GetString(viPipe, &pString, "static_drc:DRCToneMappingValue");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 200; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDrc->au16ToneMappingValue[u32IdxM] = (HI_U16)u32Value;
    }

    /* DRCColorCorrectionLut */
    ISPCFG_GetString(viPipe, &pString, "static_drc:DRCColorCorrectionLut");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 33; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDrc->au16ColorCorrectionLut[u32IdxM] = (HI_U16)u32Value;
    }

    if(pString != NULL)
        free(pString);

    pString = NULL;
#endif

    return 0;
}

static int ISPCFG_GetCCM(VI_PIPE viPipe, ISPCFG_CCM_PARAM_S *pStaticCCM)
{
    int i = 0;
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;
    char astloadpara[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticCCM == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticCCm = %p\n", viPipe, pStaticCCM);
        return -1;
    }

    /* CcmopType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_COLORMATRIX_ATTR_S:enOpType");
    pStaticCCM->CcmOpType = (HI_U8)u32Value;

    /* AutoISOActEn */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_COLORMATRIX_ATTR_S:stAuto.bISOActEn");
    pStaticCCM->bISOActEn = (HI_BOOL)u32Value;

    /* AutoTempActEn */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_COLORMATRIX_ATTR_S:stAuto.bTempActEn");
    pStaticCCM->bTempActEn = (HI_BOOL)u32Value;

    /* AutoCCMTabNum */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_COLORMATRIX_ATTR_S:stAuto.u16CCMTabNum");
    pStaticCCM->u16CCMTabNum = (HI_U16)u32Value;

    //加载不同色温下的色温值
    for(i = 0;i < pStaticCCM->u16CCMTabNum;i++)
    {
        /* AutoCCMTabNum */
        snprintf(astloadpara, 128, "hiISP_COLORMATRIX_ATTR_S:stAuto.astCCMTab[%d].u16ColorTemp", i);
        u32Value = ISPCFG_GetValue(viPipe, astloadpara);
        pStaticCCM->astCCMTab[i].u16ColorTemp = (HI_U16)u32Value;

        /*AutoBlcCtrl*/
        snprintf(astloadpara, 128, "hiISP_COLORMATRIX_ATTR_S:stAuto.astCCMTab[%d].au16CCM", i);
        ISPCFG_GetString(viPipe, &pString, astloadpara);
        ISPCFG_GetNumInLine(pString);

        for(u32IdxM = 0; u32IdxM < 9; u32IdxM++)
        {
            u32Value = ISPCFG_LineValue[u32IdxM];
            pStaticCCM->astCCMTab[i].au16CCM[u32IdxM] = ISPCFG_LineValue[u32IdxM];
        }
    }

    return 0;
}

static int ISPCFG_GetDehaze(VI_PIPE viPipe, ISPCFG_DEHAZE_PARAM_S *pStaticDeHaze)
{
    unsigned int u32Value;
    unsigned int u32IdxM = 0;
    char *pString = NULL;
    char astloadpara[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticDeHaze == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticDeHaze = %p\n", viPipe, pStaticDeHaze);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:bEnable");
    pStaticDeHaze->bEnable = (HI_BOOL)u32Value;

    /* bDehazeUserLutEnable */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:bUserLutEnable");
    pStaticDeHaze->bUserLutEnable = (HI_BOOL)u32Value;

    /* DehazeOpType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:enOpType");
    pStaticDeHaze->u8DehazeOpType = (HI_U8)u32Value;

    /* Manualstrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:stManual.u8strength");
    pStaticDeHaze->u8ManualStrength = (HI_U8)u32Value;

    /* Autostrength */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:stAuto.u8strength");
    pStaticDeHaze->u8Autostrength = (HI_U8)u32Value;

    /* TmprfltIncrCoef */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:u16TmprfltIncrCoef");
    pStaticDeHaze->u16prfltIncrCoef = (HI_U16)u32Value;

    /* TmprfltDecrCoef */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEHAZE_ATTR_S:u16TmprfltDecrCoef");
    pStaticDeHaze->u16prfltDecrCoef = (HI_U16)u32Value;

    /* DehazeLut */
    snprintf(astloadpara, 128, "hiISP_DEHAZE_ATTR_S:au8DehazeLut");
    ISPCFG_GetString(viPipe, &pString, astloadpara);
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 256; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDeHaze->au8DehazeLut[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    return 0;
}

//曝光权重设置
static int ISPCFG_GetStatistics(VI_PIPE viPipe, ISPCFG_STATISTICS_PARAM_S *pStaticStatistics)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int idx = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticStatistics == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticStatistics = %p\n", viPipe, pStaticStatistics);
        return -1;
    }

    /*AEWeight*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_AE_STATISTICS_CFG_S:au8Weight");
    ISPCFG_GetNumInLine(pString);

    /* 设置曝光权重占比,默认场景下中间区域权要大,可根据实际业务场景需要来动态设置某一个区域的权重 */

    prtMD("au8Weight:\n");
    for (i = 0; i < AE_ZONE_ROW; i++)
    {
        for (j = 0; j < AE_ZONE_COLUMN; j++)
        {
            pStaticStatistics->au8AEWeight[i][j] = (HI_U8)ISPCFG_LineValue[idx++];
            printf("%d ", pStaticStatistics->au8AEWeight[i][j]);
        }
        printf("\n");
    }

    return 0;
}

static int ISPCFG_GetCSC(VI_PIPE viPipe, ISPCFG_CSC_PARAM_S *pStaticCSC)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticCSC))
    {
        prtMD("invalid input viPipe = %d or pStaticCSC = %p\n", viPipe, pStaticCSC);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:bEnable");
    pStaticCSC->bEnable = (HI_BOOL)u32Value;

    /*Hue 色调 */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:u8Hue");
    pStaticCSC->u8Hue = (HI_U8)u32Value;

    /*Luma 亮度 */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:u8Luma");
    pStaticCSC->u8Luma = (HI_U8)u32Value;

    /*Contrast 对比度 */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:u8Contr");
    pStaticCSC->u8Contr = (HI_U8)u32Value;

    /*Saturation 饱和度 */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:u8Satu");
    pStaticCSC->u8Satu = (HI_U8)u32Value;

    /*ColorGamut 色域属性 */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_CSC_ATTR_S:enColorGamut");
    pStaticCSC->enColorGamut = (HI_U8)u32Value;

    return 0;
}

static int ISPCFG_GetLDCI(VI_PIPE viPipe, ISPCFG_LDCI_PARAM_S *pStaticLdci)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticLdci))
    {
        prtMD("invalid input viPipe = %d or pStaticLdci = %p\n", viPipe, pStaticLdci);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_LDCI_ATTR_S:bEnable");
    pStaticLdci->bEnable = (HI_BOOL)u32Value;

    /*LDCIOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_LDCI_ATTR_S:enOpType");
    pStaticLdci->u8LDCIOpType = (HI_U8)u32Value;

    /*ManualBlcCtrl*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_LDCI_ATTR_S:stManual.u16BlcCtrl");
    pStaticLdci->u16ManualBlcCtrl = (HI_U8)u32Value;

    /*AutoBlcCtrl*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_LDCI_ATTR_S:stAuto.au16BlcCtrl");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticLdci->u16AutoBlcCtrl[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    return 0;
}

//13 -shading
static int ISPCFG_GetShading(VI_PIPE viPipe, ISPCFG_SHADING_PARAM_S *pStaticShading)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticShading))
    {
        prtMD("invalid input viPipe = %d, pStaticShading = %p\n", viPipe, pStaticShading);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHADING_ATTR_S:bEnable");
    pStaticShading->bEnable = (HI_BOOL)u32Value;

    return 0;
}

static int ISPCFG_GetSharpen(VI_PIPE viPipe, ISPCFG_SHARPEN_PARAM_S *pStaticSharpen)
{
    int u32Value = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int idx = 0;
    char * pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticSharpen))
    {
        prtMD("invalid input viPipe = %d, pStaticSharpen = %p\n", viPipe, pStaticSharpen);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:bEnable");
    pStaticSharpen->bEnable = (HI_BOOL)u32Value;

    /* enOpType */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:enOpType");
    pStaticSharpen->enOpType = (int)u32Value;

    //////////////////////////////////////////////////////Manual
    /* ManualTextureFreq */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u16TextureFreq");
    pStaticSharpen->ManualTextureFreq = (int)u32Value;

    /* ManualEdgeFreq */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u16EdgeFreq");
    pStaticSharpen->ManualEdgeFreq = (int)u32Value;

    /* ManualOvershoot */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8OverShoot");
    pStaticSharpen->ManualOvershoot = (int)u32Value;

    /* ManualUnderShoot */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8UnderShoot");
    pStaticSharpen->ManualUnderShoot = (int)u32Value;

    /* ManualSupStr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8ShootSupStr");
    pStaticSharpen->ManualSupStr = (int)u32Value;

    /* ManualSupAdj */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8ShootSupAdj");
    pStaticSharpen->ManualSupAdj = (int)u32Value;

    /* ManualDetailCtrl */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8DetailCtrl");
    pStaticSharpen->ManualDetailCtrl = (int)u32Value;

    /* ManualDetailCtrlThr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8DetailCtrlThr");
    pStaticSharpen->ManualDetailCtrlThr = (int)u32Value;

    /* ManualEdgeFiltStr */
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_SHARPEN_ATTR_S:stManual.u8EdgeFiltStr");
    pStaticSharpen->ManualEdgeFiltStr = (int)u32Value;

    /* AutoLumaWgt */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8LumaWgt");
    ISPCFG_GetNumInLine(pString);

    idx = 0;
    for(i = 0; i < ISP_SHARPEN_LUMA_NUM; i++)
    {
        for(j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
        {
            pStaticSharpen->AutoLumaWgt[i][j] = (HI_U16)ISPCFG_LineValue[idx++];
        }
    }

    /* AutoTextureFreq */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au16TextureFreq");
    ISPCFG_GetNumInLine(pString);
    for(i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoTextureFreq[i] = ISPCFG_LineValue[i];
    }

    /* AutoEdgeFreq */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au16EdgeFreq");
    ISPCFG_GetNumInLine(pString);
    for(i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoEdgeFreq[i] = ISPCFG_LineValue[i];
    }

    /* AutoOvershoot */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8OverShoot");
    ISPCFG_GetNumInLine(pString);
    for(i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->au8OverShoot[i] = ISPCFG_LineValue[i];
    }

    /* AutoUnderShoot */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8UnderShoot");
    ISPCFG_GetNumInLine(pString);
    for(i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->au8UnderShoot[i] = ISPCFG_LineValue[i];
    }

    /* AutoSupStr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8ShootSupStr");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoSupStr[i] = ISPCFG_LineValue[i];
    }

    /* AutoSupAdj */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8ShootSupAdj");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoSupAdj[i] = ISPCFG_LineValue[i];
    }

    /* AutoDetailCtrl */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8DetailCtrl");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoDetailCtrl[i] = ISPCFG_LineValue[i];
    }

    /* AutoDetailCtrlThr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8DetailCtrlThr");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < 16; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoDetailCtrlThr[i] = ISPCFG_LineValue[i];
    }

    /* AutoEdgeFiltStr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8EdgeFiltStr");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoEdgeFiltStr[i] = ISPCFG_LineValue[i];
    }

    /* AutoEdgeFiltMaxCap */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8EdgeFiltMaxCap");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoEdgeFiltMaxCap[i] = ISPCFG_LineValue[i];
    }

    /* AutoRGain */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8RGain");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoRGain[i] = ISPCFG_LineValue[i];
    }

    /* AutoGGain */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8GGain");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i ++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoGGain[i] = ISPCFG_LineValue[i];
    }

    /* AutoBGain */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8BGain");
    ISPCFG_GetNumInLine(pString);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoBGain[i] = ISPCFG_LineValue[i];
    }

    /* AutoSkinGain */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au8SkinGain");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoSkinGain[i] = ISPCFG_LineValue[i];
    }

    /* AutoMaxSharpGain */
    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au16MaxSharpGain");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        u32Value = ISPCFG_LineValue[i];
        pStaticSharpen->AutoMaxSharpGain[i] = ISPCFG_LineValue[i];
    }

    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au16TextureStr");
    ISPCFG_GetNumInLine(pString);
    idx = 0;
    for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
    {
        for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
        {
            pStaticSharpen->au16TextureStr[i][j] = (HI_U16)ISPCFG_LineValue[idx++];
        }
    }

    ISPCFG_GetString(viPipe, &pString, "hiISP_SHARPEN_ATTR_S:stAuto.au16EdgeStr");
    ISPCFG_GetNumInLine(pString);
     idx = 0;
    for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
    {
        for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
        {
            pStaticSharpen->au16EdgeStr[i][j] = (HI_U16)ISPCFG_LineValue[idx++];
        }
    }

    return 0;
}

static int ISPCFG_GetDemosaic(VI_PIPE viPipe, ISPCFG_DEMOSAIC_PARAM_S *pStaticDemosaic)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticDemosaic))
    {
        prtMD("invalid input viPipe = %d, pStaticDemosaic = %p\n", viPipe, pStaticDemosaic);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:bEnable");
    pStaticDemosaic->bEnable = (HI_BOOL)u32Value;

    /*OpType*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:enOpType");
    pStaticDemosaic->enOpType = (HI_S32)u32Value;

    /* 手动模式 */
    /*ManualNonDirStr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:stManual.u8NonDirStr");
    pStaticDemosaic->u8ManualNonDirStr = (HI_U8)u32Value;

    /*ManualNonDirMFDetailEhcStr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:stManual.u8NonDirMFDetailEhcStr");
    pStaticDemosaic->u8ManualNonDirMFDetailEhcStr = (HI_U8)u32Value;

    /*ManualNonDirHFDetailEhcStr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:stManual.u8NonDirHFDetailEhcStr");
    pStaticDemosaic->u8ManualNonDirHFDetailEhcStr = (HI_U8)u32Value;

    /*ManualDetailSmoothRange*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_DEMOSAIC_ATTR_S:stManual.u8DetailSmoothRange");
    pStaticDemosaic->u16ManualDetailSmoothStr = (HI_U16)u32Value;

    /* 自动模式 */
    /*AutoColorNoiseThdF*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8ColorNoiseThdF");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseThdF[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseStrF*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8ColorNoiseStrF");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseStrF[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseThdY*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8ColorNoiseThdY");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseThdY[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseStrY*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8ColorNoiseStrY");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseStrY[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirStr*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8NonDirStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirMFDetailEhcStr*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8NonDirMFDetailEhcStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirMFDetailEhcStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirHFDetailEhcStr*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8NonDirHFDetailEhcStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirHFDetailEhcStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoDetailSmoothRange*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_DEMOSAIC_ATTR_S:stAuto.au8DetailSmoothRange");
    ISPCFG_GetNumInLine(pString);

    prtMD("pStaticDemosaic->u16AutoDetailSmoothStr:");
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u16AutoDetailSmoothRange[u32IdxM] = ISPCFG_LineValue[u32IdxM];

        printf(" %d", pStaticDemosaic->u16AutoDetailSmoothRange[u32IdxM]);
    }
    printf("\n");

    return 0;
}

static int ISPCFG_GetViNR(VI_PIPE viPipe, ISPCFG_VI_NR_PARAM_S *pNrxParam)
{
    int value = 0;
    char *pString = NULL;
    unsigned int i = 0;
    VI_PIPE_NRX_PARAM_V2_S *pNrnManuParamV2 = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pNrxParam))
    {
        prtMD("invalid input viPipe = %d, pNrxParam = %p\n", viPipe, pNrxParam);
        return -1;
    }

    /* 使能标志 */
    value = ISPCFG_GetValue(viPipe, "hiISP_3DNRX_ATTR_S:bNrEn");
    pNrxParam->bOpen = (HI_U8)value;

    /* 版本信息确认 */
    value = ISPCFG_GetValue(viPipe, "hiISP_3DNRX_ATTR_S:enNRVersion");
    pNrxParam->stNrParam.enNRVersion = value;
    if (pNrxParam->stNrParam.enNRVersion != VI_NR_V2)
    {
        prtMD("not supported! value = %d\n", value);
        return -1;
    }

    /* 手动 or 自动 */
    value = ISPCFG_GetValue(viPipe, "hiISP_3DNRX_ATTR_S:enOptMode");
    pNrxParam->stNrParam.stNRXParamV2.enOptMode = value;
    if (pNrxParam->stNrParam.stNRXParamV2.enOptMode != OPERATION_MODE_MANUAL)
    {
        prtMD("not supported! value = %d\n", value);
        return -1;
    }

    pNrnManuParamV2 = &pNrxParam->stNrParam.stNRXParamV2.stNRXManualV2.stNRXParamV2;

    /* 手动参数获取 */
    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SPN6");
    pNrnManuParamV2->SFy.SPN6 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFR");
    pNrnManuParamV2->SFy.SFR = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SBN6");
    pNrnManuParamV2->SFy.SBN6 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.PBR6");
    pNrnManuParamV2->SFy.PBR6 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SRT0");
    pNrnManuParamV2->SFy.SRT0 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SRT1");
    pNrnManuParamV2->SFy.SRT1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.JMODE");
    pNrnManuParamV2->SFy.JMODE = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.DeIdx");
    pNrnManuParamV2->SFy.DeIdx = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.DeRate");
    pNrnManuParamV2->SFy.DeRate = value;

    value = ISPCFG_GetString(viPipe, &pString, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFR6");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < 3; i++)
    {
        pNrnManuParamV2->SFy.SFR6[i] = ISPCFG_LineValue[i];
    }

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFS1");
    pNrnManuParamV2->SFy.SFS1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFT1");
    pNrnManuParamV2->SFy.SFT1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SBR1");
    pNrnManuParamV2->SFy.SBR1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFS2");
    pNrnManuParamV2->SFy.SFS2 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFT2");
    pNrnManuParamV2->SFy.SFT2 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SBR2");
    pNrnManuParamV2->SFy.SBR2 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFS4");
    pNrnManuParamV2->SFy.SFS4 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFT4");
    pNrnManuParamV2->SFy.SFT4 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SBR4");
    pNrnManuParamV2->SFy.SBR4 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.STH1");
    pNrnManuParamV2->SFy.STH1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFN1");
    pNrnManuParamV2->SFy.SFN1 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.NRyEn");
    pNrnManuParamV2->SFy.NRyEn = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFN0");
    pNrnManuParamV2->SFy.SFN0 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.STH2");
    pNrnManuParamV2->SFy.STH2 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFN2");
    pNrnManuParamV2->SFy.SFN2 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.BWSF4");
    pNrnManuParamV2->SFy.BWSF4 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.kMode");
    pNrnManuParamV2->SFy.kMode = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.STH3");
    pNrnManuParamV2->SFy.STH3 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.SFN3");
    pNrnManuParamV2->SFy.SFN3 = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy.TriTh");
    pNrnManuParamV2->SFy.TriTh = value;

    value = ISPCFG_GetValue(viPipe, "hiNRX_PARAM_MANUAL_V2_S:stNRXParamV2.SFy._rb0_");
    pNrnManuParamV2->SFy._rb0_ = value;

    return 0;
}

static int ISPCFG_GetVpssNR(VI_PIPE viPipe, ISPCFG_VPSS_NR_PARAM_S *pNrxParam)
{
    int value = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    char strings[256] = {0};
    char *pString = NULL;
    VPSS_NRX_V2_S *pNrxManual = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pNrxParam))
    {
        prtMD("invalid input viPipe = %d, pNrxParam = %p\n", viPipe, pNrxParam);
        return -1;
    }

    /* 开启 or 关闭 */
    value = ISPCFG_GetValue(viPipe, "hiVPSS_3DNRX_ATTR_S:bNrEn");
    pNrxParam->bOpen = value;

    /* 版本信息 */
    value = ISPCFG_GetValue(viPipe, "hiVPSS_3DNRX_ATTR_S:enNRVersion");
    pNrxParam->stNrParam.enNRVer = value;
    if (pNrxParam->stNrParam.enNRVer != VPSS_NR_V2)
    {
        prtMD("unsupported value = %d\n", value);
        return -1;
    }

    /* 手动或自动 */
    value = ISPCFG_GetValue(viPipe, "hiVPSS_3DNRX_ATTR_S:enOptMode");
    pNrxParam->stNrParam.stNRXParam_V2.enOptMode = value;
    if (pNrxParam->stNrParam.stNRXParam_V2.enOptMode != OPERATION_MODE_MANUAL)
    {
        prtMD("unsupported value = %d\n", value);
        return -1;
    }

    pNrxManual = &pNrxParam->stNrParam.stNRXParam_V2.stNRXManual.stNRXParam;

    for (i = 0; i < 3; i++)
    {
        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d].IES0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i].IES0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d].IES1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i].IES1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d].IES2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i].IES2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d].IES3", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i].IES3 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d].IEDZ", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i].IEDZ = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.IEy[%d]._rb_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->IEy[i]._rb_ = value;
    }

    for (i = 0; i < 3; i++)
    {
        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SPN6", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SPN6 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFR", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFR = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SBN6", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SBN6 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].PBR6", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].PBR6 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SRT0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SRT0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SRT1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SRT1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].JMODE", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].JMODE = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].DeIdx", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].DeIdx = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].DeRate", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].DeRate = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFR6", i);
        value = ISPCFG_GetString(viPipe, &pString, strings);
        ISPCFG_GetNumInLine(pString);
        for (j = 0; j < 3; j++)
        {
            pNrxManual->SFy[i].SFR6[j] = ISPCFG_LineValue[j];
        }

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFS1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFS1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFT1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFT1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SBR1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SBR1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFS2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFS2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFT2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFT2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SBR2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SBR2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFS4", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFS4 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFT4", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFT4 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SBR4", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SBR4 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].STH1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].STH1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFN1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFN1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFN0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFN0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].NRyEn", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].NRyEn = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].STH2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].STH2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFN2", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFN2 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].BWSF4", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].BWSF4 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].kMode", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].kMode = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].STH3", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].STH3 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SFN3", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].SFN3 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].TriTh", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i].TriTh = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d]._rb0_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->SFy[i]._rb0_ = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SBSk", i);
        value = ISPCFG_GetString(viPipe, &pString, strings);
        ISPCFG_GetNumInLine(pString);
        for (j = 0; j < 32; j++)
        {
            pNrxManual->SFy[i].SBSk[j] = (HI_U16)ISPCFG_LineValue[j];
        }

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.SFy[%d].SDSk", i);
        value = ISPCFG_GetString(viPipe, &pString, strings);
        ISPCFG_GetNumInLine(pString);
        for (j = 0; j < 32; j++)
        {
            pNrxManual->SFy[i].SDSk[j] = (HI_U16)ISPCFG_LineValue[j];
        }
    }

    for (i = 0; i < 2; i++)
    {
        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MADZ0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MADZ0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI00", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI00 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI01", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI01 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI02", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI02 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].biPath", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].biPath = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MADZ1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MADZ1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI10", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI10 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI11", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI11 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MAI12", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MAI12 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d]._rb0_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i]._rb0_ = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MABR0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MABR0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MABR1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MABR1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MATH0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MATH0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MATE0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MATE0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MATW", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MATW = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MATH1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MATH1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MATE1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MATE1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d]._rb1_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i]._rb1_ = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MASW", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MASW = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d]._rb2_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i]._rb2_ = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MABW0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MABW0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.MDy[%d].MABW1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->MDy[i].MABW1 = value;
    }

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.RFs.advMATH");
    pNrxManual->RFs.advMATH = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.RFs.RFDZ");
    pNrxManual->RFs.RFDZ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.RFs._rb_");
    pNrxManual->RFs._rb_ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.RFs.RFUI");
    pNrxManual->RFs.RFUI = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.RFs.RFSLP");
    pNrxManual->RFs.RFSLP = value;

    for (i = 0; i < 2; i++)
    {
        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TFS0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TFS0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TDZ0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TDZ0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TDX0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TDX0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TFS1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TFS1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TDZ1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TDZ1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TDX1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TDX1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].SDZ0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].SDZ0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].STR0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].STR0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].DZMode0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].DZMode0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].SDZ1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].SDZ1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].STR1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].STR1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].DZMode1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].DZMode1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TFR0", i);
        ISPCFG_GetString(viPipe, &pString, strings);
        ISPCFG_GetNumInLine(pString);
        for (j = 0; j < 6; j++)
        {
            pNrxManual->TFy[i].TFR0[j] = ISPCFG_LineValue[j];
        }

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TSS0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TSS0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TSI0", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TSI0 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TFR1", i);
        ISPCFG_GetString(viPipe, &pString, strings);
        ISPCFG_GetNumInLine(pString);
        for (j = 0; j < 6; j++)
        {
            pNrxManual->TFy[i].TFR1[j] = ISPCFG_LineValue[j];
        }

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TSS1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TSS1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].TSI1", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].TSI1 = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].RFI", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].RFI = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].tEdge", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].tEdge = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d].bRef", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i].bRef = value;

        snprintf(strings, 256, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.TFy[%d]._rb_", i);
        value = ISPCFG_GetValue(viPipe, strings);
        pNrxManual->TFy[i]._rb_ = value;
    }

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc.SFC");
    pNrxManual->pNRc.SFC = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc.CTFS");
    pNrxManual->pNRc.CTFS = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc._rb_");
    pNrxManual->pNRc._rb_ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc.TFC");
    pNrxManual->pNRc.TFC = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc._rb1_");
    pNrxManual->pNRc._rb1_ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc.MODE");
    pNrxManual->pNRc.MODE = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc._rb2_");
    pNrxManual->pNRc._rb2_ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.pNRc.PRESFC");
    pNrxManual->pNRc.PRESFC = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy.IES0");
    pNrxManual->NRc.IEy.IES0 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy.IES1");
    pNrxManual->NRc.IEy.IES1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy.IES2");
    pNrxManual->NRc.IEy.IES2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy.IES3");
    pNrxManual->NRc.IEy.IES3 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy.IEDZ");
    pNrxManual->NRc.IEy.IEDZ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.IEy._rb_");
    pNrxManual->NRc.IEy._rb_ = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SPN6");
    pNrxManual->NRc.SFy.SPN6 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFR");
    pNrxManual->NRc.SFy.SFR = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SBN6");
    pNrxManual->NRc.SFy.SBN6 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.PBR6");
    pNrxManual->NRc.SFy.PBR6 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SRT0");
    pNrxManual->NRc.SFy.SRT0 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SRT1");
    pNrxManual->NRc.SFy.SRT1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.JMODE");
    pNrxManual->NRc.SFy.JMODE = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.DeIdx");
    pNrxManual->NRc.SFy.DeIdx = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.DeRate");
    pNrxManual->NRc.SFy.DeRate = value;

    value = ISPCFG_GetString(viPipe, &pString, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFR6");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < 3; i++)
    {
        pNrxManual->NRc.SFy.SFR6[i] = ISPCFG_LineValue[i];
    }

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFS1");
    pNrxManual->NRc.SFy.SFS1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFT1");
    pNrxManual->NRc.SFy.SFT1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SBR1");
    pNrxManual->NRc.SFy.SBR1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFS2");
    pNrxManual->NRc.SFy.SFS2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFT2");
    pNrxManual->NRc.SFy.SFT2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SBR2");
    pNrxManual->NRc.SFy.SBR2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFS4");
    pNrxManual->NRc.SFy.SFS4 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFT4");
    pNrxManual->NRc.SFy.SFT4 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SBR4");
    pNrxManual->NRc.SFy.SBR4 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.STH1");
    pNrxManual->NRc.SFy.STH1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFN1");
    pNrxManual->NRc.SFy.SFN1 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFN0");
    pNrxManual->NRc.SFy.SFN0 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.NRyEn");
    pNrxManual->NRc.SFy.NRyEn = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.STH2");
    pNrxManual->NRc.SFy.STH2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFN2");
    pNrxManual->NRc.SFy.SFN2 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.BWSF4");
    pNrxManual->NRc.SFy.BWSF4 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.kMode");
    pNrxManual->NRc.SFy.kMode = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.STH3");
    pNrxManual->NRc.SFy.STH3 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SFN3");
    pNrxManual->NRc.SFy.SFN3 = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.TriTh");
    pNrxManual->NRc.SFy.TriTh = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy._rb0_");
    pNrxManual->NRc.SFy._rb0_ = value;

    value = ISPCFG_GetString(viPipe, &pString, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SBSk");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < 32; i++)
    {
        pNrxManual->NRc.SFy.SBSk[i] = ISPCFG_LineValue[i];
    }

    value = ISPCFG_GetString(viPipe, &pString, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.SFy.SDSk");
    ISPCFG_GetNumInLine(pString);
    for (i = 0; i < 32; i++)
    {
        pNrxManual->NRc.SFy.SDSk[i] = ISPCFG_LineValue[i];
    }

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc.NRcEn");
    pNrxManual->NRc.NRcEn = value;

    value = ISPCFG_GetValue(viPipe, "hiVPSS_NRX_PARAM_MANUAL_V2_S:stNRXParam.NRc._rb_");
    pNrxManual->NRc._rb_ = value;

    return 0;
}

int ISPCFG_GetFSWDR(VI_PIPE viPipe, ISPCFG_WDRFS_PARAM_S *pStaticWDRFs)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticWDRFs))
    {
        prtMD("invalid input viPipe = %d, pStaticWDRFs = %p\n", viPipe, pStaticWDRFs);
        return -1;
    }

    /*MergeMode*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:enWDRMergeMode");
    pStaticWDRFs->enWDRMergeMode = (HI_U8)u32Value;

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.bMotionComp");
    pStaticWDRFs->bCombineMotionComp = (HI_BOOL)u32Value;

    /*CombineShortThr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.u16ShortThr");
    pStaticWDRFs->u16CombineShortThr = (HI_U16)u32Value;
    //prtMD("pStaticWDRFs->u16CombineShortThr = %d\n",pStaticWDRFs->u16CombineShortThr);

    /*CombineLongThr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.u16LongThr");
    pStaticWDRFs->u16CombineLongThr = (HI_U16)u32Value;
    //prtMD("pStaticWDRFs->u16CombineLongThr = %d\n",pStaticWDRFs->u16CombineLongThr);

    /*CombineForceLong*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.bForceLong");
    pStaticWDRFs->bCombineForceLong = (HI_BOOL)u32Value;

    /*CombineForceLongLowThr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.u16ForceLongLowThr");
    pStaticWDRFs->u16CombineForceLongLowThr = (HI_U16)u32Value;

    /*CombineForceLongHigThr*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.u16ForceLongHigThr");
    pStaticWDRFs->u16CombineForceLongHigThr = (HI_U16)u32Value;

    /*WDRMdtShortExpoChk*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.bShortExpoChk");
    pStaticWDRFs->bWDRMdtShortExpoChk = (HI_U16)u32Value;

    /*WDRMdtShortCheckThd*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.u16ShortCheckThd");
    pStaticWDRFs->u16WDRMdtShortCheckThd = (HI_U16)u32Value;

    /*WDRMdtMDRefFlicker*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.bMDRefFlicker");
    pStaticWDRFs->bWDRMdtMDRefFlicker = (HI_BOOL)u32Value;

    /*WDRMdtMdtStillThd*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.u8MdtStillThd");
    pStaticWDRFs->u8WDRMdtMdtStillThd = (HI_U8)u32Value;

    /*WDRMdtMdtLongBlend*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.u8MdtLongBlend");
    pStaticWDRFs->u8WDRMdtMdtLongBlend = (HI_U8)u32Value;

    /*WDRMdtOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.enOpType");
    pStaticWDRFs->u8WDRMdtOpType = (HI_U8)u32Value;

    /*AutoMdThrLowGain*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.stAuto.au8MdThrLowGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < ISP_AUTO_ISO_STRENGTH_NUM; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->au8AutoMdThrLowGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoMdThrHigGain*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_WDR_FS_ATTR_S:stWDRCombine.stWDRMdt.stAuto.au8MdThrHigGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < ISP_AUTO_ISO_STRENGTH_NUM; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->au8AutoMdThrHigGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*FusionFusionThr*/
    ISPCFG_GetString(viPipe, &pString, "hiISP_WDR_FS_ATTR_S:stFusion.au16FusionThr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->u16FusionFusionThr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    return 0;
}

static int ISPCFG_GetGamma(VI_PIPE viPipe, ISPCFG_GAMMA_PARAM_S *pDynamicGamma)
{
    int i = 0;
    unsigned int u32Value = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pDynamicGamma == NULL))
    {
        prtMD("invalid input viPipe = %d, pDynamicGanma = %p\n", viPipe, pDynamicGamma);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_GAMMA_ATTR_S:bEnable");
    pDynamicGamma->bEnable = (int)u32Value;

    /*enCurveType*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_GAMMA_ATTR_S:enCurveType");
    pDynamicGamma->enCurveType = (ISP_GAMMA_CURVE_TYPE_E)u32Value;

    /* Table */
    ISPCFG_GetString(viPipe, &pString, "hiISP_GAMMA_ATTR_S:u16Table");
    ISPCFG_GetNumInLine(pString);

    for(i = 0; i < GAMMA_NODE_NUM; i++)
    {
        pDynamicGamma->u16Table[i] = ISPCFG_LineValue[i];
    }

    return 0;
}

static int ISPCFG_GetBNR(VI_PIPE viPipe, ISPCFG_BNR_PARAM_S *pStaticBnr)
{
    unsigned int u32Value = 0;
    unsigned int u32Idx = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    char * pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticBnr == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticBnr = %p\n", viPipe, pStaticBnr);
        return -1;
    }

    /*BnrEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_NR_ATTR_S:bEnable");
    pStaticBnr->bEnable = (HI_BOOL)u32Value;

    /*NrLscEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_NR_ATTR_S:bNrLscEnable");
    pStaticBnr->bNrLscEnable = (HI_BOOL)u32Value;

    /*BnrLscMaxGain*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_NR_ATTR_S:u8BnrLscMaxGain");
    pStaticBnr->u8BnrMaxGain = (HI_U8)u32Value;

    /*BnrLscCmpStrength*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_NR_ATTR_S:u16BnrLscCmpStrength");
    pStaticBnr->u16BnrLscCmpStrength = (HI_U16)u32Value;

    /*BnrOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "hiISP_NR_ATTR_S:enOpType");
    pStaticBnr->u8BnrOptype = (HI_U8)u32Value;

    /* AutoChromaStr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_NR_ATTR_S:stAuto.au8ChromaStr");
    ISPCFG_GetNumInLine(pString);

    u32Idx = 0;
    for (i = 0; i < ISP_BAYER_CHN_NUM; i++)
    {
        for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
        {
            pStaticBnr->au8ChromaStr[i][j] = ISPCFG_LineValue[u32Idx ++];
        }
    }

    /* AutoFineStr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_NR_ATTR_S:stAuto.au8FineStr");
    ISPCFG_GetNumInLine(pString);

    u32Idx = 0;
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        pStaticBnr->au8FineStr[i] = ISPCFG_LineValue[u32Idx ++];
    }

    /* AutoCoringWgt */
    ISPCFG_GetString(viPipe, &pString, "hiISP_NR_ATTR_S:stAuto.au16CoringWgt");
    ISPCFG_GetNumInLine(pString);

    u32Idx = 0;
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        pStaticBnr->au16CoringWgt[i] = ISPCFG_LineValue[u32Idx ++];
    }

    /* AutoCoarseSt */
    ISPCFG_GetString(viPipe, &pString, "hiISP_NR_ATTR_S:stAuto.au16CoarseStr");
    ISPCFG_GetNumInLine(pString);

    u32Idx = 0;
    for (i = 0; i < ISP_BAYER_CHN_NUM; i++)
    {
        for(j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
        {
            pStaticBnr->au16CoarseStr[i][j] = ISPCFG_LineValue[u32Idx ++];
        }
    }

    /* WDRFrameStr */
    ISPCFG_GetString(viPipe, &pString, "hiISP_NR_ATTR_S:stWdr.au8WDRFrameStr");
    ISPCFG_GetNumInLine(pString);

    u32Idx = 0;
    for(i = 0; i < ISP_BAYER_CHN_NUM; i++)
    {
        pStaticBnr->au8WDRFrameStr[i] = ISPCFG_LineValue[u32Idx ++];
    }

    return 0;
}

//设置ISP各个模块的控制参数
int ISPCFG_SetBypass(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = -1;
    ISP_MODULE_CTRL_U stModCtrl = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetModuleControl(ViPipe, &stModCtrl);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetModuleControl is failed!\n");
    }

    stModCtrl.bitBypassISPDGain =  stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassISPDGain;
    stModCtrl.bitBypassAntiFC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassAntiFC;
    stModCtrl.bitBypassCrosstalkR = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassCrosstalkR;

    stModCtrl.bitBypassDPC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassDPC;
    stModCtrl.bitBypassNR = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassNR;
    stModCtrl.bitBypassDehaze = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassDehaze;
    stModCtrl.bitBypassWBGain = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassWBGain;

    stModCtrl.bitBypassMeshShading = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassMeshShading;
    stModCtrl.bitBypassDRC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassDRC;
    stModCtrl.bitBypassDemosaic = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassDemosaic;
    stModCtrl.bitBypassColorMatrix = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassColorMatrix;
    stModCtrl.bitBypassGamma = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassGamma;
    stModCtrl.bitBypassFSWDR = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassFSWDR;

    stModCtrl.bitBypassCA = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassCA;
    stModCtrl.bitBypassCsConv = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassCsConv;
    stModCtrl.bitBypassSharpen = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassSharpen;
    stModCtrl.bitBypassLCAC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassLCAC;
    stModCtrl.bitBypassGCAC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassGCAC;

    stModCtrl.bit2ChnSelect = stISPCfgPipeParam[ViPipe].stBypassParam.bit2ChnSelect;
    stModCtrl.bitBypassLdci = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassLdci;

    stModCtrl.bitBypassPreGamma = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassPreGamma;
    stModCtrl.bitBypassAEStatFE = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassAEStatFE;

    stModCtrl.bitBypassAEStatBE = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassAEStatBE;
    stModCtrl.bitBypassMGStat = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassMGSta;
    stModCtrl.bitBypassDE = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassDE;
    stModCtrl.bitBypassAFStatBE = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassAFStatBE;

    stModCtrl.bitBypassAWBStat = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassAWBStat;
    stModCtrl.bitBypassCLUT = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassCLUT;
    stModCtrl.bitBypassHLC = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassHLC;
    stModCtrl.bitBypassEdgeMark = stISPCfgPipeParam[ViPipe].stBypassParam.bitBypassEdgeMark;

    prtMD("stModCtrl.bitBypassEdgeMark = %d\n", stModCtrl.bitBypassEdgeMark);
    s32Ret = HI_MPI_ISP_SetModuleControl(ViPipe, &stModCtrl);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetModuleControl is failed!\n");
    }

    return 0;
}

static int ISPCFG_SetBlackLevel(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;
    int i = 0;
    ISP_BLACK_LEVEL_S stBlackLevel = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetBlackLevelAttr(ViPipe, &stBlackLevel);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetBlackLevelAttr is failed!s32Ret = %#x\n",s32Ret);
    }

    stBlackLevel.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stBlackParam.enOpType;
    for(i = 0;i < 4;i++)
    {
        stBlackLevel.au16BlackLevel[i] = stISPCfgPipeParam[ViPipe].stBlackParam.au16BlackLevel[i];
    }

    s32Ret = HI_MPI_ISP_SetBlackLevelAttr(ViPipe, &stBlackLevel);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetBlackLevelAttr is failed!s32Ret = %#x\n",s32Ret);
    }

    return 0;
}

/* 设置自动曝光参数值 */
static int ISPCFG_SetAE(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_EXPOSURE_ATTR_S stExposureAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetExposureAttr(ViPipe, &stExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    /* AE 扩展分配路线是否生效开关: 1-使用 AE 扩展分配路线; 0-使用普通 AE 分配路线 */
    stExposureAttr.bAERouteExValid = stISPCfgPipeParam[ViPipe].stAeParam.bAERouteExValid;

    /* 曝光类型: 0-auto;1-手动 */
    stExposureAttr.enOpType = stISPCfgPipeParam[ViPipe].stAeParam.ExposureOpType;

    /* AE 算法运行的间隔 */
    stExposureAttr.u8AERunInterval = stISPCfgPipeParam[ViPipe].stAeParam.u8AERunInterval;

    /* 曝光时间 */
    stExposureAttr.stAuto.stExpTimeRange.u32Max = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoExpTimeMax;
    stExposureAttr.stAuto.stExpTimeRange.u32Min = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoExpTimeMin;

    /* sensor模拟增益 */
    stExposureAttr.stAuto.stAGainRange.u32Max = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoAGainMax;
    stExposureAttr.stAuto.stAGainRange.u32Min = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoAGainMin;

    /* sensor数字增益 */
    stExposureAttr.stAuto.stDGainRange.u32Max = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoDGainMax;
    stExposureAttr.stAuto.stDGainRange.u32Min = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoDGainMin;

    /* ISP数字增益 */
    stExposureAttr.stAuto.stISPDGainRange.u32Max = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoISPDGainMax;
    stExposureAttr.stAuto.stISPDGainRange.u32Min = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoISPDGainMin;

    /* 系统增益 */
    stExposureAttr.stAuto.stSysGainRange.u32Max = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoSysGainMax;
    stExposureAttr.stAuto.stSysGainRange.u32Min = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoSysGainMin;

    /* 自动降帧时的系统增益门限值 */
    stExposureAttr.stAuto.u32GainThreshold = stISPCfgPipeParam[ViPipe].stAeParam.u32AutoGainThreshold;

    /* 自动曝光调整速度 */
    stExposureAttr.stAuto.u8Speed = stISPCfgPipeParam[ViPipe].stAeParam.u8AutoSpeed;

    /* 画面由暗到亮 AE 调节速度的偏差值，该值越大，画面从暗到 亮的速度越快 */
    stExposureAttr.stAuto.u16BlackSpeedBias = stISPCfgPipeParam[ViPipe].stAeParam.u16AutoBlackSpeedBias;

    /* 自动曝光调整时的目标亮度 */
    stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stAeParam.u8AutostaticCompesation;

    /* 自动曝光调整时的曝光量偏差值 */
    stExposureAttr.stAuto.u16EVBias = stISPCfgPipeParam[ViPipe].stAeParam.u16AutoEVBias;

    /* 自动曝光策略:高光优先或低光优先 */
    stExposureAttr.stAuto.enAEStrategyMode = stISPCfgPipeParam[ViPipe].stAeParam.bAutoAEStrategMode;

    /* 感兴趣区域的权重 */
    stExposureAttr.stAuto.u16HistRatioSlope = stISPCfgPipeParam[ViPipe].stAeParam.u16AutoHistRatioSlope;

    /* 感兴趣区域对统计平均值影响的最大程度 */
    stExposureAttr.stAuto.u8MaxHistOffset = stISPCfgPipeParam[ViPipe].stAeParam.u8AutoMaxHistOffset;

    /* 自动曝光调整时对画面亮度的容忍偏差 */
    stExposureAttr.stAuto.u8Tolerance = stISPCfgPipeParam[ViPipe].stAeParam.u8AutoTolerance;

    /* 延时属性设置 */
    stExposureAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = stISPCfgPipeParam[ViPipe].stAeParam.u16AutoBlackDelayFrame;
    stExposureAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = stISPCfgPipeParam[ViPipe].stAeParam.u16AutoWhiteDelayFrame;

    //stExposureAttr.stAuto.enFSWDRMode = stISPCfgPipeParam[ViPipe].stAeParam.enFSWDRMode;

    prtMD("bAERouteExValid = %d, u8AERunInterval = %d, u32Max = %d, u8Compensation = %d\n",
        stExposureAttr.bAERouteExValid,
        stExposureAttr.u8AERunInterval,
        stExposureAttr.stAuto.stExpTimeRange.u32Max,
        stExposureAttr.stAuto.u8Compensation);

    s32Ret = HI_MPI_ISP_SetExposureAttr(ViPipe, &stExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetLongFrameAE(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_EXPOSURE_ATTR_S stExposureAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetExposureAttr(ViPipe, &stExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stExposureAttr.stAuto.enFSWDRMode = 1;  //长帧模式
    prtMD("stExposureAttr.stAuto.u8Compensation = %d\n", stExposureAttr.stAuto.enFSWDRMode);

    s32Ret = HI_MPI_ISP_SetExposureAttr(ViPipe, &stExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetAWB(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_WB_ATTR_S stWbAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetWBAttr(ViPipe, &stWbAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetWBAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stWbAttr.stAuto.u16RefColorTemp = stISPCfgPipeParam[ViPipe].stAwbParam.u16AutoRefColorTemp;
    for(i = 0;i < 4;i++)
    {
        stWbAttr.stAuto.au16StaticWB[i] = stISPCfgPipeParam[ViPipe].stAwbParam.au16AutoStaticWB[i];
    }

    for(i = 0; i < 6;i++)
    {
        stWbAttr.stAuto.as32CurvePara[i] = stISPCfgPipeParam[ViPipe].stAwbParam.as32AutoCurvePara[i];
    }

    stWbAttr.stAuto.u8RGStrength = stISPCfgPipeParam[ViPipe].stAwbParam.u8AutoRGStrength;
    stWbAttr.stAuto.u8BGStrength = stISPCfgPipeParam[ViPipe].stAwbParam.u8AutoBGStrength;
    stWbAttr.stAuto.u16Speed = stISPCfgPipeParam[ViPipe].stAwbParam.u16AutoSpeed;
    stWbAttr.stAuto.u16ZoneSel = stISPCfgPipeParam[ViPipe].stAwbParam.u16AutoZoneSel;
    stWbAttr.stAuto.u16HighColorTemp = stISPCfgPipeParam[ViPipe].stAwbParam.u16AutoHighColorTemp;
    stWbAttr.stAuto.u16LowColorTemp = stISPCfgPipeParam[ViPipe].stAwbParam.u16AutoLowColorTemp;

    s32Ret = HI_MPI_ISP_SetWBAttr(ViPipe, &stWbAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetWBAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetWdrAE(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_WDR_EXPOSURE_ATTR_S stWdrExposureAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetWDRExposureAttr(ViPipe, &stWdrExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetWDRExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stWdrExposureAttr.enExpRatioType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stWdrAeParam.u8ExpRatioType;
    stWdrExposureAttr.u32ExpRatioMax = stISPCfgPipeParam[ViPipe].stWdrAeParam.u32ExpRatioMax;
    stWdrExposureAttr.u32ExpRatioMin = stISPCfgPipeParam[ViPipe].stWdrAeParam.u32ExpRatioMin;

    stWdrExposureAttr.u16Tolerance = stISPCfgPipeParam[ViPipe].stWdrAeParam.u16Tolerance;
    stWdrExposureAttr.u16Speed = stISPCfgPipeParam[ViPipe].stWdrAeParam.u16Speed;
    stWdrExposureAttr.u16RatioBias = stISPCfgPipeParam[ViPipe].stWdrAeParam.u16RatioBias;
    for(i = 0 ; i < 3; i++)
    {
        stWdrExposureAttr.au32ExpRatio[i] = stISPCfgPipeParam[ViPipe].stWdrAeParam.au32ExpRatio[i];
    }

    s32Ret = HI_MPI_ISP_SetWDRExposureAttr(ViPipe, &stWdrExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetWDRExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetSaturation(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    ISP_SATURATION_ATTR_S stSaturationAttr;

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetSaturationAttr(ViPipe, &stSaturationAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetSaturationAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stSaturationAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stSaturationParam.bOpType;
    stSaturationAttr.stManual.u8Saturation = stISPCfgPipeParam[ViPipe].stSaturationParam.u8ManualSat;

    for (i = 0; i < 16; i++)
    {
        stSaturationAttr.stAuto.au8Sat[i] = stISPCfgPipeParam[ViPipe].stSaturationParam.au8AutoSat[i];
    }

    s32Ret = HI_MPI_ISP_SetSaturationAttr(ViPipe, &stSaturationAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetSaturationAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetDRC(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DRC_ATTR_S stDrcAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetDRCAttr(ViPipe, &stDrcAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDRCAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stDrcAttr.bEnable = stISPCfgPipeParam[ViPipe].stDrcParam.bEnable;
    stDrcAttr.enCurveSelect = (ISP_DRC_CURVE_SELECT_E)stISPCfgPipeParam[ViPipe].stDrcParam.u8CurveSelect;

    stDrcAttr.u8PDStrength = stISPCfgPipeParam[ViPipe].stDrcParam.u8PDStrength;
    stDrcAttr.u8LocalMixingBrightMax = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingBrightMax;
    stDrcAttr.u8LocalMixingBrightMin = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingBrightMin;
    stDrcAttr.u8LocalMixingBrightThr = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingBrightThr;
    stDrcAttr.s8LocalMixingBrightSlo = stISPCfgPipeParam[ViPipe].stDrcParam.s8LocalMixingBrightSlo;

    stDrcAttr.u8LocalMixingDarkMax = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingDarkMax;
    stDrcAttr.u8LocalMixingDarkMin = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingDarkMin;
    stDrcAttr.u8LocalMixingDarkThr = stISPCfgPipeParam[ViPipe].stDrcParam.u8LocalMixingDarkThr;
    stDrcAttr.s8LocalMixingDarkSlo = stISPCfgPipeParam[ViPipe].stDrcParam.s8LocalMixingDarkSlo;

    stDrcAttr.u8BrightGainLmt = stISPCfgPipeParam[ViPipe].stDrcParam.u8BrightGainLmt;
    stDrcAttr.u8BrightGainLmtStep = stISPCfgPipeParam[ViPipe].stDrcParam.u8BrightGainLmtStep;
    stDrcAttr.u8DarkGainLmtY = stISPCfgPipeParam[ViPipe].stDrcParam.u8DarkGainLmtY;
    stDrcAttr.u8DarkGainLmtC = stISPCfgPipeParam[ViPipe].stDrcParam.u8DarkGainLmtC;
    stDrcAttr.u8ContrastControl = stISPCfgPipeParam[ViPipe].stDrcParam.u8ContrastControl;
    stDrcAttr.s8DetailAdjustFactor = stISPCfgPipeParam[ViPipe].stDrcParam.s8DetailAdjustFactor;
    stDrcAttr.u8SpatialFltCoef = stISPCfgPipeParam[ViPipe].stDrcParam.u8SpatialFltCoef;
    stDrcAttr.u8RangeFltCoef = stISPCfgPipeParam[ViPipe].stDrcParam.u8RangeFltCoef;
    stDrcAttr.u8RangeAdaMax = stISPCfgPipeParam[ViPipe].stDrcParam.u8RangeAdaMax;
    stDrcAttr.u8GradRevMax = stISPCfgPipeParam[ViPipe].stDrcParam.u8GradRevMax;
    stDrcAttr.u8GradRevThr = stISPCfgPipeParam[ViPipe].stDrcParam.u8GradRevThr;
    stDrcAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stDrcParam.u8DRCOpType;
    stDrcAttr.stAuto.u16Strength = stISPCfgPipeParam[ViPipe].stDrcParam.u16AutoStrength;
    stDrcAttr.stAuto.u16StrengthMax = stISPCfgPipeParam[ViPipe].stDrcParam.u16AutoStrengthMax;
    stDrcAttr.stAuto.u16StrengthMin = stISPCfgPipeParam[ViPipe].stDrcParam.u16AutoStrengthMin;
    stDrcAttr.stManual.u16Strength = stISPCfgPipeParam[ViPipe].stDrcParam.u16ManualStrength;

    stDrcAttr.stAsymmetryCurve.u8Asymmetry = stISPCfgPipeParam[ViPipe].stDrcParam.u8Asymmetry;
    stDrcAttr.stAsymmetryCurve.u8Compress = stISPCfgPipeParam[ViPipe].stDrcParam.u8Compress;
    stDrcAttr.stAsymmetryCurve.u8SecondPole = stISPCfgPipeParam[ViPipe].stDrcParam.u8SecondPole;
    stDrcAttr.stAsymmetryCurve.u8Stretch = stISPCfgPipeParam[ViPipe].stDrcParam.u8Stretch;
#if 0
    HI_S32 i = 0,j = 0;

    for(i = 0;i < 10; i++)
    {
    for(j = 0; j < 20;j++)
    {
    stDrcAttr.au16ToneMappingValue[(i * 20) + j] = stISPCfgPipeParam[ViPipe].stDrcParam.au16ToneMappingValue[i][j];
    //prtMD("stDrcAttr.au16ToneMappingValue[%d] = %d\n",((i * 20) + j),stDrcAttr.au16ToneMappingValue[(i * 20) + j]);
    }
    }
#endif
#if 0
    for(i = 0;i < 33; i++)
    {
    stDrcAttr.au16ColorCorrectionLut[i] = stISPCfgPipeParam[ViPipe].stDrcParam.au16ColorCorrectionLut[i];
    }
#endif

    s32Ret = HI_MPI_ISP_SetDRCAttr(ViPipe, &stDrcAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDRCAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetCCM(VI_PIPE ViPipe)
{
    HI_S32 i = 0,j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_COLORMATRIX_ATTR_S stCCMAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetCCMAttr(ViPipe,&stCCMAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetCCMAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stCCMAttr.enOpType = stISPCfgPipeParam[ViPipe].stCcmParam.CcmOpType;
    if(stCCMAttr.enOpType == OP_TYPE_AUTO)
    {
        stCCMAttr.stAuto.bISOActEn = stISPCfgPipeParam[ViPipe].stCcmParam.bISOActEn;
        stCCMAttr.stAuto.bTempActEn = stISPCfgPipeParam[ViPipe].stCcmParam.bTempActEn;
        stCCMAttr.stAuto.u16CCMTabNum = stISPCfgPipeParam[ViPipe].stCcmParam.u16CCMTabNum;
        for(i = 0;i < stCCMAttr.stAuto.u16CCMTabNum;i++)
        {
            stCCMAttr.stAuto.astCCMTab[i].u16ColorTemp = stISPCfgPipeParam[ViPipe].stCcmParam.astCCMTab[i].u16ColorTemp;
            for(j = 0;j < 9;j++)
            {
                stCCMAttr.stAuto.astCCMTab[i].au16CCM[j] = stISPCfgPipeParam[ViPipe].stCcmParam.astCCMTab[i].au16CCM[j];
            }
        }
    }
    else if(stCCMAttr.enOpType == OP_TYPE_MANUAL)
    {
        ;//do nothing
    }

    s32Ret = HI_MPI_ISP_SetCCMAttr(ViPipe,&stCCMAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetCCMAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetDehaze(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEHAZE_ATTR_S stDehazeAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetDehazeAttr(ViPipe, &stDehazeAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDehazeAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stDehazeAttr.bEnable = stISPCfgPipeParam[ViPipe].stDehazeParam.bEnable;
    stDehazeAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stDehazeParam.u8DehazeOpType;
    stDehazeAttr.bUserLutEnable = stISPCfgPipeParam[ViPipe].stDehazeParam.bUserLutEnable;

    stDehazeAttr.stManual.u8strength = stISPCfgPipeParam[ViPipe].stDehazeParam.u8ManualStrength;
    stDehazeAttr.stAuto.u8strength = stISPCfgPipeParam[ViPipe].stDehazeParam.u8Autostrength;
    stDehazeAttr.u16TmprfltIncrCoef = stISPCfgPipeParam[ViPipe].stDehazeParam.u16prfltIncrCoef;
    stDehazeAttr.u16TmprfltDecrCoef = stISPCfgPipeParam[ViPipe].stDehazeParam.u16prfltDecrCoef;

    for (i = 0; i < 256; i++)
    {
        stDehazeAttr.au8DehazeLut[i] = stISPCfgPipeParam[ViPipe].stDehazeParam.au8DehazeLut[i];
    }

    s32Ret = HI_MPI_ISP_SetDehazeAttr(ViPipe, &stDehazeAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDehazeAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

/* 设置3A统计配置信息 */
int ISPCFG_SetStatistics(VI_PIPE ViPipe)
{
    HI_S32 i, j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_STATISTICS_CFG_S stStatisticsCfg = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetStatisticsConfig(ViPipe, &stStatisticsCfg);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    for (i = 0; i < AE_ZONE_ROW; i++)
    {
        for (j = 0; j < AE_ZONE_COLUMN; j++)
        {
            stStatisticsCfg.stAECfg.au8Weight[i][j] = stISPCfgPipeParam[ViPipe].stStatisticsParam.au8AEWeight[i][j];
        }
    }

    s32Ret = HI_MPI_ISP_SetStatisticsConfig(ViPipe, &stStatisticsCfg);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetCSC(VI_PIPE ViPipe)
{
    int s32Ret = -1;
    ISP_CSC_ATTR_S stCSCAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetCSCAttr(ViPipe, &stCSCAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stCSCAttr.bEnable = stISPCfgPipeParam[ViPipe].stCscParam.bEnable;

    stCSCAttr.u8Contr = stISPCfgPipeParam[ViPipe].stCscParam.u8Contr;
    stCSCAttr.u8Luma = stISPCfgPipeParam[ViPipe].stCscParam.u8Luma;
    if(ViPipe == 2)
    {
        stCSCAttr.u8Satu = 0;   //如果是IR数据输出，饱和度设置为0
        stCSCAttr.u8Hue = 0;  //色调可以设置为0
    }
    else
    {
        stCSCAttr.u8Satu = stISPCfgPipeParam[ViPipe].stCscParam.u8Satu;  //RGB输出
        stCSCAttr.u8Hue = stISPCfgPipeParam[ViPipe].stCscParam.u8Hue;
    }
    stCSCAttr.enColorGamut = stISPCfgPipeParam[ViPipe].stCscParam.enColorGamut;

    s32Ret = HI_MPI_ISP_SetCSCAttr(ViPipe, &stCSCAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetLDCI(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_LDCI_ATTR_S stLDCI = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetLDCIAttr(ViPipe, &stLDCI);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetLDCIAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stLDCI.bEnable = stISPCfgPipeParam[ViPipe].stLdciParam.bEnable;
    stLDCI.enOpType = stISPCfgPipeParam[ViPipe].stLdciParam.u8LDCIOpType;

    if(stLDCI.enOpType == OP_TYPE_MANUAL)
    {
        stLDCI.stManual.u16BlcCtrl = stISPCfgPipeParam[ViPipe].stLdciParam.u16ManualBlcCtrl;
    }
    else if(stLDCI.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0; i< 16;i++)
        {
            stLDCI.stAuto.au16BlcCtrl[i] = stISPCfgPipeParam[ViPipe].stLdciParam.u16AutoBlcCtrl[i];
        }
    }

    s32Ret = HI_MPI_ISP_SetLDCIAttr(ViPipe, &stLDCI);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetLDCIAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetMeshShading(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_SHADING_ATTR_S stShadingAttr;

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetMeshShadingAttr(ViPipe, &stShadingAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stShadingAttr.bEnable = stISPCfgPipeParam[ViPipe].stShadingParam.bEnable;

    s32Ret = HI_MPI_ISP_SetMeshShadingAttr(ViPipe, &stShadingAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetMeshShadingAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetSharpen(VI_PIPE ViPipe)
{
    HI_U32 i = 0, j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_SHARPEN_ATTR_S stSharpenAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetIspSharpenAttr(ViPipe, &stSharpenAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetIspSharpenAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stSharpenAttr.bEnable =  stISPCfgPipeParam[ViPipe].stSharpenParam.bEnable;
    stSharpenAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stSharpenParam.enOpType;
    if(stSharpenAttr.enOpType == OP_TYPE_MANUAL)
    {
        stSharpenAttr.stManual.u16TextureFreq = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualTextureFreq;
        stSharpenAttr.stManual.u16EdgeFreq = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualEdgeFreq;
        stSharpenAttr.stManual.u8OverShoot = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualOvershoot;
        stSharpenAttr.stManual.u8UnderShoot = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualUnderShoot;
        stSharpenAttr.stManual.u8ShootSupStr = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualSupStr;
        stSharpenAttr.stManual.u8ShootSupAdj = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualSupAdj;
        stSharpenAttr.stManual.u8DetailCtrl = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualDetailCtrl;
        stSharpenAttr.stManual.u8DetailCtrlThr = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualDetailCtrlThr;
        stSharpenAttr.stManual.u8EdgeFiltStr = stISPCfgPipeParam[ViPipe].stSharpenParam.ManualEdgeFiltStr;
    }
    else if(stSharpenAttr.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0;i < 16;i++)
        {
            stSharpenAttr.stAuto.au16TextureFreq[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoTextureFreq[i];
            //prtMD("stSharpenAttr.stAuto.au16TextureFreq[%d] = %d\n",i,stSharpenAttr.stAuto.au16TextureFreq[i]);
            stSharpenAttr.stAuto.au16EdgeFreq[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoEdgeFreq[i];
            stSharpenAttr.stAuto.au8ShootSupStr[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoSupStr[i];
            stSharpenAttr.stAuto.au8ShootSupAdj[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoSupAdj[i];
            stSharpenAttr.stAuto.au8DetailCtrl[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoDetailCtrl[i];
            stSharpenAttr.stAuto.au8DetailCtrlThr[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoDetailCtrlThr[i];
            stSharpenAttr.stAuto.au8EdgeFiltStr[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoEdgeFiltStr[i];
            stSharpenAttr.stAuto.au8EdgeFiltMaxCap[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoEdgeFiltMaxCap[i];
            stSharpenAttr.stAuto.au8RGain[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoRGain[i];
            stSharpenAttr.stAuto.au8GGain[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoGGain[i];
            stSharpenAttr.stAuto.au8BGain[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoBGain[i];
            stSharpenAttr.stAuto.au8SkinGain[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoSkinGain[i];
            stSharpenAttr.stAuto.au16MaxSharpGain[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoMaxSharpGain[i];
        }

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            stSharpenAttr.stAuto.au8OverShoot[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.au8OverShoot[i];
            stSharpenAttr.stAuto.au8UnderShoot[i] = stISPCfgPipeParam[ViPipe].stSharpenParam.au8UnderShoot[i];
        }

        for (i = 0; i < ISP_SHARPEN_LUMA_NUM; i++)
        {
            for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
            {
                stSharpenAttr.stAuto.au8LumaWgt[i][j] = stISPCfgPipeParam[ViPipe].stSharpenParam.AutoLumaWgt[i][j];
            }
        }

        for(i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
        {
            for(j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
            {
                stSharpenAttr.stAuto.au16TextureStr[i][j] = stISPCfgPipeParam[ViPipe].stSharpenParam.au16TextureStr[i][j];
                stSharpenAttr.stAuto.au16EdgeStr[i][j] = stISPCfgPipeParam[ViPipe].stSharpenParam.au16EdgeStr[i][j];
            }
        }
    }

    prtMD("!!!!!!!!!!!%d!!%d!!!!!!!!!!!!!!!\n", stSharpenAttr.enOpType, stISPCfgPipeParam[ViPipe].stSharpenParam.enOpType);

    s32Ret = HI_MPI_ISP_SetIspSharpenAttr(ViPipe, &stSharpenAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetIspSharpenAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetDemosaic(VI_PIPE ViPipe)
{
    HI_U32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEMOSAIC_ATTR_S stDemosaicAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetDemosaicAttr(ViPipe, &stDemosaicAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDemosaicAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stDemosaicAttr.bEnable = stISPCfgPipeParam[ViPipe].stDemosaicParam.bEnable;
    stDemosaicAttr.enOpType = stISPCfgPipeParam[ViPipe].stDemosaicParam.enOpType;
    if(stDemosaicAttr.enOpType == OP_TYPE_AUTO)
    {
        prtMD("stDemosaicAttr.stAuto.au8DetailSmoothRange:");
        for(i = 0; i < 16; i++)
        {
            stDemosaicAttr.stAuto.au8NonDirStr[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoNonDirStr[i];
            stDemosaicAttr.stAuto.au8NonDirMFDetailEhcStr[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoNonDirMFDetailEhcStr[i];
            stDemosaicAttr.stAuto.au8NonDirHFDetailEhcStr[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoNonDirHFDetailEhcStr[i];
            stDemosaicAttr.stAuto.au8DetailSmoothRange[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u16AutoDetailSmoothRange[i];
            printf(" %d", stDemosaicAttr.stAuto.au8DetailSmoothRange[i]);

            stDemosaicAttr.stAuto.au8ColorNoiseThdF[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoColorNoiseThdF[i];
            stDemosaicAttr.stAuto.au8ColorNoiseStrF[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoColorNoiseStrF[i];
            stDemosaicAttr.stAuto.au8ColorNoiseThdY[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoColorNoiseThdY[i];
            stDemosaicAttr.stAuto.au8ColorNoiseStrY[i] = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8AutoColorNoiseStrY[i];
        }
        printf("\n");
    }
    else
    {
        stDemosaicAttr.stManual.u8NonDirStr = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8ManualNonDirStr;
        stDemosaicAttr.stManual.u8NonDirMFDetailEhcStr = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8ManualNonDirMFDetailEhcStr;
        stDemosaicAttr.stManual.u8NonDirHFDetailEhcStr = stISPCfgPipeParam[ViPipe].stDemosaicParam.u8ManualNonDirHFDetailEhcStr;
        stDemosaicAttr.stManual.u8DetailSmoothRange = stISPCfgPipeParam[ViPipe].stDemosaicParam.u16ManualDetailSmoothStr;
    }

    s32Ret = HI_MPI_ISP_SetDemosaicAttr(ViPipe, &stDemosaicAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetDemosaicAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetViNR(VI_PIPE viPipe, ISPCFG_VI_NR_PARAM_S *pNrxParam)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VI_PIPE_ATTR_S stPipeAttr = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pNrxParam == NULL))
    {
        prtMD("invalid input viPipe = %d, pNrxParam = %p\n", viPipe, pNrxParam);
        return -1;
    }

    /* 设置使能标志 */
    s32Ret = HI_MPI_VI_GetPipeAttr(viPipe, &stPipeAttr);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VI_GetPipeAttr error! s32Ret = %#x\n", s32Ret);
    }

    stPipeAttr.bNrEn = pNrxParam->bOpen;

    s32Ret = HI_MPI_VI_SetPipeAttr(viPipe, &stPipeAttr);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VI_SetPipeAttr error! s32Ret = %#x\n", s32Ret);
    }

    /* 设置降噪参数 */
    s32Ret = HI_MPI_VI_SetPipeNRXParam(viPipe, &pNrxParam->stNrParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MPI_VI_SetPipeNRXParam error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetVpssNR(VPSS_GRP VpssGrp, ISPCFG_VPSS_NR_PARAM_S *pVpssParam)
{
    int s32Ret = HI_SUCCESS;
    VPSS_GRP_ATTR_S stGrpAttr = {0};

    if (pVpssParam == NULL)
    {
        prtMD("invalid input pVpssParam = %p\n", pVpssParam);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_GetGrpAttr(VpssGrp, &stGrpAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_GetGrpAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stGrpAttr.bNrEn = pVpssParam->bOpen;
    s32Ret = HI_MPI_VPSS_SetGrpAttr(VpssGrp, &stGrpAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_SetGrpAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp, &pVpssParam->stNrParam);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_SetGrpNRXParam is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetGamma(VI_PIPE ViPipe)
{
    HI_U32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_GAMMA_ATTR_S stIspGammaAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetGammaAttr(ViPipe, &stIspGammaAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetGammaAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stIspGammaAttr.bEnable = stISPCfgPipeParam[ViPipe].stGammaParam.bEnable;
    for (i = 0; i < GAMMA_NODE_NUM; i++)
    {
        stIspGammaAttr.u16Table[i] = stISPCfgPipeParam[ViPipe].stGammaParam.u16Table[i];
    }

    stIspGammaAttr.enCurveType = stISPCfgPipeParam[ViPipe].stGammaParam.enCurveType;

    s32Ret = HI_MPI_ISP_SetGammaAttr(ViPipe, &stIspGammaAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetGammaAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetFSWDR(VI_PIPE ViPipe)
{
    HI_U32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_WDR_FS_ATTR_S stIspWDRFSAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetFSWDRAttr(ViPipe, &stIspWDRFSAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetFSWDRAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stIspWDRFSAttr.enWDRMergeMode = stISPCfgPipeParam[ViPipe].stWdrfsParam.enWDRMergeMode;

    if(stIspWDRFSAttr.enWDRMergeMode ==  0)
    {
        stIspWDRFSAttr.stWDRCombine.bMotionComp = stISPCfgPipeParam[ViPipe].stWdrfsParam.bCombineMotionComp;
        stIspWDRFSAttr.stWDRCombine.u16ShortThr = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16CombineShortThr;

        stIspWDRFSAttr.stWDRCombine.u16LongThr = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16CombineLongThr;
        stIspWDRFSAttr.stWDRCombine.bForceLong = stISPCfgPipeParam[ViPipe].stWdrfsParam.bCombineForceLong;
        stIspWDRFSAttr.stWDRCombine.u16ForceLongHigThr = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16CombineForceLongHigThr;
        stIspWDRFSAttr.stWDRCombine.u16ForceLongLowThr = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16CombineForceLongLowThr;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.bShortExpoChk = stISPCfgPipeParam[ViPipe].stWdrfsParam.bWDRMdtShortExpoChk;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u16ShortCheckThd = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16WDRMdtShortCheckThd;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.bMDRefFlicker = stISPCfgPipeParam[ViPipe].stWdrfsParam.bWDRMdtMDRefFlicker;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u8MdtStillThd = stISPCfgPipeParam[ViPipe].stWdrfsParam.u8WDRMdtMdtStillThd;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u8MdtLongBlend = stISPCfgPipeParam[ViPipe].stWdrfsParam.u8WDRMdtMdtLongBlend;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.enOpType = stISPCfgPipeParam[ViPipe].stWdrfsParam.u8WDRMdtOpType;

        for(i = 0 ; i < ISP_AUTO_ISO_STRENGTH_NUM;i++)
        {
            stIspWDRFSAttr.stWDRCombine.stWDRMdt.stAuto.au8MdThrHigGain[i] = stISPCfgPipeParam[ViPipe].stWdrfsParam.au8AutoMdThrHigGain[i];
            stIspWDRFSAttr.stWDRCombine.stWDRMdt.stAuto.au8MdThrLowGain[i] = stISPCfgPipeParam[ViPipe].stWdrfsParam.au8AutoMdThrLowGain[i];
        }

        for(i = 0;i < 4;i++)
        {
            stIspWDRFSAttr.stFusion.au16FusionThr[i] = stISPCfgPipeParam[ViPipe].stWdrfsParam.u16FusionFusionThr[i];
        }
    }

    s32Ret = HI_MPI_ISP_SetFSWDRAttr(ViPipe, &stIspWDRFSAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetFSWDRAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetNR(VI_PIPE ViPipe)
{
    HI_U32 i, j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_NR_ATTR_S stNRAttr = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetNRAttr(ViPipe, &stNRAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetNRAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stNRAttr.bEnable = stISPCfgPipeParam[ViPipe].stBnrParam.bEnable;
    stNRAttr.bNrLscEnable = stISPCfgPipeParam[ViPipe].stBnrParam.bNrLscEnable;
    stNRAttr.u8BnrLscMaxGain = stISPCfgPipeParam[ViPipe].stBnrParam.u8BnrMaxGain;
    stNRAttr.u16BnrLscCmpStrength = stISPCfgPipeParam[ViPipe].stBnrParam.u16BnrLscCmpStrength;
    stNRAttr.enOpType = stISPCfgPipeParam[ViPipe].stBnrParam.u8BnrOptype;

    if(stNRAttr.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0; i < ISP_BAYER_CHN_NUM;i++)
        {
            for(j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM;j++)
            {
                stNRAttr.stAuto.au8ChromaStr[i][j] = stISPCfgPipeParam[ViPipe].stBnrParam.au8ChromaStr[i][j];
                stNRAttr.stAuto.au16CoarseStr[i][j] = stISPCfgPipeParam[ViPipe].stBnrParam.au16CoarseStr[i][j];
            }
        }

        for(i = 0;i < ISP_AUTO_ISO_STRENGTH_NUM;i++)
        {
            stNRAttr.stAuto.au8FineStr[i] = stISPCfgPipeParam[ViPipe].stBnrParam.au8FineStr[i];
            stNRAttr.stAuto.au16CoringWgt[i] = stISPCfgPipeParam[ViPipe].stBnrParam.au16CoringWgt[i];
        }

        for(i = 0;i < 4;i++)
        {
            stNRAttr.stWdr.au8WDRFrameStr[i] = stISPCfgPipeParam[ViPipe].stBnrParam.au8WDRFrameStr[i];
        }
    }

    s32Ret = HI_MPI_ISP_SetNRAttr(ViPipe, &stNRAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetNRAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_GetParam(VI_PIPE viPipe, ISPCFG_PARAM_S *pIspParam)
{
    int s32Ret = HI_SUCCESS;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pIspParam == NULL))
    {
        prtMD("invalid input viPipe = %d or pIspParam = %p\n", viPipe, pIspParam);
        return -1;
    }

    /* 加载ini文件中参数 */
    s32Ret = ISPCFG_GetBypass(viPipe, &stISPCfgPipeParam[viPipe].stBypassParam);
    if(0 != s32Ret)
    {
        prtMD("ISPCFG_GetBypass error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetBlackValue(viPipe, &stISPCfgPipeParam[viPipe].stBlackParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetBlackValue error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetAE(viPipe, &stISPCfgPipeParam[viPipe].stAeParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetAE error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetAWB(viPipe, &stISPCfgPipeParam[viPipe].stAwbParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetAWB error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetWdrExposure(viPipe, &stISPCfgPipeParam[viPipe].stWdrAeParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetWdrExposure error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetSaturation(viPipe, &stISPCfgPipeParam[viPipe].stSaturationParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetSaturation error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetDRC(viPipe, &stISPCfgPipeParam[viPipe].stDrcParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetDRC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetCCM(viPipe, &stISPCfgPipeParam[viPipe].stCcmParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetCCM error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetDehaze(viPipe, &stISPCfgPipeParam[viPipe].stDehazeParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetDehaze error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    //曝光权重
    s32Ret = ISPCFG_GetStatistics(viPipe, &stISPCfgPipeParam[viPipe].stStatisticsParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStatistics error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetCSC(viPipe, &stISPCfgPipeParam[viPipe].stCscParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetCSC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetLDCI(viPipe, &stISPCfgPipeParam[viPipe].stLdciParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetLDCI error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetShading(viPipe, &stISPCfgPipeParam[viPipe].stShadingParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetShading error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetSharpen(viPipe, &stISPCfgPipeParam[viPipe].stSharpenParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetSharpen error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetDemosaic(viPipe, &stISPCfgPipeParam[viPipe].stDemosaicParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetDemosaic error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetViNR(viPipe, &stISPCfgPipeParam[viPipe].stViNrxParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetViNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetVpssNR(viPipe, &stISPCfgPipeParam[viPipe].stVpssNrxParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetVpssNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetFSWDR(viPipe, &stISPCfgPipeParam[viPipe].stWdrfsParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetFSWDR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetGamma(viPipe, &stISPCfgPipeParam[viPipe].stGammaParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetGamma error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetBNR(viPipe, &stISPCfgPipeParam[viPipe].stBnrParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetBNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

ret:
    memcpy(pIspParam, stISPCfgPipeParam + viPipe, sizeof(ISPCFG_PARAM_S));

    return s32Ret;
}

int ISPCFG_SetParam(VI_PIPE viPipe, ISPCFG_PARAM_S *pIspParam)    //增加宽动态、线性模式识别
{
    int s32Ret = HI_SUCCESS;

    if (viPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input viPipe = %d\n", viPipe);
        return -1;
    }

    /* 设置各个模块bypass参数 */
    s32Ret = ISPCFG_SetBypass(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetBypass error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 0-设置黑电平 */
    s32Ret = ISPCFG_SetBlackLevel(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetBlackLevel error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 1-设置自动曝光 */
    s32Ret = ISPCFG_SetAE(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetAE error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 2-设置自动白平衡 */
    s32Ret = ISPCFG_SetAWB(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetAWB error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 3-设置wdr宽动态 */
    s32Ret = ISPCFG_SetWdrAE(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetWdrAE error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 4-设置色彩色温 */
    s32Ret = ISPCFG_SetSaturation(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetSaturation error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 5-设置DRC */
    s32Ret = ISPCFG_SetDRC(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetDRC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 6-设置CCM */
    s32Ret = ISPCFG_SetCCM(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetCCM error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 7-设置Dehaze */
    s32Ret = ISPCFG_SetDehaze(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetDehaze error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 8-设置曝光权重 */
    s32Ret = ISPCFG_SetStatistics(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetStatistics error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 9-设置CSC参数 */
    s32Ret = ISPCFG_SetCSC(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetCSC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 10-设置shading */
    s32Ret = ISPCFG_SetMeshShading(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetMeshShading error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 11-设置sharpen */
    s32Ret = ISPCFG_SetSharpen(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetSharpen error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 12-设置Demosaic */
    s32Ret = ISPCFG_SetDemosaic(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetDemosaic error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 13-设置gamma */
    s32Ret = ISPCFG_SetGamma(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetGamma error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

#if 0
    s32Ret = ISPCFG_SetViNR(viPipe, &stISPCfgPipeParam[viPipe].stViNrxParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetViNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_SetVpssNR(viPipe, &stISPCfgPipeParam[viPipe].stVpssNrxParam);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetVPssNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }
#endif

    /* 14-设置WDRFS */
    s32Ret = ISPCFG_SetFSWDR(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetFSWDR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 15-设置BNR */
    s32Ret = ISPCFG_SetNR(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

ret:
    prtMD("s32Ret = %#x\n", s32Ret);

    return s32Ret;
}

