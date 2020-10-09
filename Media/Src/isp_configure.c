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

int ISPCFG_SceneMode = 0;            //0:正常模式, 1:暗态模式

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
unsigned int ISPCFG_GetValue(const VI_PIPE viPipe, const char *pString)
{
    unsigned int keyvalue = 0;

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

////////
////////加载isp参数
////////
static  HI_U32 ISPCFG_GetThreshValue(HI_U32 u32Value, HI_U32 u32Count, HI_U32* pThresh)
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

static int ISPCFG_GetBypassParam(VI_PIPE viPipe, ISPCFG_BYPASS_PARAM_S *pBypassParam)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pBypassParam == NULL))
    {
        prtMD("invalid input viPipe = %d, pBypassParam = %p\n", viPipe, pBypassParam);
        return -1;
    }

    /* bitBypassISPDGain */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassISPDGain");
    pBypassParam->bitBypassISPDGain = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassISPDGain = %d\n", pBypassParam->bitBypassISPDGain);

    /* bitBypassAntiFC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassAntiFC");
    pBypassParam->bitBypassAntiFC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAntiFC = %d\n", pBypassParam->bitBypassAntiFC);

    /* bitBypassCrosstalkR */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassCrosstalkR");
    pBypassParam->bitBypassCrosstalkR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCrosstalkR = %d\n", pBypassParam->bitBypassCrosstalkR);

    /* bitBypassDPC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassDPC");
    pBypassParam->bitBypassDPC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDPC = %d\n", pBypassParam->bitBypassDPC);

    /* bitBypassNR */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassNR");
    pBypassParam->bitBypassNR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassNR = %d\n", pBypassParam->bitBypassNR);

    /* bitBypassDehaze */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassDehaze");
    pBypassParam->bitBypassDehaze = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDehaze = %d\n", pBypassParam->bitBypassDehaze);

    /* bitBypassWBGain */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassWBGain");
    pBypassParam->bitBypassWBGain = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassWBGain = %d\n", pBypassParam->bitBypassWBGain);

    /* bitBypassMeshShading */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassMeshShading");
    pBypassParam->bitBypassMeshShading = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassMeshShading = %d\n", pBypassParam->bitBypassMeshShading);

    /* bitBypassDRC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassDRC");
    pBypassParam->bitBypassDRC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDRC = %d\n", pBypassParam->bitBypassDRC);

    /* bitBypassDemosaic */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassDemosaic");
    pBypassParam->bitBypassDemosaic = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDemosaic = %d\n", pBypassParam->bitBypassDemosaic);

    /* bitBypassColorMatrix */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassColorMatrix");
    pBypassParam->bitBypassColorMatrix = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassColorMatrix = %d\n", pBypassParam->bitBypassColorMatrix);

    /* bitBypassGamma */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassGamma");
    pBypassParam->bitBypassGamma = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassGamma = %d\n", pBypassParam->bitBypassGamma);

    /* bitBypassFSWDR */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassFSWDR");
    pBypassParam->bitBypassFSWDR = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassFSWDR = %d\n", pBypassParam->bitBypassFSWDR);

    /* bitBypassCA */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassCA");
    pBypassParam->bitBypassCA = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCA = %d\n", pBypassParam->bitBypassCA);

    /* bitBypassCsConv */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassCsConv");
    pBypassParam->bitBypassCsConv = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCsConv = %d\n", pBypassParam->bitBypassCsConv);

    /* bitBypassSharpen */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassSharpen");
    pBypassParam->bitBypassSharpen = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassSharpen = %d\n", pBypassParam->bitBypassSharpen);

    /* bitBypassLCAC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassLCAC");
    pBypassParam->bitBypassLCAC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassLCAC = %d\n", pBypassParam->bitBypassLCAC);

    /* bitBypassGCAC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassGCAC");
    pBypassParam->bitBypassGCAC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassGCAC = %d\n", pBypassParam->bitBypassGCAC);

    /* bit2ChnSelect */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bit2ChnSelect");
    pBypassParam->bit2ChnSelect = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bit2ChnSelect = %d\n", pBypassParam->bit2ChnSelect);

    /* bitBypassLdci */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassLdci");
    pBypassParam->bitBypassLdci = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassLdci = %d\n", pBypassParam->bitBypassLdci);

    /* bitBypassPreGamma */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassPreGamma");
    pBypassParam->bitBypassPreGamma = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassPreGamma = %d\n", pBypassParam->bitBypassPreGamma);

    /* bitBypassAEStatFE */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassAEStatFE");
    pBypassParam->bitBypassAEStatFE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAEStatFE = %d\n", pBypassParam->bitBypassAEStatFE);

    /* bitBypassAEStatBE */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassAEStatBE");
    pBypassParam->bitBypassAEStatBE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAEStatBE = %d\n", pBypassParam->bitBypassAEStatBE);

    /* bitBypassMGSta */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassMGSta");
    pBypassParam->bitBypassMGSta = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassMGSta = %d\n", pBypassParam->bitBypassMGSta);

    /* bitBypassDE */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassDE");
    pBypassParam->bitBypassDE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassDE = %d\n", pBypassParam->bitBypassDE);

    /* bitBypassAFStatBE */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassAFStatBE");
    pBypassParam->bitBypassAFStatBE = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAFStatBE = %d\n", pBypassParam->bitBypassAFStatBE);

    /* bitBypassAWBStat */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassAWBStat");
    pBypassParam->bitBypassAWBStat = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassAWBStat = %d\n", pBypassParam->bitBypassAWBStat);

    /* bitBypassCLUT */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassCLUT");
    pBypassParam->bitBypassCLUT = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassCLUT = %d\n", pBypassParam->bitBypassCLUT);

    /* bitBypassHLC */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassHLC");
    pBypassParam->bitBypassHLC = (HI_BOOL)u32Value;
    prtMD("pBypassParam->bitBypassHLC = %d\n", pBypassParam->bitBypassHLC);

    /* bitBypassEdgeMark */
    u32Value = ISPCFG_GetValue(viPipe, "module_state:bitBypassEdgeMark");
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
    u32Value = ISPCFG_GetValue(viPipe, "static_blacklevel:enOptype");
    pBlackParam->enOpType = (HI_S32)u32Value;

    /* AutoStaticWb 包含了4个参数 */
    ret = ISPCFG_GetString(viPipe, &pString,"static_blacklevel:au16BlackLevel");
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
static int ISPCFG_GetStaticAe(VI_PIPE viPipe, ISPCFG_STATIC_AE_S *pStaticAe)
{
    unsigned int u32Value = 0;
    int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticAe == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticAe = %p\n", viPipe, pStaticAe);
        return -1;
    }

    /* AERouteExValid */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AERouteExValid");
    pStaticAe->bAERouteExValid = (HI_BOOL)u32Value;

    /* ExposureOpType */  //2019-08-29 add
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:ExposureOpType");
    pStaticAe->ExposureOpType = (HI_BOOL)u32Value;

    /* AERunInterval */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AERunInterval");
    pStaticAe->u8AERunInterval = (HI_U8)u32Value;
    //prtMD("1-------u32Value = %d\n",pStaticAe->u8AERunInterval);

    /* AutoExpTimeMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoExpTimeMax");
    pStaticAe->u32AutoExpTimeMax = u32Value;

    /* AutoExpTimeMix */  //2019-08-29 add
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoExpTimeMix");
    pStaticAe->u32AutoExpTimeMin = u32Value;

    prtMD("[AE] valid:%d, type:%d, RunInterval:%d, MaxExpTime:%d, MinExpTime:%d\n",
        pStaticAe->bAERouteExValid,
        pStaticAe->ExposureOpType,
        pStaticAe->u8AERunInterval,
        pStaticAe->u32AutoExpTimeMax,
        pStaticAe->u32AutoExpTimeMin);

    /* AutoAGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoAGainMax");
    pStaticAe->u32AutoAGainMax = u32Value;

    /* AutoAGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoAGainMix");
    pStaticAe->u32AutoAGainMin = u32Value;

    /* AutoDGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoDGainMax");
    pStaticAe->u32AutoDGainMax = u32Value;

    /* AutoDGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoDGainMix");
    pStaticAe->u32AutoDGainMin = u32Value;

    /* AutoISPDGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoISPDGainMax");
    pStaticAe->u32AutoISPDGainMax = u32Value;

    /* AutoISPDGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoISPDGainMix");
    pStaticAe->u32AutoISPDGainMin = u32Value;

    /* AutoSysGainMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoSysGainMax");
    pStaticAe->u32AutoSysGainMax = u32Value;

    /* AutoSysGainMix */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoSysGainMix");
    pStaticAe->u32AutoSysGainMin = u32Value;

    /* AutoGainThreshold */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoGainThreshold");
    pStaticAe->u32AutoGainThreshold = u32Value;

    /* AutoSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoSpeed");
    pStaticAe->u8AutoSpeed = (HI_U8)u32Value;

    /* AutoBlackSpeedBias */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoBlackSpeedBias");
    pStaticAe->u16AutoBlackSpeedBias = (HI_U16)u32Value;

    /* AutoTolerance */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoTolerance");
    pStaticAe->u8AutoTolerance = (HI_U8)u32Value;

    /* AutostaticCompesation */
    ISPCFG_GetString(viPipe, &pString,"static_ae:AutostaticCompesation");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticAe->u8AutostaticCompesation[u32IdxM] = (HI_U16)u32Value;
        //prtMD("pStaticAe->u8AutostaticCompesation[u32IdxM] = %d\n",pStaticAe->u8AutostaticCompesation[u32IdxM]);
    }

    /* AutoEVBias */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoEVBias");
    pStaticAe->u16AutoEVBias = (HI_U16)u32Value;

    /* AutoAEStrategMode */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoAEStrategMode");
    pStaticAe->bAutoAEStrategMode = (HI_BOOL)u32Value;

    /* AutoHistRatioSlope */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoHistRatioSlope");
    pStaticAe->u16AutoHistRatioSlope = (HI_U16)u32Value;

    /* AutoMaxHistOffset */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoMaxHistOffset");
    pStaticAe->u8AutoMaxHistOffset = (HI_U8)u32Value;

    /* AutoBlackDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoBlackDelayFrame");
    pStaticAe->u16AutoBlackDelayFrame = (HI_U16)u32Value;

    /* AutoWhiteDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoWhiteDelayFrame");
    pStaticAe->u16AutoWhiteDelayFrame = (HI_U16)u32Value;

#if 0
    /* AutoFswdrmode */
    u32Value = ISPCFG_GetValue(viPipe, "static_ae:AutoFswdrmode");
    pStaticAe->enFSWDRMode = (HI_U16)u32Value;
#endif

    return 0;
}

static int ISPCFG_GetStaticAwb(VI_PIPE viPipe, ISPCFG_STATIC_AWB_S *pStaticAwb)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char *pString = NULL;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticAwb == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticAwb = %p\n", viPipe, pStaticAwb);
        return -1;
    }

    /* AWBOpType */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AWBOpType");
    pStaticAwb->AWBOpType = (HI_U8)u32Value;

    /* AWBEnable */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AWBEnable");
    pStaticAwb->AWBEnable = (HI_BOOL)u32Value;

    /* AutoWhiteDelayFrame */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoRefColorTemp");
    pStaticAwb->u16AutoRefColorTemp = (HI_U16)u32Value;

    /* AutoStaticWb 包含了4个参数 */
    ISPCFG_GetString(viPipe, &pString,"static_awb:AutoStaticWb");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticAwb->au16AutoStaticWB[u32IdxM] = (HI_U16)u32Value;
    }

    /* AutoCurvePara */
    ISPCFG_GetString(viPipe, &pString,"static_awb:AutoCurvePara");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 6; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticAwb->as32AutoCurvePara[u32IdxM] = (HI_S32)u32Value;
    }

    /* AutoRGStrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoRGStrength");
    pStaticAwb->u8AutoRGStrength = (HI_U8)u32Value;

    /* AutoBGStrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoBGStrength");
    pStaticAwb->u8AutoBGStrength = (HI_U8)u32Value;

    /* AutoAWBSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoAWBSpeed");
    pStaticAwb->u16AutoSpeed = (HI_U16)u32Value;

    /* AutoZoneSel */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoZoneSel");
    pStaticAwb->u16AutoZoneSel = (HI_U16)u32Value;

    /* AutoLowColorTemp */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoLowColorTemp");
    pStaticAwb->u16AutoLowColorTemp = (HI_U16)u32Value;

    /* AutoHightColorTemp */
    u32Value = ISPCFG_GetValue(viPipe, "static_awb:AutoHighColorTemp");
    pStaticAwb->u16AutoHighColorTemp = (HI_U16)u32Value;

    return 0;
}

