#include <errno.h>

#include "mipi_tx.h"

combo_dev_cfg_t mipi_tx_cfg_600X1024_RP = {
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .video_mode = BURST_MODE,
    .output_format = OUT_FORMAT_RGB_24_BIT,

    .sync_info = {
        .vid_pkt_size  = 1024,
        .vid_hsa_pixels = 4,
        .vid_hbp_pixels = 60,
        .vid_hline_pixels = 1224,
        .vid_vsa_lines = 2,
        .vid_vbp_lines = 16,
        .vid_vfp_lines = 16,
        .vid_active_lines = 634,
        .edpi_cmd_size = 0,
    },

    .phy_data_rate = 495,
    .pixel_clk = 46561,
};

combo_dev_cfg_t mipi_tx_cfg_640X480_60 =
{
    .devno = 0,
    .lane_id = {0, 1, -1, -1},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .video_mode =  BURST_MODE,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .sync_info = {
        .vid_pkt_size     = 640,
        .vid_hsa_pixels   = 120,
        .vid_hbp_pixels   = 120,
        .vid_hline_pixels = 1000,
        .vid_vsa_lines    = 5,
        .vid_vbp_lines    = 13,
        .vid_vfp_lines    = 17,
        .vid_active_lines = 480,
        .edpi_cmd_size = 0,
    },
    .phy_data_rate = 371,
    .pixel_clk = 30900,
};

int Media_MipiTx_Init(MEDIA_LCD_IDX_E lcdTypeIdx)
{
    int fd = -1;
    HI_S32 s32Ret = HI_SUCCESS;

    prtMD("start! lcdTypeIdx = %d\n", lcdTypeIdx);

    fd = open(MIPI_TX_DEV, O_RDWR);
    if (fd < 0)
    {
        prtMD("open %s failed! errno = %d\n", MIPI_TX_DEV, errno);
        return -1;
    }

    if (lcdTypeIdx == LCD_IDX_7_600X1024_RP)
    {
        s32Ret = ioctl(fd, HI_MIPI_TX_SET_DEV_CFG, &mipi_tx_cfg_600X1024_RP);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("MIPI_TX SET_DEV_CONFIG failed! s32Ret = %#x, errno = %d\n", s32Ret, errno);
            close(fd);
            return s32Ret;
        }
    }
    else if (lcdTypeIdx == LCD_IDX_3_480X640)
    {
        s32Ret = ioctl(fd, HI_MIPI_TX_SET_DEV_CFG, &mipi_tx_cfg_640X480_60);
        if (HI_SUCCESS != s32Ret)
        {
            prtMD("MIPI_TX SET_DEV_CONFIG failed! s32Ret = %#x, errno = %d\n", s32Ret, errno);
            close(fd);
            return s32Ret;
        }
    }
    else
    {
        prtMD("invalid input lcdTypeIdx = %d\n", lcdTypeIdx);
        return -1;
    }

    usleep(20 * 1000);

    s32Ret = ioctl(fd, HI_MIPI_TX_ENABLE);
    if (HI_SUCCESS != s32Ret)
    {
        prtMD("MIPI_TX enable failed\n");
        close(fd);
        return -1;
    }

    close(fd);

    prtMD("end! lcdTypeIdx = %d\n", lcdTypeIdx);

    return 0;
}

