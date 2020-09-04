/*
* Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
* Description:
* Author: Hisilicon multimedia software group
* Create: 2011/06/28
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hi_math.h>

#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"

#ifdef HI_GPIO_I2C
#include "gpioi2c_ex.h"
#else
#include "hi_i2c.h"
#endif

const unsigned char gc2145_i2c_addr     =    0x78;
const unsigned int  gc2145_addr_byte    =    1;
const unsigned int  gc2145_data_byte    =    1;
static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};
static HI_BOOL g_bStandby[ISP_MAX_PIPE_NUM] = {0};

extern ISP_SNS_STATE_S       *g_pastGc2145[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U      g_aunGc2145BusInfo[];

// sensor fps mode
#define GC2145_SENSOR_1080P_30FPS_LINEAR_MODE  (0)

int gc2145_i2c_init(VI_PIPE ViPipe)
{
    char acDevFile[16] = {0};
    HI_U8 u8DevNum;

    if (g_fd[ViPipe] >= 0) {
        return HI_SUCCESS;
    }
#ifdef HI_GPIO_I2C
    int ret;

    g_fd[ViPipe] = open("/dev/gpioi2c_ex", O_RDONLY, S_IRUSR);
    if (g_fd[ViPipe] < 0) {
        ISP_ERR_TRACE("Open gpioi2c_ex error!\n");
        return HI_FAILURE;
    }
#else
    int ret;

    u8DevNum = g_aunGc2145BusInfo[ViPipe].s8I2cDev;

    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);

    if (g_fd[ViPipe] < 0) {
        ISP_ERR_TRACE("Open /dev/hi_i2c_drv-%u error!\n", u8DevNum);
        return HI_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (gc2145_i2c_addr >> 1));
    if (ret < 0) {
        ISP_ERR_TRACE("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }
#endif

    return HI_SUCCESS;
}

int gc2145_i2c_exit(VI_PIPE ViPipe)
{
    if (g_fd[ViPipe] >= 0) {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return HI_SUCCESS;
    }
    return HI_FAILURE;
}

int gc2145_read_register(VI_PIPE ViPipe, int addr)
{
    return HI_SUCCESS;
}


int gc2145_write_register(VI_PIPE ViPipe, HI_U32 addr, HI_U32 data)
{
    if (g_fd[ViPipe] < 0) {
        return HI_SUCCESS;
    }

#ifdef HI_GPIO_I2C
    i2c_data.dev_addr = gc2145_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = gc2145_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = gc2145_data_byte;

    ret = ioctl(g_fd[ViPipe], GPIO_I2C_WRITE, &i2c_data);

    if (ret) {
        ISP_ERR_TRACE("GPIO-I2C write faild!\n");
        return ret;
    }
#else
    int idx = 0;
    int ret;
    char buf[8];

    if (gc2145_addr_byte == 2) {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    } else {
        buf[idx] = addr & 0xff;
        idx++;
    }

    if (gc2145_data_byte == 2) {
        buf[idx] = (data >> 8) & 0xff;
        idx++;
        buf[idx] = data & 0xff;
        idx++;
    } else {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_fd[ViPipe], buf, gc2145_addr_byte + gc2145_data_byte);
    if (ret < 0) {
        ISP_ERR_TRACE("I2C_WRITE error!\n");
        return HI_FAILURE;
    }
#endif
    return HI_SUCCESS;
}



//旁路
void gc2145_standby(VI_PIPE ViPipe)
{
    g_bStandby[ViPipe] = HI_TRUE;
    return;
}
//重启
void gc2145_restart(VI_PIPE ViPipe)
{
    g_bStandby[ViPipe] = HI_FALSE;
    return;
}

void gc2145_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
    return;
}

void gc2145_linear_960p30_init(VI_PIPE ViPipe);

void gc2145_init(VI_PIPE ViPipe)
{
    HI_U32           i;
    HI_U8            u8ImgMode;
    HI_BOOL         bInit;

    bInit       = g_pastGc2145[ViPipe]->bInit;
    u8ImgMode   = g_pastGc2145[ViPipe]->u8ImgMode;

    if (bInit == HI_FALSE) {
        /* 1. sensor i2c init */
        gc2145_i2c_init(ViPipe);
    }

    switch (u8ImgMode) {
        case 0:
            gc2145_linear_960p30_init(ViPipe);
            break;
        default:
            printf("GC2053_SENSOR_CTL_Not support this mode\n");
            g_pastGc2145[ViPipe]->bInit = HI_FALSE;
            break;
    }

    #if 0
    for (i = 0; i < g_pastGc2145[ViPipe]->astRegsInfo[0].u32RegNum; i++) {
        gc2145_write_register(ViPipe, g_pastGc2145[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr, g_pastGc2145[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
    }
    #endif

    g_pastGc2145[ViPipe]->bInit = HI_TRUE;

    printf("GC2053 init succuss!\n");

    return;
}

void gc2145_exit(VI_PIPE ViPipe)
{
    gc2145_i2c_exit(ViPipe);
    g_bStandby[ViPipe] = HI_FALSE;

    return;
}

void gc2145_linear_960p30_init(VI_PIPE ViPipe)
{
    printf("Func:%s, Line:%d, Date:%s, Time:%s, Start! ViPipe = %d\n", __FUNCTION__, __LINE__, __DATE__, __TIME__, ViPipe);
    gc2145_write_register(ViPipe, 0xfe, 0xf0);
    gc2145_write_register(ViPipe, 0xfe, 0xf0);
    gc2145_write_register(ViPipe, 0xfe, 0xf0);
    gc2145_write_register(ViPipe, 0xfc, 0x06);
    gc2145_write_register(ViPipe, 0xf6, 0x00);
    gc2145_write_register(ViPipe, 0xf7, 0x1d);
    gc2145_write_register(ViPipe, 0xf8, 0x84);
    gc2145_write_register(ViPipe, 0xfa, 0x00);
    gc2145_write_register(ViPipe, 0xf9, 0x8e);
    gc2145_write_register(ViPipe, 0xf2, 0x00);

    /////////////////////////////////////////////////
    //////////////////ISP reg//////////////////////
    ////////////////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0x03 , 0x06);
    gc2145_write_register(ViPipe, 0x04 , 0xb0);
    gc2145_write_register(ViPipe, 0xb1 , 0x80);
    gc2145_write_register(ViPipe, 0x05 , 0x01);
    gc2145_write_register(ViPipe, 0x06 , 0x38);
#if 1
    gc2145_write_register(ViPipe, 0x07 , 0x00);
    gc2145_write_register(ViPipe, 0x08 , 0x22);
#endif
    gc2145_write_register(ViPipe, 0x09 , 0x00);
    gc2145_write_register(ViPipe, 0x0a , 0x00);
    gc2145_write_register(ViPipe, 0x0b , 0x00);
    gc2145_write_register(ViPipe, 0x0c , 0x00);
    gc2145_write_register(ViPipe, 0x0d , 0x03);
    gc2145_write_register(ViPipe, 0x0e , 0xd0);
    gc2145_write_register(ViPipe, 0x0f , 0x05);
    gc2145_write_register(ViPipe, 0x10 , 0x10);
    gc2145_write_register(ViPipe, 0x12 , 0x2e);
    gc2145_write_register(ViPipe, 0x17 , 0x14); //mirror
    gc2145_write_register(ViPipe, 0x18 , 0x22);
    gc2145_write_register(ViPipe, 0x19 , 0x0e);
    gc2145_write_register(ViPipe, 0x1a , 0x01);
    gc2145_write_register(ViPipe, 0x1b , 0x4b);
    gc2145_write_register(ViPipe, 0x1c , 0x07);
    gc2145_write_register(ViPipe, 0x1d , 0x10);
    gc2145_write_register(ViPipe, 0x1e , 0x88);
    gc2145_write_register(ViPipe, 0x1f , 0x78);
    gc2145_write_register(ViPipe, 0x20 , 0x03);
    gc2145_write_register(ViPipe, 0x21 , 0x40);
    gc2145_write_register(ViPipe, 0x22 , 0xa0);
    gc2145_write_register(ViPipe, 0x24 , 0x16);
    gc2145_write_register(ViPipe, 0x25 , 0x01);
    gc2145_write_register(ViPipe, 0x26 , 0x10);
    gc2145_write_register(ViPipe, 0x2d , 0x60);
    gc2145_write_register(ViPipe, 0x30 , 0x01);
    gc2145_write_register(ViPipe, 0x31 , 0x90);
    gc2145_write_register(ViPipe, 0x33 , 0x06);
    gc2145_write_register(ViPipe, 0x34 , 0x01);

    /////////////////////////////////////////////////
    //////////////////ISP reg////////////////////
    /////////////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0x80 , 0xff);
    gc2145_write_register(ViPipe, 0x81 , 0x24);
    gc2145_write_register(ViPipe, 0x82 , 0xfa);
    gc2145_write_register(ViPipe, 0x83 , 0x00);
    gc2145_write_register(ViPipe, 0x84 , 0x19);
    gc2145_write_register(ViPipe, 0x86 , 0x02);
    gc2145_write_register(ViPipe, 0x88 , 0x03);
    gc2145_write_register(ViPipe, 0x89 , 0x03);
    gc2145_write_register(ViPipe, 0x85 , 0x30);
    gc2145_write_register(ViPipe, 0x8a , 0x00);
    gc2145_write_register(ViPipe, 0x8b , 0x00);
    gc2145_write_register(ViPipe, 0xb0 , 0x55);
    gc2145_write_register(ViPipe, 0xc3 , 0x00);
    gc2145_write_register(ViPipe, 0xc4 , 0x80);
    gc2145_write_register(ViPipe, 0xc5 , 0x90);
    gc2145_write_register(ViPipe, 0xc6 , 0x38);
    gc2145_write_register(ViPipe, 0xc7 , 0x40);
    gc2145_write_register(ViPipe, 0xec , 0x06);
    gc2145_write_register(ViPipe, 0xed , 0x04);
    gc2145_write_register(ViPipe, 0xee , 0x60);
    gc2145_write_register(ViPipe, 0xef , 0x90);
    gc2145_write_register(ViPipe, 0xb6 , 0x00);
    gc2145_write_register(ViPipe, 0x90 , 0x01);
    gc2145_write_register(ViPipe, 0x91 , 0x00);
    gc2145_write_register(ViPipe, 0x92 , 0x00);
    gc2145_write_register(ViPipe, 0x93 , 0x00);
    gc2145_write_register(ViPipe, 0x94 , 0x00);
    gc2145_write_register(ViPipe, 0x95 , 0x03);
    gc2145_write_register(ViPipe, 0x96 , 0xc0);
    gc2145_write_register(ViPipe, 0x97 , 0x05);
    gc2145_write_register(ViPipe, 0x98 , 0x00);

    /////////////////////////////////////////
    /////////// BLK ////////////////////////
    /////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0x18 , 0x02);
    gc2145_write_register(ViPipe, 0x40 , 0x42);
    gc2145_write_register(ViPipe, 0x41 , 0x00);
    gc2145_write_register(ViPipe, 0x43 , 0x54);
    gc2145_write_register(ViPipe, 0x5e , 0x00);
    gc2145_write_register(ViPipe, 0x5f , 0x00);
    gc2145_write_register(ViPipe, 0x60 , 0x00);
    gc2145_write_register(ViPipe, 0x61 , 0x00);
    gc2145_write_register(ViPipe, 0x62 , 0x00);
    gc2145_write_register(ViPipe, 0x63 , 0x00);
    gc2145_write_register(ViPipe, 0x64 , 0x00);
    gc2145_write_register(ViPipe, 0x65 , 0x00);
    gc2145_write_register(ViPipe, 0x66 , 0x20);
    gc2145_write_register(ViPipe, 0x67 , 0x20);
    gc2145_write_register(ViPipe, 0x68 , 0x20);
    gc2145_write_register(ViPipe, 0x69 , 0x20);
    gc2145_write_register(ViPipe, 0x76 , 0x00);
    gc2145_write_register(ViPipe, 0x6a , 0x08);
    gc2145_write_register(ViPipe, 0x6b , 0x08);
    gc2145_write_register(ViPipe, 0x6c , 0x08);
    gc2145_write_register(ViPipe, 0x6d , 0x08);
    gc2145_write_register(ViPipe, 0x6e , 0x08);
    gc2145_write_register(ViPipe, 0x6f , 0x08);
    gc2145_write_register(ViPipe, 0x70 , 0x08);
    gc2145_write_register(ViPipe, 0x71 , 0x08);
    gc2145_write_register(ViPipe, 0x76 , 0x00);
    gc2145_write_register(ViPipe, 0x72 , 0xf0);
    gc2145_write_register(ViPipe, 0x7e , 0x3c);
    gc2145_write_register(ViPipe, 0x7f , 0x00);
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0x48 , 0x15);
    gc2145_write_register(ViPipe, 0x49 , 0x00);
    gc2145_write_register(ViPipe, 0x4b , 0x0b);
    gc2145_write_register(ViPipe, 0xfe , 0x00);

    ////////////////////////////////////////
    /////////// AEC ////////////////////////
    ////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x01);
    gc2145_write_register(ViPipe, 0x01 , 0x04);
    gc2145_write_register(ViPipe, 0x02 , 0xc0);
    gc2145_write_register(ViPipe, 0x03 , 0x04);
    gc2145_write_register(ViPipe, 0x04 , 0x90);
    gc2145_write_register(ViPipe, 0x05 , 0x30);
    gc2145_write_register(ViPipe, 0x06 , 0x90);
    gc2145_write_register(ViPipe, 0x07 , 0x20);
    gc2145_write_register(ViPipe, 0x08 , 0x70);
    gc2145_write_register(ViPipe, 0x09 , 0x00);
    gc2145_write_register(ViPipe, 0x0a , 0xc2);
    gc2145_write_register(ViPipe, 0x0b , 0x11);
    gc2145_write_register(ViPipe, 0x0c , 0x10);
    gc2145_write_register(ViPipe, 0x13 , 0x40);
    gc2145_write_register(ViPipe, 0x17 , 0x00);
    gc2145_write_register(ViPipe, 0x1c , 0x11);
    gc2145_write_register(ViPipe, 0x1e , 0x61);
    gc2145_write_register(ViPipe, 0x1f , 0x30);
    gc2145_write_register(ViPipe, 0x20 , 0x40);
    gc2145_write_register(ViPipe, 0x22 , 0x80);
    gc2145_write_register(ViPipe, 0x23 , 0x20);
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0x0f , 0x04);
    gc2145_write_register(ViPipe, 0xfe , 0x01);
    gc2145_write_register(ViPipe, 0x12 , 0x35);
    gc2145_write_register(ViPipe, 0x15 , 0x50);
    gc2145_write_register(ViPipe, 0x10 , 0x31);
    gc2145_write_register(ViPipe, 0x3e , 0x28);
    gc2145_write_register(ViPipe, 0x3f , 0xe0);
    gc2145_write_register(ViPipe, 0x40 , 0x20);
    gc2145_write_register(ViPipe, 0x41 , 0x0f);

    /////////////////////////////
    //////// INTPEE /////////////
    /////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0x90 , 0x6c);
    gc2145_write_register(ViPipe, 0x91 , 0x03);
    gc2145_write_register(ViPipe, 0x92 , 0xc8);
    gc2145_write_register(ViPipe, 0x94 , 0x66);
    gc2145_write_register(ViPipe, 0x95 , 0xb5);
    gc2145_write_register(ViPipe, 0x97 , 0x64);
    gc2145_write_register(ViPipe, 0xa2 , 0x11);
    gc2145_write_register(ViPipe, 0xfe , 0x00);

    /////////////////////////////
    //////// DNDD///////////////
    /////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0x80 , 0xc1);
    gc2145_write_register(ViPipe, 0x81 , 0x08);
    gc2145_write_register(ViPipe, 0x82 , 0x08);
    gc2145_write_register(ViPipe, 0x83 , 0x08);
    gc2145_write_register(ViPipe, 0x84 , 0x0a);
    gc2145_write_register(ViPipe, 0x86 , 0xf0);
    gc2145_write_register(ViPipe, 0x87 , 0x50);
    gc2145_write_register(ViPipe, 0x88 , 0x15);
    gc2145_write_register(ViPipe, 0x89 , 0x50);
    gc2145_write_register(ViPipe, 0x8a , 0x30);
    gc2145_write_register(ViPipe, 0x8b , 0x10);

    /////////////////////////////////////////
    /////////// ASDE ////////////////////////
    /////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x01);
    gc2145_write_register(ViPipe, 0x21 , 0x14);
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0xa3 , 0x40);
    gc2145_write_register(ViPipe, 0xa4 , 0x20);
    gc2145_write_register(ViPipe, 0xa5 , 0x40);
    gc2145_write_register(ViPipe, 0xa6 , 0x80);
    gc2145_write_register(ViPipe, 0xab , 0x40);
    gc2145_write_register(ViPipe, 0xae , 0x0c);
    gc2145_write_register(ViPipe, 0xb3 , 0x34);
    gc2145_write_register(ViPipe, 0xb4 , 0x44);
    gc2145_write_register(ViPipe, 0xb6 , 0x38);
    gc2145_write_register(ViPipe, 0xb7 , 0x02);
    gc2145_write_register(ViPipe, 0xb9 , 0x30);
    gc2145_write_register(ViPipe, 0x3c , 0x08);
    gc2145_write_register(ViPipe, 0x3d , 0x30);
    gc2145_write_register(ViPipe, 0x4b , 0x0d);
    gc2145_write_register(ViPipe, 0x4c , 0x20);
    gc2145_write_register(ViPipe, 0xfe , 0x00);

    ///////////////////////////////////////////////
    ///////////YCP ///////////////////////
    ///////////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0xd1 , 0x30);
    gc2145_write_register(ViPipe, 0xd2 , 0x30);
    gc2145_write_register(ViPipe, 0xd3 , 0x45);
    gc2145_write_register(ViPipe, 0xdd , 0x14);
    gc2145_write_register(ViPipe, 0xde , 0x86);
    gc2145_write_register(ViPipe, 0xed , 0x01);
    gc2145_write_register(ViPipe, 0xee , 0x28);
    gc2145_write_register(ViPipe, 0xef , 0x30);
    gc2145_write_register(ViPipe, 0xd8 , 0xd8);

    //////////////////////////////////////
    ///////////  OUTPUT   ////////////////
    //////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe, 0x00);
    gc2145_write_register(ViPipe, 0xf2, 0x00);

    //////////////frame rate 50Hz/////////
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0xf7 , 0x1d);
    gc2145_write_register(ViPipe, 0xf8 , 0x84);
    gc2145_write_register(ViPipe, 0x03 , 0x04);
    gc2145_write_register(ViPipe, 0x04 , 0xe2);
    gc2145_write_register(ViPipe, 0x05 , 0x01);
    gc2145_write_register(ViPipe, 0x06 , 0x32);
    gc2145_write_register(ViPipe, 0x07 , 0x00);
    gc2145_write_register(ViPipe, 0x08 , 0x22);
    gc2145_write_register(ViPipe, 0xfe , 0x01);
    gc2145_write_register(ViPipe, 0x25 , 0x00);
    gc2145_write_register(ViPipe, 0x26 , 0x96);
    gc2145_write_register(ViPipe, 0x27 , 0x04);
    gc2145_write_register(ViPipe, 0x28 , 0xb0);
    gc2145_write_register(ViPipe, 0x29 , 0x04);
    gc2145_write_register(ViPipe, 0x2a , 0xb0);
    gc2145_write_register(ViPipe, 0x2b , 0x04);
    gc2145_write_register(ViPipe, 0x2c , 0xb0);
    gc2145_write_register(ViPipe, 0x2d , 0x04);
    gc2145_write_register(ViPipe, 0x2e , 0xb0);
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0x3c , 0x00);
    gc2145_write_register(ViPipe, 0xfe , 0x00);

    ///////////////dark sun////////////////////
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    gc2145_write_register(ViPipe, 0x18 , 0x22);
    gc2145_write_register(ViPipe, 0xfe , 0x02);
    gc2145_write_register(ViPipe, 0x40 , 0xbf);
    gc2145_write_register(ViPipe, 0x46 , 0xcf);
    gc2145_write_register(ViPipe, 0xfe , 0x00);
    /////////////////////////////////////////////////////
    //////////////////////   MIPI   /////////////////////
    /////////////////////////////////////////////////////
    gc2145_write_register(ViPipe, 0xfe, 0x03);
    gc2145_write_register(ViPipe, 0x01, 0x07);
    gc2145_write_register(ViPipe, 0x02, 0x22);
    gc2145_write_register(ViPipe, 0x03, 0x10);
    gc2145_write_register(ViPipe, 0x04, 0x90);
    gc2145_write_register(ViPipe, 0x05, 0x01);
    gc2145_write_register(ViPipe, 0x06, 0x88);

    gc2145_write_register(ViPipe, 0x10, 0x91);
    gc2145_write_register(ViPipe, 0x11, 0x2b);
    gc2145_write_register(ViPipe, 0x12, 0x40);
    gc2145_write_register(ViPipe, 0x13, 0x06);
    gc2145_write_register(ViPipe, 0x15, 0x12);
    gc2145_write_register(ViPipe, 0x17, 0xf1);
    gc2145_write_register(ViPipe, 0xfe, 0x00);

    printf("Func:%s, Line:%d, Date:%s, Time:%s, end!\n", __FUNCTION__, __LINE__, __DATE__, __TIME__);

    return;
}