static int ISPCFG_GetWdrExposure(VI_PIPE viPipe, ISPCFG_STATIC_WDR_AE_S *pStaticWdrExposure)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:ExpRatioType");
    pStaticWdrExposure->u8ExpRatioType = (HI_U8)u32Value;

    /* ExpRatioMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:ExpRatioMax");
    pStaticWdrExposure->u32ExpRatioMax = (HI_U32)u32Value;

    /* ExpRatioMin */
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:ExpRatioMin");
    pStaticWdrExposure->u32ExpRatioMin = (HI_U32)u32Value;

    /* Tolerance */
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:Tolerance");
    pStaticWdrExposure->u16Tolerance = (HI_U16)u32Value;

    /* WDRSpeed */
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:WDRSpeed");
    pStaticWdrExposure->u16Speed = (HI_U16)u32Value;

    /* RatioBias */
    u32Value = ISPCFG_GetValue(viPipe, "static_wdrexposure:RatioBias");
    pStaticWdrExposure->u16RatioBias = (HI_U16)u32Value;

    /* ExpRatio */
    ISPCFG_GetString(viPipe, &pString, "static_wdrexposure:ExpRatio");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 3; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWdrExposure->au32ExpRatio[u32IdxM] = (HI_U32)u32Value;
    }

    return 0;
}

static int ISPCFG_GetStaticSaturation(VI_PIPE viPipe, ISPCFG_STATIC_SATURATION_S *pStaticSaturation)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_saturation:OpType");
    pStaticSaturation->bOpType = (HI_BOOL)u32Value;

    /* ManualSat */
    u32Value = ISPCFG_GetValue(viPipe, "static_saturation:ManualSat");
    pStaticSaturation->u8ManualSat = (HI_U8)u32Value;

    /* AutoSat */
    s32Ret = ISPCFG_GetString(viPipe, &pString, "static_saturation:AutoSat");
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

static int ISPCFG_GetStaticDRC(VI_PIPE viPipe, ISPCFG_STATIC_DRC_S *pStaticDrc)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticDrc == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticDrc = %p\n", viPipe, pStaticDrc);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:bEnable");
    pStaticDrc->bEnable = (HI_BOOL)u32Value;

    /* CurveSelect */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:CurveSelect");
    pStaticDrc->u8CurveSelect = (HI_U8)u32Value;

    /* PDStrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:PDStrength");
    pStaticDrc->u8PDStrength = (HI_U8)u32Value;

    /* LocalMixingBrightMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingBrightMax");
    pStaticDrc->u8LocalMixingBrightMax = (HI_U8)u32Value;

    /* LocalMixingBrightMin */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingBrightMin");
    pStaticDrc->u8LocalMixingBrightMin = (HI_U8)u32Value;

    /* LocalMixingBrightThr */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingBrightThr");
    pStaticDrc->u8LocalMixingBrightThr = (HI_U8)u32Value;

    /* LocalMixingBrightSlo */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingBrightSlo");
    pStaticDrc->s8LocalMixingBrightSlo = (HI_S8)u32Value;

    /* LocalMixingDarkMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingDarkMax");
    pStaticDrc->u8LocalMixingDarkMax = (HI_U8)u32Value;

    /* LocalMixingDarkMin */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingDarkMin");
    pStaticDrc->u8LocalMixingDarkMin = (HI_U8)u32Value;

    /* LocalMixingDarkThr */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingDarkThr");
    pStaticDrc->u8LocalMixingDarkThr = (HI_U8)u32Value;

    /* LocalMixingDarkSlo */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:LocalMixingDarkSlo");
    pStaticDrc->s8LocalMixingDarkSlo = (HI_S8)u32Value;

    /* BrightGainLmt */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:BrightGainLmt");
    pStaticDrc->u8BrightGainLmt = (HI_U8)u32Value;

    /* BrightGainLmtStep */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:BrightGainLmtStep");
    pStaticDrc->u8BrightGainLmtStep = (HI_BOOL)u32Value;

    /* DarkGainLmtY */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:DarkGainLmtY");
    pStaticDrc->u8DarkGainLmtY = (HI_U8)u32Value;

    /* DarkGainLmtC */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:DarkGainLmtC");
    pStaticDrc->u8DarkGainLmtC = (HI_U8)u32Value;

    /* ContrastControl */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:ContrastControl");
    pStaticDrc->u8ContrastControl = (HI_U8)u32Value;

    /* DetailAdjustFactor */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:DetailAdjustFactor");
    pStaticDrc->s8DetailAdjustFactor = (HI_S8)u32Value;

    /* SpatialFltCoef */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:SpatialFltCoef");
    pStaticDrc->u8SpatialFltCoef = (HI_U8)u32Value;

    /* RangeFltCoef */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:RangeFltCoef");
    pStaticDrc->u8RangeFltCoef = (HI_U8)u32Value;

    /* RangeAdaMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:RangeAdaMax");
    pStaticDrc->u8RangeAdaMax = (HI_U8)u32Value;

    /* GradRevMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:GradRevMax");
    pStaticDrc->u8GradRevMax = (HI_U8)u32Value;

    /* GradRevThr */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:GradRevThr");
    pStaticDrc->u8GradRevThr = (HI_U8)u32Value;

    /* DRCOpType */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:DRCOpType");
    pStaticDrc->u8DRCOpType = (HI_U8)u32Value;

    /* ManualStrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:ManualStrength");
    pStaticDrc->u16ManualStrength = (HI_U16)u32Value;

    /* AutoStrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:AutoStrength");
    pStaticDrc->u16AutoStrength = (HI_U16)u32Value;

    /* AutoStrengthMax */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:AutoStrengthMax");
    pStaticDrc->u16AutoStrengthMax = (HI_U16)u32Value;

    /* AutoStrengthMin */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:AutoStrengthMin");
    pStaticDrc->u16AutoStrengthMin = (HI_U16)u32Value;

    /* Asy_u8Asymmetry */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:Asy_u8Asymmetry");
    pStaticDrc->u8Asymmetry = (HI_U16)u32Value;

    /* Asy_u8SecondPole */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:Asy_u8SecondPole");
    pStaticDrc->u8SecondPole = (HI_U16)u32Value;

    /* Asy_u8Stretch */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:Asy_u8Stretch");
    pStaticDrc->u8Stretch = (HI_U16)u32Value;

    /* Asy_u8Compress */
    u32Value = ISPCFG_GetValue(viPipe, "static_drc:Asy_u8Compress");
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

static int ISPCFG_GetStaticCCM(VI_PIPE viPipe, ISPCFG_STATIC_CCM_S *pStaticCCM)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_ccm:CcmopType");
    pStaticCCM->CcmOpType = (HI_U8)u32Value;

    /* AutoISOActEn */
    u32Value = ISPCFG_GetValue(viPipe, "static_ccm:AutoISOActEn");
    pStaticCCM->bISOActEn = (HI_BOOL)u32Value;

    /* AutoTempActEn */
    u32Value = ISPCFG_GetValue(viPipe, "static_ccm:AutoTempActEn");
    pStaticCCM->bTempActEn = (HI_BOOL)u32Value;

    /* AutoCCMTabNum */
    u32Value = ISPCFG_GetValue(viPipe, "static_ccm:AutoCCMTabNum");
    pStaticCCM->u16CCMTabNum = (HI_U16)u32Value;

    //加载不同色温下的色温值
    for(i = 0;i < pStaticCCM->u16CCMTabNum;i++)
    {
        /* AutoCCMTabNum */
        snprintf(astloadpara, 128, "static_ccm:CCMTabTemp%d", i);
        u32Value = ISPCFG_GetValue(viPipe, astloadpara);
        pStaticCCM->astCCMTab[i].u16ColorTemp = (HI_U16)u32Value;

        /*AutoBlcCtrl*/
        snprintf(astloadpara, 128, "static_ccm:CCMTab%d", i);
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

static int ISPCFG_GetStaticDehaze(VI_PIPE viPipe, ISPCFG_STATIC_DEHAZE_S *pStaticDeHaze)
{
    unsigned int u32Value;
    unsigned int u32IdxM = 0,i = 0;
    char *pString = NULL;
    char astloadpara[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticDeHaze == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticDeHaze = %p\n", viPipe, pStaticDeHaze);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:bEnable");
    pStaticDeHaze->bEnable = (HI_BOOL)u32Value;

    /* bDehazeUserLutEnable */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:bDehazeUserLutEnable");
    pStaticDeHaze->bUserLutEnable = (HI_BOOL)u32Value;

    /* DehazeOpType */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:DehazeOpType");
    pStaticDeHaze->u8DehazeOpType = (HI_U8)u32Value;

    /* Manualstrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:Manualstrength");
    pStaticDeHaze->u8ManualStrength = (HI_U8)u32Value;

    /* Autostrength */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:Autostrength");
    pStaticDeHaze->u8Autostrength = (HI_U8)u32Value;

    /* TmprfltIncrCoef */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:TmprfltIncrCoef");
    pStaticDeHaze->u16prfltIncrCoef = (HI_U16)u32Value;

    /* TmprfltDecrCoef */
    u32Value = ISPCFG_GetValue(viPipe, "static_dehaze:TmprfltDecrCoef");
    pStaticDeHaze->u16prfltDecrCoef = (HI_U16)u32Value;

    /* DehazeLut0 */
    for(i = 0;i < 16;i++)
    {
        snprintf(astloadpara, 128, "static_dehaze:DehazeLut%d", i);
        ISPCFG_GetString(viPipe, &pString, astloadpara);
        ISPCFG_GetNumInLine(pString);

        for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
        {
            u32Value = ISPCFG_LineValue[u32IdxM];
            pStaticDeHaze->au8DehazeLut[i * 16 + u32IdxM] = ISPCFG_LineValue[u32IdxM];
        }
    }

    return 0;
}

//曝光权重
static int ISPCFG_GetStaticStatistics(VI_PIPE viPipe, ISPCFG_STATIC_STATISTICSCFG_S *pStaticStatistics)
{
    unsigned int u32IdxM = 0;
    unsigned int u32IdxN = 0;
    char *pString = NULL;
    char aszIniNodeName[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticStatistics == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticStatistics = %p\n", viPipe, pStaticStatistics);
        return -1;
    }

    /*AEWeight*/
    for(u32IdxM = 0; u32IdxM < 15; u32IdxM++)
    {
        snprintf(aszIniNodeName, 128, "static_statistics:ExpWeight_%d", u32IdxM);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for (u32IdxN = 0; u32IdxN < 17; u32IdxN++)
        {
            pStaticStatistics->au8AEWeight[u32IdxM][u32IdxN] = (HI_U8)ISPCFG_LineValue[u32IdxN];
        }
    }

    return 0;
}

static int ISPCFG_GetStaticCSC(VI_PIPE viPipe, ISPCFG_STATIC_CSC_S *pStaticCSC)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticCSC))
    {
        prtMD("invalid input viPipe = %d or pStaticCSC = %p\n", viPipe, pStaticCSC);
        return -1;
    }

    /* bEnable */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscbEnable");
    pStaticCSC->bEnable = (HI_BOOL)u32Value;

    /*Hue 色调 */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscHue");
    pStaticCSC->u8Hue = (HI_U8)u32Value;

    /*Luma 亮度 */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscLuma");
    pStaticCSC->u8Luma = (HI_U8)u32Value;

    /*Contrast 对比度 */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscContrast");
    pStaticCSC->u8Contr = (HI_U8)u32Value;

    /*Saturation 饱和度 */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscSaturation");
    pStaticCSC->u8Satu = (HI_U8)u32Value;

    /*ColorGamut 色域属性 */
    u32Value = ISPCFG_GetValue(viPipe, "static_csc:CscColorGamut");
    pStaticCSC->enColorGamut = (HI_U8)u32Value;

    return 0;
}

static int ISPCFG_GetStaticLDCI(VI_PIPE viPipe, ISPCFG_STATIC_LDCI_S *pStaticLdci)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_ldci:bEnable");
    pStaticLdci->bEnable = (HI_BOOL)u32Value;

    /*LDCIOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "static_ldci:LDCIOpType");
    pStaticLdci->u8LDCIOpType = (HI_U8)u32Value;

    /*ManualBlcCtrl*/
    u32Value = ISPCFG_GetValue(viPipe, "static_ldci:ManualBlcCtrl");
    pStaticLdci->u16ManualBlcCtrl = (HI_U8)u32Value;

    /*AutoBlcCtrl*/
    ISPCFG_GetString(viPipe, &pString, "static_ldci:AutoBlcCtrl");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticLdci->u16AutoBlcCtrl[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    return 0;
}

