#include "mipi_rx.h"
#include <errno.h>

combo_dev_attr_t MIPI_4lane_CHN0_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3}
        }
    }
};

combo_dev_attr_t MIPI_4lane_CHN0_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_DOL,
            {0, 1, 2, 3}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 2, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_720P_NOWDR_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1280, 720},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 2, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_720P_NOWDR_ATTR =
{
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1280, 720},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR =
{
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_DOL,
            {0, 2, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR =
{
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_DOL,
            {0, 1, -1, -1}
        }
    }
};

int Media_MipiRx_SetHsMode(lane_divide_mode_t lane_divide_mode)
{
    HI_S32 fd = -1;
    HI_S32 s32Ret = 0;

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed, errno = %d\n", errno);
        return -1;
    }

    s32Ret = ioctl(fd, HI_MIPI_SET_HS_MODE, &lane_divide_mode);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("HI_MIPI_SET_HS_MODE error! s32Ret = %#x\n", s32Ret);
    }

    close(fd);
    return s32Ret;
};

int Media_MipiRx_EnableClock(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32              i = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    combo_dev_t         devno = 0;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;

    if (!pViParam)
    {
        prtMD("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        devno = pViInfo->stSnsInfo.MipiDev;

        s32Ret = ioctl(fd, HI_MIPI_ENABLE_MIPI_CLOCK, &devno);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("MIPI_ENABLE_CLOCK %d failed\n", devno);
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_ResetInterface(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32              i = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    combo_dev_t         devno = 0;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;

    if (!pViParam)
    {
        prtMD("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        devno = pViInfo->stSnsInfo.MipiDev;

        s32Ret = ioctl(fd, HI_MIPI_RESET_MIPI, &devno);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("RESET_MIPI %d failed\n", devno);
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_ResetSensor(void)
{
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    sns_rst_source_t    SnsDev = 0;

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
    {
        s32Ret = ioctl(fd, HI_MIPI_RESET_SENSOR, &SnsDev);

        if (HI_SUCCESS != s32Ret)
        {
            prtMD("HI_MIPI_RESET_SENSOR failed\n");
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_EnableSensorClock(void)
{
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    sns_clk_source_t    SnsDev = 0;

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (SnsDev = 0; SnsDev < SNS_MAX_CLK_SOURCE_NUM; SnsDev++)
    {
        s32Ret = ioctl(fd, HI_MIPI_ENABLE_SENSOR_CLOCK, &SnsDev);

        if (HI_SUCCESS != s32Ret)
        {
            prtMD("HI_MIPI_ENABLE_SENSOR_CLOCK failed\n");
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_GetAttr(VI_SNS_TYPE_E enSnsType, combo_dev_t MipiDev, combo_dev_attr_t *pComboAttr)
{
    if (pComboAttr == NULL)
    {
        prtMD("invalid input pComboAttr = %p\n", pComboAttr);
        return -1;
    }

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
            hi_memcpy(pComboAttr, sizeof(combo_dev_attr_t), &MIPI_4lane_CHN0_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR, sizeof(combo_dev_attr_t));
            break;

        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            hi_memcpy(pComboAttr, sizeof(combo_dev_attr_t), &MIPI_4lane_CHN0_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR, sizeof(combo_dev_attr_t));
            break;

        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
            if (0 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR, sizeof(combo_dev_attr_t));
            }
            else if (1 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR, sizeof(combo_dev_attr_t));
            }
            break;

        case SONY_IMX327_2L_MIPI_720P_30FPS_12BIT:
            if (0 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_720P_NOWDR_ATTR, sizeof(combo_dev_attr_t));
            }
            else if (1 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_720P_NOWDR_ATTR, sizeof(combo_dev_attr_t));
            }
            break;

        case SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
            if (0 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN0_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR, sizeof(combo_dev_attr_t));
            }
            else if (1 == MipiDev)
            {
                memcpy(pComboAttr, &MIPI_2lane_CHN1_SENSOR_IMX327_12BIT_2M_WDR2to1_ATTR, sizeof(combo_dev_attr_t));
            }
            break;

        default:
            prtMD("not support enSnsType: %d\n", enSnsType);
            hi_memcpy(pComboAttr, sizeof(combo_dev_attr_t), &MIPI_4lane_CHN0_SENSOR_IMX327_12BIT_2M_NOWDR_ATTR, sizeof(combo_dev_attr_t));
    }

    return HI_SUCCESS;
}

int Media_MipiRx_SetAttr(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32              i = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;
    combo_dev_attr_t    stcomboDevAttr = {0};

    if (!pViParam)
    {
        prtMD("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum  = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        s32Ret = Media_MipiRx_GetAttr(pViInfo->stSnsInfo.enSnsType, pViInfo->stSnsInfo.MipiDev, &stcomboDevAttr);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("Media_MipiRx_GetAttr error! s32Ret = %#x\n", s32Ret);
            goto EXIT;
        }

        stcomboDevAttr.devno = pViInfo->stSnsInfo.MipiDev;

        prtMD("MipiDev %d, SetMipiAttr enWDRMode: %d\n", pViInfo->stSnsInfo.MipiDev, pViInfo->stDevInfo.enWDRMode);

        s32Ret = ioctl(fd, HI_MIPI_SET_DEV_ATTR, &stcomboDevAttr);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("MIPI_SET_DEV_ATTR failed\n");
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_UnresetInterface(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32              i = 0;
    HI_S32              s32ViNum = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    combo_dev_t         devno = 0;
    MEDIA_VI_INFO_S*    pViInfo = HI_NULL;

    if (!pViParam)
    {
        prtMD("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (i = 0; i < pViParam->s32WorkingViNum; i++)
    {
        s32ViNum = pViParam->s32WorkingViId[i];
        pViInfo = &pViParam->stViInfo[s32ViNum];

        devno = pViInfo->stSnsInfo.MipiDev;

        s32Ret = ioctl(fd, HI_MIPI_UNRESET_MIPI, &devno);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("UNRESET_MIPI %d failed\n", devno);
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_UnresetSensor(void)
{
    HI_S32              s32Ret = HI_SUCCESS;
    HI_S32              fd = -1;
    sns_rst_source_t    SnsDev = 0;

    fd = open(MIPI_DEV_NODE, O_RDWR);
    if (fd < 0)
    {
        prtMD("open hi_mipi dev failed\n");
        return HI_FAILURE;
    }

    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
    {
        s32Ret = ioctl(fd, HI_MIPI_UNRESET_SENSOR, &SnsDev);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("HI_MIPI_UNRESET_SENSOR failed\n");
            goto EXIT;
        }
    }

EXIT:
    close(fd);

    return s32Ret;
}

int Media_MipiRx_Start(MEDIA_VI_PARAM_S *pViParam)
{
    HI_S32 s32Ret = HI_SUCCESS;
    lane_divide_mode_t lane_divide_mode = LANE_DIVIDE_MODE_1;   /* 2 + 2 */

    if (pViParam == NULL)
    {
        prtMD("invalid pViParam = %p\n", pViParam);
        return -1;
    }

    s32Ret = Media_MipiRx_SetHsMode(lane_divide_mode);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_SetHsMode error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_EnableClock(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_EnableClock error! s32Ret = %#x\n", s32Ret);
        return -1;
    }
    
    s32Ret = Media_MipiRx_ResetInterface(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_ResetInterface error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_EnableSensorClock();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_EnableSensorClock error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_ResetSensor();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_ResetSensor error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_SetAttr(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_SetAttr error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_UnresetInterface(pViParam);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_UnresetInterface error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    s32Ret = Media_MipiRx_UnresetSensor();
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("Media_MipiRx_UnresetSensor error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    return s32Ret;
}