//13 -shading
static int ISPCFG_GetStaticShading(VI_PIPE viPipe, ISPCFG_STATIC_SHADING_S *pStaticShading)
{
    unsigned int u32Value = 0;

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticShading))
    {
        prtMD("invalid input viPipe = %d, pStaticShading = %p\n", viPipe, pStaticShading);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "static_shading:bEnable");
    pStaticShading->bEnable = (HI_BOOL)u32Value;

    return 0;
}

static int ISPCFG_GetStaticSharpen(VI_PIPE viPipe, ISPCFG_STATIC_SHARPEN_S *pStaticSharpen)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    unsigned int u32IdxN = 0;
    char * pString = NULL;
    char aszIniNodeName[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pStaticSharpen))
    {
        prtMD("invalid input viPipe = %d, pStaticSharpen = %p\n", viPipe, pStaticSharpen);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:bEnable");
    pStaticSharpen->bEnable = (HI_BOOL)u32Value;

    /* enOpType */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:enOpType");
    pStaticSharpen->enOpType = (int)u32Value;

    //////////////////////////////////////////////////////Manual
    /* ManualTextureFreq */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualTextureFreq");
    pStaticSharpen->ManualTextureFreq = (int)u32Value;

    /* ManualEdgeFreq */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualEdgeFreq");
    pStaticSharpen->ManualEdgeFreq = (int)u32Value;

    /* ManualOvershoot */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualOvershoot");
    pStaticSharpen->ManualOvershoot = (int)u32Value;

    /* ManualUnderShoot */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualUnderShoot");
    pStaticSharpen->ManualUnderShoot = (int)u32Value;

    /* ManualSupStr */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualSupStr");
    pStaticSharpen->ManualSupStr = (int)u32Value;

    /* ManualSupAdj */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualSupAdj");
    pStaticSharpen->ManualSupAdj = (int)u32Value;

    /* ManualDetailCtrl */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualDetailCtrl");
    pStaticSharpen->ManualDetailCtrl = (int)u32Value;

    /* ManualDetailCtrlThr */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualDetailCtrlThr");
    pStaticSharpen->ManualDetailCtrlThr = (int)u32Value;

    /* ManualEdgeFiltStr */
    u32Value = ISPCFG_GetValue(viPipe, "static_sharpen:ManualEdgeFiltStr");
    pStaticSharpen->ManualEdgeFiltStr = (int)u32Value;

    //////////////////////////////////////////////////////auto

    /* AutoLumaWgt */
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        snprintf(aszIniNodeName, 128, "static_sharpen:AutoLumaWgt_%d", u32IdxM);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for (u32IdxN = 0; u32IdxN < 32; u32IdxN++)
        {
            pStaticSharpen->AutoLumaWgt[u32IdxM][u32IdxN] = (HI_U16)ISPCFG_LineValue[u32IdxN];
            //prtMD("pStaticSharpen->au16TextureStr[%d][%d] = %d\n",u32IdxM,u32IdxN,pStaticSharpen->au16TextureStr[u32IdxM][u32IdxN]);
        }
    }

    /* AutoTextureFreq */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoTextureFreq");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoTextureFreq[u32IdxM] = ISPCFG_LineValue[u32IdxM];
        //prtMD("pStaticSharpen->AutoTextureFreq[%d] = %d\n",u32IdxM,pStaticSharpen->AutoTextureFreq[u32IdxM]);
    }

    /* AutoEdgeFreq */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoEdgeFreq");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoEdgeFreq[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoOvershoot */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoOvershoot");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->au8OverShoot[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoUnderShoot */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoUnderShoot");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->au8UnderShoot[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoSupStr */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoSupStr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoSupStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoSupAdj */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoSupAdj");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoSupAdj[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoDetailCtrl */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoDetailCtrl");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoDetailCtrl[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoDetailCtrlThr */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoDetailCtrlThr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoDetailCtrlThr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoEdgeFiltStr */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoEdgeFiltStr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoEdgeFiltStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoEdgeFiltMaxCap */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoEdgeFiltMaxCap");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoEdgeFiltMaxCap[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoRGain */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoRGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoRGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoGGain */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoGGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoGGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoBGain */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoBGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoBGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoSkinGain */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoSkinGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoSkinGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoMaxSharpGain */
    ISPCFG_GetString(viPipe, &pString, "static_sharpen:AutoMaxSharpGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticSharpen->AutoMaxSharpGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

#if 1

    /* TextureStrTable0 */
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        snprintf(aszIniNodeName, 128, "static_sharpen:TextureStrTable%d", u32IdxM);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for (u32IdxN = 0; u32IdxN < 32; u32IdxN++)
        {
            pStaticSharpen->au16TextureStr[u32IdxM][u32IdxN] = (HI_U16)ISPCFG_LineValue[u32IdxN];
            //prtMD("pStaticSharpen->au16TextureStr[%d][%d] = %d\n",u32IdxM,u32IdxN,pStaticSharpen->au16TextureStr[u32IdxM][u32IdxN]);
        }
    }

    /* EdgeStrTable0 */
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        snprintf(aszIniNodeName, 128, "static_sharpen:EdgeStrTable%d", u32IdxM);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for (u32IdxN = 0; u32IdxN < 32; u32IdxN++)
        {
            pStaticSharpen->au16EdgeStr[u32IdxM][u32IdxN] = (HI_U16)ISPCFG_LineValue[u32IdxN];
            //prtMD("pStaticSharpen->au16EdgeStr[%d][%d] = %d\n",u32IdxM,u32IdxN,pStaticSharpen->au16EdgeStr[u32IdxM][u32IdxN]);
        }
    }

#endif

    return 0;
}

static int ISPCFG_GetStaticDemosaic(VI_PIPE viPipe, ISPCFG_STATIC_DEMOSAIC_S *pStaticDemosaic)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:bEnable");
    pStaticDemosaic->bEnable = (HI_BOOL)u32Value;

    /*OpType*/
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:OpType");
    pStaticDemosaic->enOpType = (HI_S32)u32Value;

    /* 手动模式 */
    /*ManualNonDirStr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:ManualNonDirStr");
    pStaticDemosaic->u8ManualNonDirStr = (HI_U8)u32Value;

    /*ManualNonDirMFDetailEhcStr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:ManualNonDirMFDetailEhcStr");
    pStaticDemosaic->u8ManualNonDirMFDetailEhcStr = (HI_U8)u32Value;

    /*ManualNonDirHFDetailEhcStr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:ManualNonDirHFDetailEhcStr");
    pStaticDemosaic->u8ManualNonDirHFDetailEhcStr = (HI_U8)u32Value;

    /*ManualDetailSmoothRange*/
    u32Value = ISPCFG_GetValue(viPipe, "static_demosaic:ManualDetailSmoothRange");
    pStaticDemosaic->u16ManualDetailSmoothStr = (HI_U16)u32Value;

    /* 自动模式 */
    /*AutoColorNoiseThdF*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoColorNoiseThdF");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseThdF[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseStrF*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoColorNoiseStrF");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseStrF[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseThdY*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoColorNoiseThdY");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseThdY[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoColorNoiseStrY*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoColorNoiseStrY");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoColorNoiseStrY[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirStr*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoNonDirStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirMFDetailEhcStr*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoNonDirMFDetailEhcStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirMFDetailEhcStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoNonDirHFDetailEhcStr*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoNonDirHFDetailEhcStr");
    ISPCFG_GetNumInLine(pString);
    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticDemosaic->u8AutoNonDirHFDetailEhcStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoDetailSmoothRange*/
    ISPCFG_GetString(viPipe, &pString, "static_demosaic:AutoDetailSmoothRange");
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

static int ISPCFG_GetDynamic3DNR(VI_PIPE viPipe, ISPCFG_DYNAMIC_THREEDNR_S *pDynamic3Dnr)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    char * pString = NULL;
    char aszIniNodeName[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (NULL == pDynamic3Dnr))
    {
        prtMD("invalid input viPipe = %d, pDynamic3DNR = %p\n", viPipe, pDynamic3Dnr);
        return -1;
    }

    /*ThreeDNRCount*/
    u32Value = ISPCFG_GetValue(viPipe, "dynamic_threednr:ThreeDNRCount");
    pDynamic3Dnr->u32ThreeDNRCount = (HI_U32)u32Value;

    /*VI_3DNRStartPoint*/
    u32Value = ISPCFG_GetValue(viPipe, "dynamic_threednr:VI_3DNRStartPoint");
    pDynamic3Dnr->u16VI_3DNRStartPoint = (HI_U32)u32Value;


    /* IsoThresh */
    ISPCFG_GetString(viPipe, &pString, "dynamic_threednr:IsoThresh");
    ISPCFG_GetNumInLine(pString);

    for (u32IdxM = 0; u32IdxM < pDynamic3Dnr->u32ThreeDNRCount; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pDynamic3Dnr->au32ThreeDNRIso[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* ISO value */
    for (u32IdxM = 0; u32IdxM < pDynamic3Dnr->u32ThreeDNRCount; u32IdxM++)
    {
        //初始化数据结构体
        ISPCFG_VI_3DNR_S *pVIX      = &(pDynamic3Dnr->astThreeDNRVIValue[u32IdxM]);
        ISPCFG_VPSS_3DNR_S *pVPX    = &(pDynamic3Dnr->astThreeDNRVPSSValue[u32IdxM]);

        ISPCFG_VI_3DNR_IE_S *pViI    = &(pVIX->IEy);
        ISPCFG_VI_3DNR_SF_S *pViS    = &(pVIX->SFy);
        ISPCFG_VPSS_3DNR_IE_S  *pVpI = pVPX->IEy;
        ISPCFG_VPSS_3DNR_SF_S  *pVpS = pVPX->SFy;
        ISPCFG_VPSS_3DNR_MD_S  *pVpM = pVPX->MDy;
        ISPCFG_VPSS_3DNR_TF_S  *pVpT = pVPX->TFy;
        ISPCFG_VPSS_3DNR_RF_S  *pVpR = &pVPX->RFs;
        ISPCFG_VPSS_3DNR_NRC_S  *pVpNRc = &pVPX->NRc;
        ISPCFG_3DNR_VPSS_PNRC_S *pVpPNRc = &pVPX->pNRc;

        //第1行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-en", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"          |             %3d |             %3d |             %3d",
                &pVpS[0].NRyEn, &pVpS[1].NRyEn, &pVpS[2].NRyEn);
            prtMD("[en]:  %d,  %d,  %d\n", pVpS[0].NRyEn, pVpS[1].NRyEn, pVpS[2].NRyEn);
        }

        //第2行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsf1", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d |    %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |    %3d:%3d:%3d ",
                             &pViS->SFS1, &pViS->SFT1, &pViS->SBR1,
                             &pVpS[0].SFS1, &pVpS[0].SFT1, &pVpS[0].SBR1,
                             &pVpS[1].SFS1, &pVpS[1].SFT1, &pVpS[1].SBR1,
                             &pVpS[2].SFS1, &pVpS[2].SFT1, &pVpS[2].SBR1,
                             &pVpNRc->SFy.SFS1,&pVpNRc->SFy.SFT1,&pVpNRc->SFy.SBR1);
            prtMD("[nXsf1]:  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d\n",
                pViS->SFS1, pViS->SFT1, pViS->SBR1,
                pVpS[0].SFS1, pVpS[0].SFT1, pVpS[0].SBR1,
                pVpS[1].SFS1, pVpS[1].SFT1, pVpS[1].SBR1,
                pVpS[2].SFS1, pVpS[2].SFT1, pVpS[2].SBR1);
        }

        //第3行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsf2", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d |    %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |    %3d:%3d:%3d ",
                              &pViS->SFS2, &pViS->SFT2, &pViS->SBR2, &pVpS[0].SFS2, &pVpS[0].SFT2, &pVpS[0].SBR2, &pVpS[1].SFS2, &pVpS[1].SFT2, &pVpS[1].SBR2, &pVpS[2].SFS2, &pVpS[2].SFT2, &pVpS[2].SBR2,&pVpNRc->SFy.SFS2,&pVpNRc->SFy.SFT2,&pVpNRc->SFy.SBR2);
        }

        //第4行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsf4", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d |    %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |    %3d:%3d:%3d ",
                              &pViS->SFS4, &pViS->SFT4, &pViS->SBR4, &pVpS[0].SFS4, &pVpS[0].SFT4, &pVpS[0].SBR4, &pVpS[1].SFS4, &pVpS[1].SFT4, &pVpS[1].SBR4, &pVpS[2].SFS4, &pVpS[2].SFT4, &pVpS[2].SBR4,&pVpNRc->SFy.SFS4,&pVpNRc->SFy.SFT4,&pVpNRc->SFy.SBR4);
        }

        //第5行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-bwsf4", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d |            %3d |                 |                 |            %3d",
                             &pViS->BWSF4, &pVpS[0].BWSF4,&pVpNRc->SFy.BWSF4);
        }

        //第6行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-kmsf4", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |                 |             %3d |             %3d |                ",
                             &pVpS[1].kMode, &pVpS[2].kMode);
        }

        //第7行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsf5", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString," %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d",
                             &pViI->IES0, &pViI->IES1, &pViI->IES2, &pViI->IES3, &pVpI[0].IES0, &pVpI[0].IES1, &pVpI[0].IES2, &pVpI[0].IES3, &pVpI[1].IES0, &pVpI[1].IES1, &pVpI[1].IES2, &pVpI[1].IES3, &pVpI[2].IES0, &pVpI[2].IES1, &pVpI[2].IES2, &pVpI[2].IES3, &pVpNRc->IEy.IES0, &pVpNRc->IEy.IES1, &pVpNRc->IEy.IES2, &pVpNRc->IEy.IES3);
        }

        //第8行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-dzsf5", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"       %3d |             %3d |             %3d |             %4d |            %4d ",
                             &pViI->IEDZ, &pVpI[0].IEDZ, &pVpI[1].IEDZ, &pVpI[2].IEDZ, &pVpNRc->IEy.IEDZ);
        }

        //第9行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsf6", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d",
                             &pViS->SPN6, &pViS->SBN6, &pViS->PBR6, &pViS->JMODE, &pVpS[0].SPN6, &pVpS[0].SBN6, &pVpS[0].PBR6, &pVpS[0].JMODE, &pVpS[1].SPN6, &pVpS[1].SBN6, &pVpS[1].PBR6, &pVpS[1].JMODE, &pVpS[2].SPN6, &pVpS[2].SBN6, &pVpS[2].PBR6, &pVpS[2].JMODE,&pVpNRc->SFy.SPN6,&pVpNRc->SFy.SBN6,&pVpNRc->SFy.PBR6,&pVpNRc->SFy.JMODE);
        }

        //第10行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsfr6", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d |    %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d",
                             &pViS->SFR6[0], &pViS->SFR6[1], &pViS->SFR6[2], &pVpS[0].SFR6[0], &pVpS[0].SFR6[1], &pVpS[0].SFR6[2], &pVpS[1].SFR6[0], &pVpS[1].SFR6[1], &pVpS[1].SFR6[2], &pVpS[2].SFR6[0], &pVpS[2].SFR6[1], &pVpS[2].SFR6[2],&pVpNRc->SFy.SFR6[0],&pVpNRc->SFy.SFR6[1],&pVpNRc->SFy.SFR6[2]);
        }

        //第11行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-SelRt", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"   %3d:%3d |                 |                 |                 |         %3d:%3d ",
                            &pViS->SRT0, &pViS->SRT1, &pVpNRc->SFy.SRT0, &pVpNRc->SFy.SRT1);
        }

        //第12行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-DeRt", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"   %3d:%3d |                 |                 |                 |         %3d:%3d",
                            &pViS->DeRate, &pViS->DeIdx, &pVpNRc->SFy.DeRate, &pVpNRc->SFy.DeIdx);
        }

        //第13行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-TriTh", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"       %3d |             %3d |             %3d |             %3d |            %3d ",
                           &pViS->TriTh, &pVpS[0].TriTh, &pVpS[1].TriTh, &pVpS[2].TriTh, &pVpNRc->SFy.TriTh);
        }

        //第14行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsfn", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"%3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d | %3d:%3d:%3d:%3d",
                            &pViS->SFN0, &pViS->SFN1, &pViS->SFN3, &pViS->SFN3, &pVpS[0].SFN0, &pVpS[0].SFN1, &pVpS[0].SFN2, &pVpS[0].SFN3, &pVpS[1].SFN0, &pVpS[1].SFN1, &pVpS[1].SFN2, &pVpS[1].SFN3, &pVpS[2].SFN0, &pVpS[2].SFN1, &pVpS[2].SFN2, &pVpS[2].SFN3, &pVpNRc->SFy.SFN0, &pVpNRc->SFy.SFN1, &pVpNRc->SFy.SFN2, &pVpNRc->SFy.SFN3);
        }

        //第15行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsth", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString," %3d:%3d:%3d |       %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |    %3d:%3d:%3d",
                            &pViS->STH1, &pViS->STH2, &pViS->STH3, &pVpS[0].STH1, &pVpS[0].STH2, &pVpS[0].STH3, &pVpS[1].STH1, &pVpS[1].STH2, &pVpS[1].STH3, &pVpS[2].STH1, &pVpS[2].STH2, &pVpS[2].STH3, &pVpNRc->SFy.STH1, &pVpNRc->SFy.STH2, &pVpNRc->SFy.STH3);
        }

        //第16行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-sfr0", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString," %3d |               %3d |             %3d |             %3d |            %3d ",
                             &pViS->SFR, &pVpS[0].SFR, &pVpS[1].SFR, &pVpS[2].SFR, &pVpNRc->SFy.SFR);
        }

        //第17行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-tedge", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |             %3d |             %3d |                ",
                             &pVpT[0].tEdge, &pVpT[1].tEdge);
        }

        //第18行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-ref", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |             %3d |                 |                ",
                           &pVpT[0].bRef);
        }

        //第19行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-refUpt", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |             %3d |                 |                  ",
                             &pVpR[0].RFUI);
        }

        //第20行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-rftIdx", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |                 |                 ",
                              &pVpT[0].RFI,&pVpT[1].RFI);
        }

        //第21行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-refCtl", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |                 |                ",
                              &pVpR[0].RFDZ, &pVpR[0].RFSLP);
        }

        //第22行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-biPath", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |             %3d |             %3d |                 ",
                              &pVpM[0].biPath, &pVpM[1].biPath);
        }

        //第23行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXstr1", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |         %3d:%3d |                ",
                              &pVpT[0].STR0, &pVpT[0].STR1, &pVpT[1].STR0, &pVpT[1].STR1);
        }

        //第24行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXsdz", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"       |         %3d:%3d |         %3d:%3d |                  ",
                              &pVpT[0].SDZ0, &pVpT[0].SDZ1, &pVpT[1].SDZ0, &pVpT[1].SDZ1);
        }

        //第25行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtss", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].TSS0, &pVpT[0].TSS1, &pVpT[1].TSS0, &pVpT[1].TSS1);
        }


        //第26行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtsi", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"              |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].TSI0, &pVpT[0].TSI1, &pVpT[1].TSI0, &pVpT[1].TSI1);
        }

        //第27行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtfs3", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"            |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].TFS0, &pVpT[0].TFS1, &pVpT[1].TFS0, &pVpT[1].TFS1);
        }

        //第28行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXdzm", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"            |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].DZMode0, &pVpT[0].DZMode1, &pVpT[1].DZMode0, &pVpT[1].DZMode1);
        }

        //第29行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtdz", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"            |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].TDZ0, &pVpT[0].TDZ1, &pVpT[1].TDZ0, &pVpT[1].TDZ1);
        }

        //第30行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtdx", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"            |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpT[0].TDX0, &pVpT[0].TDX1, &pVpT[1].TDX0, &pVpT[1].TDX1);
            //prtMD("<%s>,%d ,%d,%d,%d\n",__FUNCTION__,pVpT[0].TDX0, pVpT[0].TDX1, pVpT[1].TDX0, pVpT[1].TDX1);
        }

        //第31行数据 //待定
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtfr0", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"  |%3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |  %3d:%3d:%3d | ",
                              &pVpT[0].TFR0[0], &pVpT[0].TFR0[1], &pVpT[0].TFR0[2], &pVpT[1].TFR0[0], &pVpT[1].TFR0[1], &pVpT[1].TFR0[2],&pVpT[0].TFR0[3], &pVpT[0].TFR0[4], &pVpT[0].TFR0[5], &pVpT[1].TFR0[3], &pVpT[1].TFR0[4], &pVpT[1].TFR0[5]);
        }

        //第32行数据 //待定
        snprintf(aszIniNodeName, 128, "iso_%d:-nXtfr1", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        //prtMD("<%s>,iso_%d\n",__FUNCTION__,((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString," | %3d:%3d:%3d |     %3d:%3d:%3d |     %3d:%3d:%3d |   %3d:%3d:%3d | ",
                              &pVpT[0].TFR1[0], &pVpT[0].TFR1[1], &pVpT[0].TFR1[2], &pVpT[1].TFR1[0], &pVpT[1].TFR1[1], &pVpT[1].TFR1[2],&pVpT[0].TFR1[3], &pVpT[0].TFR1[4], &pVpT[0].TFR1[5], &pVpT[1].TFR1[3], &pVpT[1].TFR1[4], &pVpT[1].TFR1[5]);

            //prtMD("%d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d\n", pVpT[0].TFR1[0], pVpT[0].TFR1[1], pVpT[0].TFR1[2], pVpT[1].TFR1[0], pVpT[1].TFR1[1], pVpT[1].TFR1[2],pVpT[0].TFR1[3], pVpT[0].TFR1[4], pVpT[0].TFR1[5], pVpT[1].TFR1[3], pVpT[1].TFR1[4], pVpT[1].TFR1[5]);
        }

        //第33行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXid0", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |     %3d:%3d:%3d |     %3d:%3d:%3d |                 ",
                              &pVpM[0].MAI00, &pVpM[0].MAI01, &pVpM[0].MAI02, &pVpM[1].MAI00, &pVpM[1].MAI01, &pVpM[1].MAI02);
        }

        //第34行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXid1", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |     %3d:%3d:%3d |     %3d:%3d:%3d |                 ",
                              &pVpM[0].MAI10, &pVpM[0].MAI11, &pVpM[0].MAI12, &pVpM[1].MAI10, &pVpM[1].MAI11, &pVpM[1].MAI12);
        }

        //第35行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmadz", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"             |         %3d:%3d |         %3d:%3d |               ",
                              &pVpM[0].MADZ0, &pVpM[0].MADZ1, &pVpM[1].MADZ0, &pVpM[1].MADZ1);
        }

        //第36行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmabr", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"             |         %3d:%3d |         %3d:%3d |               ",
                              &pVpM[0].MABR0, &pVpM[0].MABR1, &pVpM[1].MABR0, &pVpM[1].MABR1);
        }

        //第37行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-AdvMath", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d     |                 ",
                              &pVpR[0].advMATH);
        }

        //第38行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-sfc", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"  %3d  ",&pVpPNRc->SFC);
        }

        //第39行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmath", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"       |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpM[0].MATH0, &pVpM[0].MATH1, &pVpM[1].MATH0, &pVpM[1].MATH1);
        }

        //第40行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-ctfs", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString," %3d  ",&pVpPNRc->CTFS);
        }


        //第41行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-tfc", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"  %3d   ",&pVpPNRc->TFC);
        }

        //第42行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmate", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpM[0].MATE0, &pVpM[0].MATE1, &pVpM[1].MATE0, &pVpM[1].MATE1);
        }

        //第43行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmabw", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |         %3d:%3d |         %3d:%3d |                 ",
                              &pVpM[0].MABW0, &pVpM[0].MABW1, &pVpM[1].MABW0, &pVpM[1].MABW1);
        }

        //第44行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmatw", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"           |             %3d |             %3d |                ", &pVpM[0].MATW, &pVpM[1].MATW);
        }

        //第45行数据
        snprintf(aszIniNodeName, 128, "iso_%d:-mXmasw", ((HI_U32)pow(2.0,(float)u32IdxM)) * 100);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        if (pString != NULL)
        {
            sscanf(pString,"       |             %3d |             %3d |                ", &pVpM[0].MASW, &pVpM[1].MASW);
        }

    }

    //prtMD("ISPCFG_GetDynamic3DNRend -------------------------------\n");
    return 0;
}

int ISPCFG_GetStaticFSWDR(VI_PIPE viPipe, ISPCFG_STATIC_WDRFS_S *pStaticWDRFs)
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
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:MergeMode");
    pStaticWDRFs->enWDRMergeMode = (HI_U8)u32Value;

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:MotionComp");
    pStaticWDRFs->bCombineMotionComp = (HI_BOOL)u32Value;

    /*CombineShortThr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:CombineShortThr");
    pStaticWDRFs->u16CombineShortThr = (HI_U16)u32Value;
    //prtMD("pStaticWDRFs->u16CombineShortThr = %d\n",pStaticWDRFs->u16CombineShortThr);

    /*CombineLongThr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:CombineLongThr");
    pStaticWDRFs->u16CombineLongThr = (HI_U16)u32Value;
    //prtMD("pStaticWDRFs->u16CombineLongThr = %d\n",pStaticWDRFs->u16CombineLongThr);

    /*CombineForceLong*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:CombineForceLong");
    pStaticWDRFs->bCombineForceLong = (HI_BOOL)u32Value;

    /*CombineForceLongLowThr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:CombineForceLongLowThr");
    pStaticWDRFs->u16CombineForceLongLowThr = (HI_U16)u32Value;

    /*CombineForceLongHigThr*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:CombineForceLongHigThr");
    pStaticWDRFs->u16CombineForceLongHigThr = (HI_U16)u32Value;

    /*WDRMdtShortExpoChk*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtShortExpoChk");
    pStaticWDRFs->bWDRMdtShortExpoChk = (HI_U16)u32Value;

    /*WDRMdtShortCheckThd*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtShortCheckThd");
    pStaticWDRFs->u16WDRMdtShortCheckThd = (HI_U16)u32Value;

    /*WDRMdtMDRefFlicker*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtMDRefFlicker");
    pStaticWDRFs->bWDRMdtMDRefFlicker = (HI_BOOL)u32Value;

    /*WDRMdtMdtStillThd*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtMdtStillThd");
    pStaticWDRFs->u8WDRMdtMdtStillThd = (HI_U8)u32Value;

    /*WDRMdtMdtLongBlend*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtMdtLongBlend");
    pStaticWDRFs->u8WDRMdtMdtLongBlend = (HI_U8)u32Value;

    /*WDRMdtOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "static_fswdr:WDRMdtOpType");
    pStaticWDRFs->u8WDRMdtOpType = (HI_U8)u32Value;

    /*AutoMdThrLowGain*/
    ISPCFG_GetString(viPipe, &pString, "static_fswdr:AutoMdThrLowGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->au8AutoMdThrLowGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*AutoMdThrHigGain*/
    ISPCFG_GetString(viPipe, &pString, "static_fswdr:AutoMdThrHigGain");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->au8AutoMdThrHigGain[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /*FusionFusionThr*/
    ISPCFG_GetString(viPipe, &pString, "static_fswdr:FusionFusionThr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticWDRFs->u16FusionFusionThr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    return 0;
}

static int ISPCFG_GetDynamicGamma(VI_PIPE viPipe, ISPCFG_DYNAMIC_GAMMA_S *pDynamicGamma)
{
    int i = 0;
    int j = 0;
    unsigned int u32Value = 0;
    char *pString = NULL;
    char aszIniNodeName[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pDynamicGamma == NULL))
    {
        prtMD("invalid input viPipe = %d, pDynamicGanma = %p\n", viPipe, pDynamicGamma);
        return -1;
    }

    /*bEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "dynamic_gamma:bEnable");
    pDynamicGamma->bEnable = (int)u32Value;

    /*enCurveType*/
    u32Value = ISPCFG_GetValue(viPipe, "dynamic_gamma:enCurveType");
    pDynamicGamma->enCurveType = (ISP_GAMMA_CURVE_TYPE_E)u32Value;

    //add gamma
    for(i = 0;i < 25;i++)
    {
        snprintf(aszIniNodeName, 128, "dynamic_gamma:gammaTable%d", i);
        /* Table */
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for(j = 0;j < 41;j++)
        {
            u32Value = ISPCFG_LineValue[j];
            pDynamicGamma->u16Table[i][j] = ISPCFG_LineValue[j];
        }
    }

    return 0;
}

static int ISPCFG_GetStaticBNR(VI_PIPE viPipe, ISPCFG_STATIC_BNR_S *pStaticBnr)
{
    unsigned int u32Value = 0;
    unsigned int u32IdxM = 0;
    int i = 0;
    char * pString = NULL;
    char aszIniNodeName[128] = {0};

    if ((viPipe >= VI_MAX_PIPE_NUM) || (pStaticBnr == NULL))
    {
        prtMD("invalid input viPipe = %d or pStaticBnr = %p\n", viPipe, pStaticBnr);
        return -1;
    }

    /*BnrEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "static_BNR:BnrEnable");
    pStaticBnr->bEnable = (HI_BOOL)u32Value;

    /*NrLscEnable*/
    u32Value = ISPCFG_GetValue(viPipe, "static_BNR:NrLscEnable");
    pStaticBnr->bNrLscEnable = (HI_BOOL)u32Value;

    /*BnrLscMaxGain*/
    u32Value = ISPCFG_GetValue(viPipe, "static_BNR:BnrLscMaxGain");
    pStaticBnr->u8BnrMaxGain = (HI_U8)u32Value;

    /*BnrLscCmpStrength*/
    u32Value = ISPCFG_GetValue(viPipe, "static_BNR:BnrLscCmpStrength");
    pStaticBnr->u16BnrLscCmpStrength = (HI_U16)u32Value;

    /*BnrOpType*/
    u32Value = ISPCFG_GetValue(viPipe, "static_BNR:BnrOpType");
    pStaticBnr->u8BnrOptype = (HI_U8)u32Value;

    /* AutoChromaStr */
    for(i = 0;i < 4;i++)
    {
        snprintf(aszIniNodeName,128,"static_BNR:AutoChromaStr%d",i);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
        {
            u32Value = ISPCFG_LineValue[u32IdxM];
            pStaticBnr->au8ChromaStr[i][u32IdxM] = ISPCFG_LineValue[u32IdxM];
        }

        //prtMD("%s\n",aszIniNodeName);
    }

    /* AutoFineStr */
    ISPCFG_GetString(viPipe, &pString, "static_BNR:AutoFineStr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticBnr->au8FineStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoCoringWgt */
    ISPCFG_GetString(viPipe, &pString, "static_BNR:AutoCoringWgt");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticBnr->au16CoringWgt[u32IdxM] = ISPCFG_LineValue[u32IdxM];
    }

    /* AutoCoarseSt */
    for(i = 0;i < 4;i++)
    {
        snprintf(aszIniNodeName,128,"static_BNR:AutoCoarseSt%d",i);
        ISPCFG_GetString(viPipe, &pString, aszIniNodeName);
        ISPCFG_GetNumInLine(pString);

        for(u32IdxM = 0; u32IdxM < 16; u32IdxM++)
        {
            u32Value = ISPCFG_LineValue[u32IdxM];
            pStaticBnr->au16CoarseStr[i][u32IdxM] = ISPCFG_LineValue[u32IdxM];
        }
        //prtMD("%s\n",aszIniNodeName);
    }

    /* WDRFrameStr */
    ISPCFG_GetString(viPipe, &pString, "static_BNR:WDRFrameStr");
    ISPCFG_GetNumInLine(pString);

    for(u32IdxM = 0; u32IdxM < 4; u32IdxM++)
    {
        u32Value = ISPCFG_LineValue[u32IdxM];
        pStaticBnr->au8WDRFrameStr[u32IdxM] = ISPCFG_LineValue[u32IdxM];
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

    stModCtrl.bitBypassISPDGain =  stISPCfgPipeParam[ViPipe].stModuleState.bitBypassISPDGain;
    stModCtrl.bitBypassAntiFC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassAntiFC;
    stModCtrl.bitBypassCrosstalkR = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassCrosstalkR;

    stModCtrl.bitBypassDPC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassDPC;
    stModCtrl.bitBypassNR = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassNR;
    stModCtrl.bitBypassDehaze = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassDehaze;
    stModCtrl.bitBypassWBGain = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassWBGain;

    stModCtrl.bitBypassMeshShading = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassMeshShading;
    stModCtrl.bitBypassDRC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassDRC;
    stModCtrl.bitBypassDemosaic = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassDemosaic;
    stModCtrl.bitBypassColorMatrix = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassColorMatrix;
    stModCtrl.bitBypassGamma = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassGamma;
    stModCtrl.bitBypassFSWDR = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassFSWDR;

    stModCtrl.bitBypassCA = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassCA;
    stModCtrl.bitBypassCsConv = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassCsConv;
    stModCtrl.bitBypassSharpen = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassSharpen;
    stModCtrl.bitBypassLCAC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassLCAC;
    stModCtrl.bitBypassGCAC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassGCAC;

    stModCtrl.bit2ChnSelect = stISPCfgPipeParam[ViPipe].stModuleState.bit2ChnSelect;
    stModCtrl.bitBypassLdci = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassLdci;

    stModCtrl.bitBypassPreGamma = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassPreGamma;
    stModCtrl.bitBypassAEStatFE = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassAEStatFE;

    stModCtrl.bitBypassAEStatBE = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassAEStatBE;
    stModCtrl.bitBypassMGStat = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassMGSta;
    stModCtrl.bitBypassDE = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassDE;
    stModCtrl.bitBypassAFStatBE = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassAFStatBE;

    stModCtrl.bitBypassAWBStat = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassAWBStat;
    stModCtrl.bitBypassCLUT = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassCLUT;
    stModCtrl.bitBypassHLC = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassHLC;
    stModCtrl.bitBypassEdgeMark = stISPCfgPipeParam[ViPipe].stModuleState.bitBypassEdgeMark;

    prtMD("stModCtrl.bitBypassEdgeMark = %d\n", stModCtrl.bitBypassEdgeMark);
    s32Ret = HI_MPI_ISP_SetModuleControl(ViPipe, &stModCtrl);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetModuleControl is failed!\n");
    }

    return 0;
}

static int ISPCFG_SetStaticBlackLevel(VI_PIPE ViPipe)
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

    stBlackLevel.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticBlackLevel.enOpType;
    for(i = 0;i < 4;i++)
    {
        stBlackLevel.au16BlackLevel[i] = stISPCfgPipeParam[ViPipe].stStaticBlackLevel.au16BlackLevel[i];
    }

    s32Ret = HI_MPI_ISP_SetBlackLevelAttr(ViPipe, &stBlackLevel);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetBlackLevelAttr is failed!s32Ret = %#x\n",s32Ret);
    }

    return 0;
}

/* 设置自动曝光参数值 */
static int ISPCFG_SetDynamicAE(VI_PIPE ViPipe)
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
    stExposureAttr.bAERouteExValid = stISPCfgPipeParam[ViPipe].stStaticAe.bAERouteExValid;

    /* 曝光类型: 0-auto;1-手动 */
    stExposureAttr.enOpType = stISPCfgPipeParam[ViPipe].stStaticAe.ExposureOpType;

    /* AE 算法运行的间隔 */
    stExposureAttr.u8AERunInterval = stISPCfgPipeParam[ViPipe].stStaticAe.u8AERunInterval;

    /* 曝光时间 */
    stExposureAttr.stAuto.stExpTimeRange.u32Max = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoExpTimeMax;
    stExposureAttr.stAuto.stExpTimeRange.u32Min = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoExpTimeMin;

    /* sensor模拟增益 */
    stExposureAttr.stAuto.stAGainRange.u32Max = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoAGainMax;
    stExposureAttr.stAuto.stAGainRange.u32Min = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoAGainMin;

    /* sensor数字增益 */
    stExposureAttr.stAuto.stDGainRange.u32Max = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoDGainMax;
    stExposureAttr.stAuto.stDGainRange.u32Min = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoDGainMin;

    /* ISP数字增益 */
    stExposureAttr.stAuto.stISPDGainRange.u32Max = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoISPDGainMax;
    stExposureAttr.stAuto.stISPDGainRange.u32Min = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoISPDGainMin;

    /* 系统增益 */
    stExposureAttr.stAuto.stSysGainRange.u32Max = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoSysGainMax;
    stExposureAttr.stAuto.stSysGainRange.u32Min = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoSysGainMin;

    /* 自动降帧时的系统增益门限值 */
    stExposureAttr.stAuto.u32GainThreshold = stISPCfgPipeParam[ViPipe].stStaticAe.u32AutoGainThreshold;

    /* 自动曝光调整速度 */
    stExposureAttr.stAuto.u8Speed = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutoSpeed;

    /* 画面由暗到亮 AE 调节速度的偏差值，该值越大，画面从暗到 亮的速度越快 */
    stExposureAttr.stAuto.u16BlackSpeedBias = stISPCfgPipeParam[ViPipe].stStaticAe.u16AutoBlackSpeedBias;

    /* 自动曝光调整时的目标亮度 */
    stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[0];

    /* 自动曝光调整时的曝光量偏差值 */
    stExposureAttr.stAuto.u16EVBias = stISPCfgPipeParam[ViPipe].stStaticAe.u16AutoEVBias;

    /* 自动曝光策略:高光优先或低光优先 */
    stExposureAttr.stAuto.enAEStrategyMode = stISPCfgPipeParam[ViPipe].stStaticAe.bAutoAEStrategMode;

    /* 感兴趣区域的权重 */
    stExposureAttr.stAuto.u16HistRatioSlope = stISPCfgPipeParam[ViPipe].stStaticAe.u16AutoHistRatioSlope;

    /* 感兴趣区域对统计平均值影响的最大程度 */
    stExposureAttr.stAuto.u8MaxHistOffset = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutoMaxHistOffset;

    /* 自动曝光调整时对画面亮度的容忍偏差 */
    stExposureAttr.stAuto.u8Tolerance = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutoTolerance;

    /* 延时属性设置 */
    stExposureAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = stISPCfgPipeParam[ViPipe].stStaticAe.u16AutoBlackDelayFrame;
    stExposureAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = stISPCfgPipeParam[ViPipe].stStaticAe.u16AutoWhiteDelayFrame;

    //stExposureAttr.stAuto.enFSWDRMode = stISPCfgPipeParam[ViPipe].stStaticAe.enFSWDRMode;

    prtMD("bAERouteExValid = %d, u8AERunInterval = %d, u32Max = %d\n",
        stExposureAttr.bAERouteExValid,
        stExposureAttr.u8AERunInterval,
        stExposureAttr.stAuto.stExpTimeRange.u32Max);

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

static int ISPCFG_SetStaticAWB(VI_PIPE ViPipe)
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

    stWbAttr.stAuto.u16RefColorTemp = stISPCfgPipeParam[ViPipe].stStaticAwb.u16AutoRefColorTemp;
    for(i = 0;i < 4;i++)
    {
        stWbAttr.stAuto.au16StaticWB[i] = stISPCfgPipeParam[ViPipe].stStaticAwb.au16AutoStaticWB[i];
    }

    for(i = 0; i < 6;i++)
    {
        stWbAttr.stAuto.as32CurvePara[i] = stISPCfgPipeParam[ViPipe].stStaticAwb.as32AutoCurvePara[i];
    }

    stWbAttr.stAuto.u8RGStrength = stISPCfgPipeParam[ViPipe].stStaticAwb.u8AutoRGStrength;
    stWbAttr.stAuto.u8BGStrength = stISPCfgPipeParam[ViPipe].stStaticAwb.u8AutoBGStrength;
    stWbAttr.stAuto.u16Speed = stISPCfgPipeParam[ViPipe].stStaticAwb.u16AutoSpeed;
    stWbAttr.stAuto.u16ZoneSel = stISPCfgPipeParam[ViPipe].stStaticAwb.u16AutoZoneSel;
    stWbAttr.stAuto.u16HighColorTemp = stISPCfgPipeParam[ViPipe].stStaticAwb.u16AutoHighColorTemp;
    stWbAttr.stAuto.u16LowColorTemp = stISPCfgPipeParam[ViPipe].stStaticAwb.u16AutoLowColorTemp;

    s32Ret = HI_MPI_ISP_SetWBAttr(ViPipe, &stWbAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetWBAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetWDRExposure(VI_PIPE ViPipe)
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

    stWdrExposureAttr.enExpRatioType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u8ExpRatioType;
    stWdrExposureAttr.u32ExpRatioMax = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u32ExpRatioMax;
    stWdrExposureAttr.u32ExpRatioMin = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u32ExpRatioMin;

    stWdrExposureAttr.u16Tolerance = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u16Tolerance;
    stWdrExposureAttr.u16Speed = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u16Speed;
    stWdrExposureAttr.u16RatioBias = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.u16RatioBias;
    for(i = 0 ; i < 3; i++)
    {
        stWdrExposureAttr.au32ExpRatio[i] = stISPCfgPipeParam[ViPipe].stStaticWdrExposure.au32ExpRatio[i];
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

    stSaturationAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticSaturation.bOpType;
    stSaturationAttr.stManual.u8Saturation = stISPCfgPipeParam[ViPipe].stStaticSaturation.u8ManualSat;

    for (i = 0; i < 16; i++)
    {
        stSaturationAttr.stAuto.au8Sat[i] = stISPCfgPipeParam[ViPipe].stStaticSaturation.au8AutoSat[i];
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

    stDrcAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticDrc.bEnable;
    stDrcAttr.enCurveSelect = (ISP_DRC_CURVE_SELECT_E)stISPCfgPipeParam[ViPipe].stStaticDrc.u8CurveSelect;

    stDrcAttr.u8PDStrength = stISPCfgPipeParam[ViPipe].stStaticDrc.u8PDStrength;
    stDrcAttr.u8LocalMixingBrightMax = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingBrightMax;
    stDrcAttr.u8LocalMixingBrightMin = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingBrightMin;
    stDrcAttr.u8LocalMixingBrightThr = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingBrightThr;
    stDrcAttr.s8LocalMixingBrightSlo = stISPCfgPipeParam[ViPipe].stStaticDrc.s8LocalMixingBrightSlo;

    stDrcAttr.u8LocalMixingDarkMax = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingDarkMax;
    stDrcAttr.u8LocalMixingDarkMin = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingDarkMin;
    stDrcAttr.u8LocalMixingDarkThr = stISPCfgPipeParam[ViPipe].stStaticDrc.u8LocalMixingDarkThr;
    stDrcAttr.s8LocalMixingDarkSlo = stISPCfgPipeParam[ViPipe].stStaticDrc.s8LocalMixingDarkSlo;

    stDrcAttr.u8BrightGainLmt = stISPCfgPipeParam[ViPipe].stStaticDrc.u8BrightGainLmt;
    stDrcAttr.u8BrightGainLmtStep = stISPCfgPipeParam[ViPipe].stStaticDrc.u8BrightGainLmtStep;
    stDrcAttr.u8DarkGainLmtY = stISPCfgPipeParam[ViPipe].stStaticDrc.u8DarkGainLmtY;
    stDrcAttr.u8DarkGainLmtC = stISPCfgPipeParam[ViPipe].stStaticDrc.u8DarkGainLmtC;
    stDrcAttr.u8ContrastControl = stISPCfgPipeParam[ViPipe].stStaticDrc.u8ContrastControl;
    stDrcAttr.s8DetailAdjustFactor = stISPCfgPipeParam[ViPipe].stStaticDrc.s8DetailAdjustFactor;
    stDrcAttr.u8SpatialFltCoef = stISPCfgPipeParam[ViPipe].stStaticDrc.u8SpatialFltCoef;
    stDrcAttr.u8RangeFltCoef = stISPCfgPipeParam[ViPipe].stStaticDrc.u8RangeFltCoef;
    stDrcAttr.u8RangeAdaMax = stISPCfgPipeParam[ViPipe].stStaticDrc.u8RangeAdaMax;
    stDrcAttr.u8GradRevMax = stISPCfgPipeParam[ViPipe].stStaticDrc.u8GradRevMax;
    stDrcAttr.u8GradRevThr = stISPCfgPipeParam[ViPipe].stStaticDrc.u8GradRevThr;
    stDrcAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticDrc.u8DRCOpType;
    stDrcAttr.stAuto.u16Strength = stISPCfgPipeParam[ViPipe].stStaticDrc.u16AutoStrength;
    stDrcAttr.stAuto.u16StrengthMax = stISPCfgPipeParam[ViPipe].stStaticDrc.u16AutoStrengthMax;
    stDrcAttr.stAuto.u16StrengthMin = stISPCfgPipeParam[ViPipe].stStaticDrc.u16AutoStrengthMin;
    stDrcAttr.stManual.u16Strength = stISPCfgPipeParam[ViPipe].stStaticDrc.u16ManualStrength;

    stDrcAttr.stAsymmetryCurve.u8Asymmetry = stISPCfgPipeParam[ViPipe].stStaticDrc.u8Asymmetry;
    stDrcAttr.stAsymmetryCurve.u8Compress = stISPCfgPipeParam[ViPipe].stStaticDrc.u8Compress;
    stDrcAttr.stAsymmetryCurve.u8SecondPole = stISPCfgPipeParam[ViPipe].stStaticDrc.u8SecondPole;
    stDrcAttr.stAsymmetryCurve.u8Stretch = stISPCfgPipeParam[ViPipe].stStaticDrc.u8Stretch;
#if 0
    HI_S32 i = 0,j = 0;

    for(i = 0;i < 10; i++)
    {
    for(j = 0; j < 20;j++)
    {
    stDrcAttr.au16ToneMappingValue[(i * 20) + j] = stISPCfgPipeParam[ViPipe].stStaticDrc.au16ToneMappingValue[i][j];
    //prtMD("stDrcAttr.au16ToneMappingValue[%d] = %d\n",((i * 20) + j),stDrcAttr.au16ToneMappingValue[(i * 20) + j]);
    }
    }
#endif
#if 0
    for(i = 0;i < 33; i++)
    {
    stDrcAttr.au16ColorCorrectionLut[i] = stISPCfgPipeParam[ViPipe].stStaticDrc.au16ColorCorrectionLut[i];
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

    stCCMAttr.enOpType = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.CcmOpType;
    if(stCCMAttr.enOpType == OP_TYPE_AUTO)
    {
        stCCMAttr.stAuto.bISOActEn = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.bISOActEn;
        stCCMAttr.stAuto.bTempActEn = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.bTempActEn;
        stCCMAttr.stAuto.u16CCMTabNum = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.u16CCMTabNum;
        for(i = 0;i < stCCMAttr.stAuto.u16CCMTabNum;i++)
        {
            stCCMAttr.stAuto.astCCMTab[i].u16ColorTemp = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.astCCMTab[i].u16ColorTemp;
            for(j = 0;j < 9;j++)
            {
                stCCMAttr.stAuto.astCCMTab[i].au16CCM[j] = stISPCfgPipeParam[ViPipe].stStaticCCMAttr.astCCMTab[i].au16CCM[j];
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

    stDehazeAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticDehaze.bEnable;
    stDehazeAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticDehaze.u8DehazeOpType;
    stDehazeAttr.bUserLutEnable = stISPCfgPipeParam[ViPipe].stStaticDehaze.bUserLutEnable;

    stDehazeAttr.stManual.u8strength = stISPCfgPipeParam[ViPipe].stStaticDehaze.u8ManualStrength;
    stDehazeAttr.stAuto.u8strength = stISPCfgPipeParam[ViPipe].stStaticDehaze.u8Autostrength;
    stDehazeAttr.u16TmprfltIncrCoef = stISPCfgPipeParam[ViPipe].stStaticDehaze.u16prfltIncrCoef;
    stDehazeAttr.u16TmprfltDecrCoef = stISPCfgPipeParam[ViPipe].stStaticDehaze.u16prfltDecrCoef;

    for (i = 0; i < 256; i++)
    {
        stDehazeAttr.au8DehazeLut[i] = stISPCfgPipeParam[ViPipe].stStaticDehaze.au8DehazeLut[i];
    }

    s32Ret = HI_MPI_ISP_SetDehazeAttr(ViPipe, &stDehazeAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetDehazeAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

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

    if(ISPCFG_SceneMode == 0)
    {
        for (i = 0; i < AE_ZONE_ROW; i++)
        {
            for (j = 0; j < AE_ZONE_COLUMN; j++)
            {
                stStatisticsCfg.stAECfg.au8Weight[i][j] = stISPCfgPipeParam[ViPipe].stStatistics.au8AEWeight[i][j];
            }
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

    stCSCAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticCsc.bEnable;

    stCSCAttr.u8Contr = stISPCfgPipeParam[ViPipe].stStaticCsc.u8Contr;
    stCSCAttr.u8Luma = stISPCfgPipeParam[ViPipe].stStaticCsc.u8Luma;
    if(ViPipe == 2)
    {
        stCSCAttr.u8Satu = 0;   //如果是IR数据输出，饱和度设置为0
        stCSCAttr.u8Hue = 0;  //色调可以设置为0
    }
    else
    {
        stCSCAttr.u8Satu = stISPCfgPipeParam[ViPipe].stStaticCsc.u8Satu;  //RGB输出
        stCSCAttr.u8Hue = stISPCfgPipeParam[ViPipe].stStaticCsc.u8Hue;
    }
    stCSCAttr.enColorGamut = stISPCfgPipeParam[ViPipe].stStaticCsc.enColorGamut;

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

    stLDCI.bEnable = stISPCfgPipeParam[ViPipe].stStaticLdci.bEnable;
    stLDCI.enOpType = stISPCfgPipeParam[ViPipe].stStaticLdci.u8LDCIOpType;

    if(stLDCI.enOpType == OP_TYPE_MANUAL)
    {
        stLDCI.stManual.u16BlcCtrl = stISPCfgPipeParam[ViPipe].stStaticLdci.u16ManualBlcCtrl;
    }
    else if(stLDCI.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0; i< 16;i++)
        {
            stLDCI.stAuto.au16BlcCtrl[i] = stISPCfgPipeParam[ViPipe].stStaticLdci.u16AutoBlcCtrl[i];
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

    stShadingAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticShading.bEnable;

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

    stSharpenAttr.bEnable =  stISPCfgPipeParam[ViPipe].stStaticSharpen.bEnable;
    stSharpenAttr.enOpType = (ISP_OP_TYPE_E)stISPCfgPipeParam[ViPipe].stStaticSharpen.enOpType;
    if(stSharpenAttr.enOpType == OP_TYPE_MANUAL)
    {
        stSharpenAttr.stManual.u16TextureFreq = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualTextureFreq;
        stSharpenAttr.stManual.u16EdgeFreq = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualEdgeFreq;
        stSharpenAttr.stManual.u8OverShoot = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualOvershoot;
        stSharpenAttr.stManual.u8UnderShoot = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualUnderShoot;
        stSharpenAttr.stManual.u8ShootSupStr = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualSupStr;
        stSharpenAttr.stManual.u8ShootSupAdj = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualSupAdj;
        stSharpenAttr.stManual.u8DetailCtrl = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualDetailCtrl;
        stSharpenAttr.stManual.u8DetailCtrlThr = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualDetailCtrlThr;
        stSharpenAttr.stManual.u8EdgeFiltStr = stISPCfgPipeParam[ViPipe].stStaticSharpen.ManualEdgeFiltStr;
    }
    else if(stSharpenAttr.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0;i < 16;i++)
        {
            stSharpenAttr.stAuto.au16TextureFreq[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoTextureFreq[i];
            //prtMD("stSharpenAttr.stAuto.au16TextureFreq[%d] = %d\n",i,stSharpenAttr.stAuto.au16TextureFreq[i]);
            stSharpenAttr.stAuto.au16EdgeFreq[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoEdgeFreq[i];
            stSharpenAttr.stAuto.au8ShootSupStr[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoSupStr[i];
            stSharpenAttr.stAuto.au8ShootSupAdj[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoSupAdj[i];
            stSharpenAttr.stAuto.au8DetailCtrl[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoDetailCtrl[i];
            stSharpenAttr.stAuto.au8DetailCtrlThr[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoDetailCtrlThr[i];
            stSharpenAttr.stAuto.au8EdgeFiltStr[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoEdgeFiltStr[i];
            stSharpenAttr.stAuto.au8EdgeFiltMaxCap[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoEdgeFiltMaxCap[i];
            stSharpenAttr.stAuto.au8RGain[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoRGain[i];
            stSharpenAttr.stAuto.au8GGain[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoGGain[i];
            stSharpenAttr.stAuto.au8BGain[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoBGain[i];
            stSharpenAttr.stAuto.au8SkinGain[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoSkinGain[i];
            stSharpenAttr.stAuto.au16MaxSharpGain[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoMaxSharpGain[i];
        }

        for (i = 0; i < 16; i++)
        {
            stSharpenAttr.stAuto.au8OverShoot[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.au8OverShoot[i];
            stSharpenAttr.stAuto.au8UnderShoot[i] = stISPCfgPipeParam[ViPipe].stStaticSharpen.au8UnderShoot[i];
        }
#if 1
        for(j = 0; j < 16;j++)
        {
            for(i = 0; i < 32; i++)
            {
                stSharpenAttr.stAuto.au8LumaWgt[i][j] = stISPCfgPipeParam[ViPipe].stStaticSharpen.AutoLumaWgt[j][i];
                stSharpenAttr.stAuto.au16TextureStr[i][j] = stISPCfgPipeParam[ViPipe].stStaticSharpen.au16TextureStr[j][i];
                stSharpenAttr.stAuto.au16EdgeStr[i][j] = stISPCfgPipeParam[ViPipe].stStaticSharpen.au16EdgeStr[j][i];
            }
        }
#endif
    }

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

    stDemosaicAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticDemosaic.bEnable;
    stDemosaicAttr.enOpType = stISPCfgPipeParam[ViPipe].stStaticDemosaic.enOpType;
    if(stDemosaicAttr.enOpType == OP_TYPE_AUTO)
    {
        prtMD("stDemosaicAttr.stAuto.au8DetailSmoothRange:");
        for(i = 0; i < 16; i++)
        {
            stDemosaicAttr.stAuto.au8NonDirStr[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoNonDirStr[i];
            stDemosaicAttr.stAuto.au8NonDirMFDetailEhcStr[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoNonDirMFDetailEhcStr[i];
            stDemosaicAttr.stAuto.au8NonDirHFDetailEhcStr[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoNonDirHFDetailEhcStr[i];
            stDemosaicAttr.stAuto.au8DetailSmoothRange[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u16AutoDetailSmoothRange[i];
            printf(" %d", stDemosaicAttr.stAuto.au8DetailSmoothRange[i]);
            #if 0
            stDemosaicAttr.stAuto.au8ColorNoiseThdF[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoColorNoiseThdF;
            stDemosaicAttr.stAuto.au8ColorNoiseStrF[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoColorNoiseStrF;
            stDemosaicAttr.stAuto.au8ColorNoiseThdY[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoColorNoiseThdY;
            stDemosaicAttr.stAuto.au8ColorNoiseStrY[i] = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8AutoColorNoiseStrY;
            #endif
        }
        printf("\n");
    }
    else
    {
        stDemosaicAttr.stManual.u8NonDirStr = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8ManualNonDirStr;
        stDemosaicAttr.stManual.u8NonDirMFDetailEhcStr = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8ManualNonDirMFDetailEhcStr;
        stDemosaicAttr.stManual.u8NonDirHFDetailEhcStr = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u8ManualNonDirHFDetailEhcStr;
        stDemosaicAttr.stManual.u8DetailSmoothRange = stISPCfgPipeParam[ViPipe].stStaticDemosaic.u16ManualDetailSmoothStr;
    }

    s32Ret = HI_MPI_ISP_SetDemosaicAttr(ViPipe, &stDemosaicAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetDemosaicAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

int ISPCFG_SetPipeParam(VI_PIPE ViPipe, VPSS_GRP VpssGrp)
{
    HI_U32 i = 0;
    HI_U32 j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    VI_PIPE_NRX_PARAM_S stVINRXParam = {0};
    HI_U32 u32IsoLevel = 0;
    VPSS_GRP_ATTR_S stGrpAttr = {0};
    VPSS_GRP_NRX_PARAM_S stVPSSNRXParam = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    stVINRXParam.enNRVersion = VI_NR_V2;
    stVINRXParam.stNRXParamV2.enOptMode = OPERATION_MODE_MANUAL;
    stVPSSNRXParam .enNRVer = VPSS_NR_V2;
    stVPSSNRXParam .stNRXParam_V2.enOptMode = OPERATION_MODE_MANUAL;//OPERATION_MODE_AUTO;

    s32Ret = HI_MPI_VI_GetPipeNRXParam(ViPipe, &stVINRXParam);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_GetPipeNRXParam is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_GetGrpNRXParam(VpssGrp, &stVPSSNRXParam);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_GetGrpNRXParam is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    u32IsoLevel = ISPCFG_GetThreshValue(ISPCFG_IsoValue[ViPipe], stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.u32ThreeDNRCount, stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.au32ThreeDNRIso);

    /*VI*/
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFS1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFS1;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFS2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFS2;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFS4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFS4;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFT1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFT1;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFT2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFT2;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFT4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFT4;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SBR1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SBR1;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SBR2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SBR2;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SBR4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SBR4;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SRT0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SRT0;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SRT1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SRT1;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.DeRate = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.DeRate;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFR = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFR;

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.IEy.IES0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].IEy.IES0;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.IEy.IES1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].IEy.IES1;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.IEy.IES2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].IEy.IES2;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.IEy.IES3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].IEy.IES3;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.IEy.IEDZ = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].IEy.IEDZ;

    for (j = 0; j < 3; j++)
    {
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFR6[j]  = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFR6[j];
    }

    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.BWSF4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.BWSF4;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SPN6  = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SPN6;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SBN6  = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SBN6;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.PBR6  = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.PBR6;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.JMODE = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.JMODE;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.DeIdx = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.DeIdx;
    stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.TriTh = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.TriTh;
    if (stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.TriTh)
    {
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN0;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN1;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN2;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN3;

        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.STH1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.STH1;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.STH2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.STH2;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.STH3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.STH3;
    }
    else
    {
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN0;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN1;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.SFN3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.SFN3;

        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.STH1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.STH1;
        stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.STH3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVIValue[u32IsoLevel].SFy.STH3;
    }

    /* VPSS */

    for(i = 0;i < 3;i++)
    {
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFS1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFS1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFS2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFS2;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFS4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFS4;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFT1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFT1;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFT2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFT2;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFT4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFT4;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SBR1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SBR1;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SBR2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SBR2;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SBR4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SBR4;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFR  = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFR;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.IEy[i].IES0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].IEy[i].IES0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.IEy[i].IES1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].IEy[i].IES1;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.IEy[i].IES2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].IEy[i].IES2;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.IEy[i].IES3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].IEy[i].IES3;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.IEy[i].IEDZ = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].IEy[i].IEDZ;

        for(j = 0; j < 3; j++)
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFR6[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFR6[j];
        }

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].NRyEn = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].NRyEn;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].BWSF4 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].BWSF4;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SPN6 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SPN6;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SBN6 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SBN6;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].PBR6 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].PBR6;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].JMODE = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].JMODE;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].TriTh = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].TriTh;

        if(stVINRXParam.stNRXParamV2.stNRXManualV2.stNRXParamV2.SFy.TriTh)
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN0;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN1;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN2;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN3;

            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].STH1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].STH1;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].STH2 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].STH2;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].STH3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].STH3;
        }
        else
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN0;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN1;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].SFN3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].SFN3;

            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].STH1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].STH1;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[i].STH3 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[i].STH3;
        }
    }

    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[1].kMode = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[1].kMode;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[2].kMode = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[2].kMode;

    if(stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[1].kMode > 1)
    {
        for(j = 0; j < 32; j++)
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[1].SBSk[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[1].SBSk[j];
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[1].SDSk[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[1].SDSk[j];
        }
    }

    if(stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[2].kMode > 1)
    {
        for(j = 0; j < 32; j++)
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[2].SBSk[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[2].SBSk[j];
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.SFy[2].SDSk[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].SFy[2].SDSk[j];
        }
    }

    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[0].tEdge = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[0].tEdge;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[1].tEdge = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[1].tEdge;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[0].bRef = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[0].bRef;
    ///组数据
    stGrpAttr.stNrAttr.enNrMotionMode = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].GMC.GMEMode;
    ////////////////////////////////////////////

    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[0].RFI = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[0].RFI;

    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[0].DZMode0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[0].DZMode0;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[0].DZMode1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[0].DZMode1;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[1].DZMode0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[1].DZMode0;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[1].DZMode1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[1].DZMode1;

    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.RFs.RFUI = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].RFs.RFUI;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.RFs.RFDZ = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].RFs.RFDZ ;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.RFs.RFSLP = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].RFs.RFSLP;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.RFs.advMATH = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].RFs.advMATH;

    for(i=0; i < 2;i++)
    {
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MATH0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MATH0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MATH1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MATH1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MATE0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MATE0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MATE1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MATE1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MABW0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MABW0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MABW1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MABW1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MASW = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MASW;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MADZ0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MADZ0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MADZ1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MADZ1;

        if ((stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MADZ0 > 0) || (stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MADZ1 > 0))
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MABR0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MABR0;
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MABR1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MABR1;
        }

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].biPath = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].biPath;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI00 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI00;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI01 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI01;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI02 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI02;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI10 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI10;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI11 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI11;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MAI12 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MAI12;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MATW = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MATW;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.MDy[i].MASW = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].MDy[i].MASW;
    }

    for(i =0;i < 2;i++)
    {
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSS0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TSS0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSS1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TSS1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFS0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFS0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFS1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFS1;
        prtMD("stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSS0 = %d,TFS0 = %d\n",stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSS0,stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFS0);

#if 1    ///待定参数
        for (j = 0; j < 6; j++)
        {
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFR0[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFR0[j];
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFR0[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFR0[j];
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFR1[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFR1[j];
            stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TFR1[j] = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TFR1[j];
        }
#endif

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].STR0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].STR0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].STR1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].STR1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].SDZ0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].SDZ0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].SDZ1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].SDZ1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSI0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TSI0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TSI1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TSI1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TDZ0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TDZ0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TDZ1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TDZ1;

        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TDX0 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TDX0;
        stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.TFy[i].TDX1 = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].TFy[i].TDX1;
    }


    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.pNRc.SFC = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].pNRc.SFC;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.pNRc.CTFS = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].pNRc.CTFS;
    stVPSSNRXParam.stNRXParam_V2.stNRXManual.stNRXParam.pNRc.TFC = stISPCfgPipeParam[ViPipe].stDynamicThreeDNR.astThreeDNRVPSSValue[u32IsoLevel].pNRc.TFC;
	prtMD("----------------------------3D noise -----!\n");
    stVINRXParam .enNRVersion = VI_NR_V2;
    stVINRXParam .stNRXParamV2.enOptMode = OPERATION_MODE_MANUAL;
    stVPSSNRXParam .enNRVer = VPSS_NR_V2;
    stVPSSNRXParam .stNRXParam_V2.enOptMode = OPERATION_MODE_MANUAL;

    s32Ret = HI_MPI_VI_SetPipeNRXParam(ViPipe, &stVINRXParam);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VI_SetPipeNRXParam is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp, &stVPSSNRXParam);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_SetGrpNRXParam is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_GetGrpAttr(VpssGrp, &stGrpAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_GetGrpAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    stGrpAttr.bNrEn = HI_TRUE;
    s32Ret = HI_MPI_VPSS_SetGrpAttr(VpssGrp, &stGrpAttr );
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_VPSS_SetGrpAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

static int ISPCFG_SetGamma(VI_PIPE ViPipe)
{
    HI_U32 i, j = 0;
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

    stIspGammaAttr.bEnable = stISPCfgPipeParam[ViPipe].stDynamicGamma.bEnable;
    for (i = 0; i < 25; i++)
    {
        for(j = 0;j < 41;j++)
        {
            stIspGammaAttr.u16Table[(i * 41) + j] = stISPCfgPipeParam[ViPipe].stDynamicGamma.u16Table[i][j];  //gamma参数
        }
    }

    stIspGammaAttr.enCurveType = stISPCfgPipeParam[ViPipe].stDynamicGamma.enCurveType;

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

    stIspWDRFSAttr.enWDRMergeMode = stISPCfgPipeParam[ViPipe].stStaticWDRFS.enWDRMergeMode;

    if(stIspWDRFSAttr.enWDRMergeMode ==  0)
    {
        stIspWDRFSAttr.stWDRCombine.bMotionComp = stISPCfgPipeParam[ViPipe].stStaticWDRFS.bCombineMotionComp;
        stIspWDRFSAttr.stWDRCombine.u16ShortThr = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16CombineShortThr;

        stIspWDRFSAttr.stWDRCombine.u16LongThr = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16CombineLongThr;
        stIspWDRFSAttr.stWDRCombine.bForceLong = stISPCfgPipeParam[ViPipe].stStaticWDRFS.bCombineForceLong;
        stIspWDRFSAttr.stWDRCombine.u16ForceLongHigThr = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16CombineForceLongHigThr;
        stIspWDRFSAttr.stWDRCombine.u16ForceLongLowThr = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16CombineForceLongLowThr;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.bShortExpoChk = stISPCfgPipeParam[ViPipe].stStaticWDRFS.bWDRMdtShortExpoChk;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u16ShortCheckThd = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16WDRMdtShortCheckThd;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.bMDRefFlicker = stISPCfgPipeParam[ViPipe].stStaticWDRFS.bWDRMdtMDRefFlicker;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u8MdtStillThd = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u8WDRMdtMdtStillThd;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.u8MdtLongBlend = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u8WDRMdtMdtLongBlend;
        stIspWDRFSAttr.stWDRCombine.stWDRMdt.enOpType = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u8WDRMdtOpType;

        for(i = 0 ; i < 16;i++)
        {
            stIspWDRFSAttr.stWDRCombine.stWDRMdt.stAuto.au8MdThrHigGain[i] = stISPCfgPipeParam[ViPipe].stStaticWDRFS.au8AutoMdThrHigGain[i];
            stIspWDRFSAttr.stWDRCombine.stWDRMdt.stAuto.au8MdThrLowGain[i] = stISPCfgPipeParam[ViPipe].stStaticWDRFS.au8AutoMdThrLowGain[i];
        }

        for(i = 0;i < 4;i++)
        {
            stIspWDRFSAttr.stFusion.au16FusionThr[i] = stISPCfgPipeParam[ViPipe].stStaticWDRFS.u16FusionFusionThr[i];
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

    stNRAttr.bEnable = stISPCfgPipeParam[ViPipe].stStaticBNRS.bEnable;
    stNRAttr.bNrLscEnable = stISPCfgPipeParam[ViPipe].stStaticBNRS.bNrLscEnable;
    stNRAttr.u8BnrLscMaxGain = stISPCfgPipeParam[ViPipe].stStaticBNRS.u8BnrMaxGain;
    stNRAttr.u16BnrLscCmpStrength = stISPCfgPipeParam[ViPipe].stStaticBNRS.u16BnrLscCmpStrength;
    stNRAttr.enOpType = stISPCfgPipeParam[ViPipe].stStaticBNRS.u8BnrOptype;

    if(stNRAttr.enOpType == OP_TYPE_AUTO)
    {
        for(i = 0; i < 4;i++)
        {
            for(j = 0; j < 16;j++)
            {
                stNRAttr.stAuto.au8ChromaStr[i][j] = stISPCfgPipeParam[ViPipe].stStaticBNRS.au8ChromaStr[i][j];
                stNRAttr.stAuto.au16CoarseStr[i][j] = stISPCfgPipeParam[ViPipe].stStaticBNRS.au16CoarseStr[i][j];
            }
        }

        for(i = 0;i < 16;i++)
        {
            stNRAttr.stAuto.au8FineStr[i] = stISPCfgPipeParam[ViPipe].stStaticBNRS.au8FineStr[i];
            stNRAttr.stAuto.au16CoringWgt[i] = stISPCfgPipeParam[ViPipe].stStaticBNRS.au16CoringWgt[i];
        }

        for(i = 0;i < 4;i++)
        {
            stNRAttr.stWdr.au8WDRFrameStr[i] = stISPCfgPipeParam[ViPipe].stStaticBNRS.au8WDRFrameStr[i];
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

int ISPCFG_SetExposure(VI_PIPE ViPipe)
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

    if (ISPCFG_IsoValue[ViPipe] >= 100 && ISPCFG_IsoValue[ViPipe] < 200)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[0];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 200 && ISPCFG_IsoValue[ViPipe] < 400)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[1];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 400 && ISPCFG_IsoValue[ViPipe] < 800)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[2];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 800 && ISPCFG_IsoValue[ViPipe] < 1600)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[3];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 1600 && ISPCFG_IsoValue[ViPipe] < 3200)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[4];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 3200 && ISPCFG_IsoValue[ViPipe] < 6400)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[5];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 6400 && ISPCFG_IsoValue[ViPipe] < 12800)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[6];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 12800 && ISPCFG_IsoValue[ViPipe] < 25600)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[7];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 25600 && ISPCFG_IsoValue[ViPipe] < 51200)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[8];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 51200 && ISPCFG_IsoValue[ViPipe] < 102400)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[9];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 102400 && ISPCFG_IsoValue[ViPipe] < 204800)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[10];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 204800 && ISPCFG_IsoValue[ViPipe] < 409600)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[11];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 409600 && ISPCFG_IsoValue[ViPipe] < 819200)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[12];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 819200 && ISPCFG_IsoValue[ViPipe] < 1638400)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[13];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 1638400 && ISPCFG_IsoValue[ViPipe] < 1638400)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[14];
    }
    else if (ISPCFG_IsoValue[ViPipe] >= 1638400 && ISPCFG_IsoValue[ViPipe] < 3276800)
    {
        stExposureAttr.stAuto.u8Compensation = stISPCfgPipeParam[ViPipe].stStaticAe.u8AutostaticCompesation[15];
    }
    else
    {
        stExposureAttr.stAuto.u8Compensation = 40;
    }

    //prtMD("--ViPipe = %d-stExposureAttr.stAuto.u8Compensation = %d，g_u32IsoValue[ViPipe] = %d\n",ViPipe,stExposureAttr.stAuto.u8Compensation,ISPCFG_IsoValue[ViPipe]);
    s32Ret = HI_MPI_ISP_SetExposureAttr(ViPipe, &stExposureAttr);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetExposureAttr is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    return 0;
}

//无人脸时设置曝光权重
int ISPCFG_SetNormalStatictics(VI_PIPE ViPipe)
{
    HI_S32 i = 0;
    HI_S32 j = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_STATISTICS_CFG_S stStaCfg = {0};

    if (ViPipe >= VI_MAX_PIPE_NUM)
    {
        prtMD("invalid input ViPipe = %d\n", ViPipe);
        return -1;
    }

    s32Ret = HI_MPI_ISP_GetStatisticsConfig(ViPipe, &stStaCfg);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_GetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
        return -1;
    }

    for (i = 0; i < AE_ZONE_ROW; i++)
    {
        for (j = 0; j < AE_ZONE_COLUMN; j++)
        {
            stStaCfg.stAECfg.au8Weight[i][j] = stISPCfgPipeParam[ViPipe].stStatistics.au8AEWeight[i][j];
        }
    }

    s32Ret = HI_MPI_ISP_SetStatisticsConfig(ViPipe, &stStaCfg);
    if(s32Ret != HI_SUCCESS)
    {
        prtMD("HI_MPI_ISP_SetStatisticsConfig is failed! s32Ret = %#x\n",s32Ret);
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

    s32Ret = ISPCFG_GetBypassParam(viPipe, &stISPCfgPipeParam[viPipe].stModuleState);
    if(0 != s32Ret)
    {
        prtMD("ISPCFG_GetBypassParam error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 加载ini文件中参数 */
    s32Ret = ISPCFG_GetBlackValue(viPipe, &stISPCfgPipeParam[viPipe].stStaticBlackLevel);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetBlackValue error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticAe(viPipe, &stISPCfgPipeParam[viPipe].stStaticAe);
    if (0 != s32Ret)
    {
        prtMD("SCENE_LoadStaticAE error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticAwb(viPipe, &stISPCfgPipeParam[viPipe].stStaticAwb);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticAwb error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetWdrExposure(viPipe, &stISPCfgPipeParam[viPipe].stStaticWdrExposure);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetWdrExposure error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticSaturation(viPipe, &stISPCfgPipeParam[viPipe].stStaticSaturation);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticSaturation error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticDRC(viPipe, &stISPCfgPipeParam[viPipe].stStaticDrc);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticDRC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticCCM(viPipe, &stISPCfgPipeParam[viPipe].stStaticCCMAttr);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticCCM error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticDehaze(viPipe, &stISPCfgPipeParam[viPipe].stStaticDehaze);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticDehaze error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    //曝光权重
    s32Ret = ISPCFG_GetStaticStatistics(viPipe, &stISPCfgPipeParam[viPipe].stStatistics);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticStatistics error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticCSC(viPipe, &stISPCfgPipeParam[viPipe].stStaticCsc);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticCSC error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticLDCI(viPipe, &stISPCfgPipeParam[viPipe].stStaticLdci);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticLDCI error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticShading(viPipe, &stISPCfgPipeParam[viPipe].stStaticShading);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticShading error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticSharpen(viPipe, &stISPCfgPipeParam[viPipe].stStaticSharpen);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticSharpen error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticDemosaic(viPipe, &stISPCfgPipeParam[viPipe].stStaticDemosaic);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticDemosaic error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetDynamic3DNR(viPipe, &stISPCfgPipeParam[viPipe].stDynamicThreeDNR);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetDynamic3DNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticFSWDR(viPipe, &stISPCfgPipeParam[viPipe].stStaticWDRFS);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticFSWDR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetDynamicGamma(viPipe, &stISPCfgPipeParam[viPipe].stDynamicGamma);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetDynamicGamma error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    s32Ret = ISPCFG_GetStaticBNR(viPipe, &stISPCfgPipeParam[viPipe].stStaticBNRS);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_GetStaticBNR error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
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
    s32Ret = ISPCFG_SetStaticBlackLevel(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetStaticBlackLevel error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 1-设置自动曝光 */
    s32Ret = ISPCFG_SetDynamicAE(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetDynamicAE error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 2-设置自动白平衡 */
    s32Ret = ISPCFG_SetStaticAWB(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetStaticAWB error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

    /* 3-设置wdr宽动态 */
    s32Ret = ISPCFG_SetWDRExposure(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetWDRExposure error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
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

#if 1
    /* 12-设置Demosaic */
    s32Ret = ISPCFG_SetDemosaic(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetDemosaic error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }
#endif

    /* 13-设置gamma */
    s32Ret = ISPCFG_SetGamma(viPipe);
    if (0 != s32Ret)
    {
        prtMD("ISPCFG_SetGamma error! viPipe = %d, s32Ret = %#x\n", viPipe, s32Ret);
        goto ret;
    }

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

