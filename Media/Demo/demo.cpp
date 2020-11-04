#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "demo.hpp"
#include "json.h"

using namespace std;

int GetConfigParam(const char *pData, CONFIG_PARAM_S *pConfigParam)
{
    unsigned int i = 0;
    string streams;
    string s1;
    ifstream iFile;
    unsigned int lcdType = 0;
    unsigned int encType = 0;

    if ((pData == NULL) || (pConfigParam == NULL))
    {
        std::cout << "invalid input param" << std::endl;
        return -1;
    }

    /* 从配置文件中读取json串 */
    iFile.open(pData, ios::in);
    if (!iFile)
    {
        std::cout << "open file error" << std::endl;
        return  -1;
    }

    while(!iFile.eof())
    {
        iFile >> s1;
        streams += s1;
    }

    iFile.close();

    /* 解析json字符串 */
    Json::Value root;
    Json::Reader read;
    Json::Value DISP_PARAM;
    Json::Value ENCODE_PARAM;
    Json::Value SENSOR_PARAM;

    read.parse(streams, root);

    std::cout << root["SENSOR_TYPE"] << std::endl;

    /* 获取基础配置信息 */
    lcdType = root["LCD_TYPE"].asInt();
    encType = root["ENC_TYPE"].asInt();

    pConfigParam->stInitParam.stSensorParam.sensorNumber = root["SENSOR_NUM"].asInt();
    pConfigParam->stInitParam.enLcdIdx = (MEDIA_LCD_IDX_E)lcdType;

    SENSOR_PARAM = root["SENSOR_PARAM"];

    for (i = 0; i < pConfigParam->stInitParam.stSensorParam.sensorNumber; i++)
    {
        pConfigParam->stInitParam.stSensorParam.enSensorIdx[i] = (VI_SNS_TYPE_E)SENSOR_PARAM[i]["sensorIdx"].asInt();
    }

    /* 显示配置信息 */
    DISP_PARAM = root["DISP_PARAM"];

    pConfigParam->stDispParam.stRect.x = DISP_PARAM[lcdType]["x"].asInt();
    pConfigParam->stDispParam.stRect.y = DISP_PARAM[lcdType]["y"].asInt();
    pConfigParam->stDispParam.stRect.w = DISP_PARAM[lcdType]["w"].asInt();
    pConfigParam->stDispParam.stRect.h = DISP_PARAM[lcdType]["h"].asInt();

    pConfigParam->stDispParam.bOpen = true;
    pConfigParam->stDispParam.enRotation = MEDIA_ROTATION_0;

    /* 编码配置信息 */
    ENCODE_PARAM = root["ENCODE_PARAM"];
    pConfigParam->stRecordParam.streamType = STREAM_TYPE_VIDEO;     /* 默认视频流 */
    pConfigParam->stRecordParam.stVideoParam.w = ENCODE_PARAM[encType]["w"].asInt();
    pConfigParam->stRecordParam.stVideoParam.h = ENCODE_PARAM[encType]["h"].asInt();
    pConfigParam->stRecordParam.stVideoParam.fps = ENCODE_PARAM[encType]["fps"].asInt();
    pConfigParam->stRecordParam.stVideoParam.gop = ENCODE_PARAM[encType]["gop"].asInt();
    pConfigParam->stRecordParam.stVideoParam.bitRate = ENCODE_PARAM[encType]["bitRate"].asInt();
    pConfigParam->stRecordParam.stVideoParam.bitType = 0;

    /* 还需要设置开启关闭状态和编码类型 */
    pConfigParam->stRecordParam.stVideoParam.bOpen = true;          /* 编码功能默认开启 */

    if ((ENCODE_PARAM[encType]["type"] == "H265") == 0)
    {
        /* H265 */
        pConfigParam->stRecordParam.stVideoParam.enType = ENCODE_TYPE_H265;
    }
    else if ((ENCODE_PARAM[encType]["type"] == "H264") == 0)
    {
        /* H264 */
        pConfigParam->stRecordParam.stVideoParam.enType = ENCODE_TYPE_H264;
    }
    else if ((ENCODE_PARAM[encType]["type"] == "JPEG") == 0)
    {
        pConfigParam->stRecordParam.stVideoParam.enType = ENCODE_TYPE_JPEG;
    }
    else
    {
        pConfigParam->stRecordParam.stVideoParam.enType = ENCODE_TYPE_H264;
    }

    return 0;
}

int main()
{
    int s32Ret = 0;
    unsigned int countTimes = 0;
    CONFIG_PARAM_S stConfigParam = {0};

    /* 从配置文件中获取配置参数 */
    s32Ret = GetConfigParam("/opt/config.json", &stConfigParam);

    /* 初始化DSP模块 */
    s32Ret = DSP_Init();
    if (0 != s32Ret)
    {
        printf("DSP_Init error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 设置媒体参数 */
    s32Ret = DSP_SetMedia(&stConfigParam.stInitParam);
    if (0 != s32Ret)
    {
        printf("DSP_SetMedia error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 设置显示相关区域 */
    s32Ret = DSP_SetVideoDisp(SENSOR_TYPE_RGB, &stConfigParam.stDispParam);
    if (0 != s32Ret)
    {
        printf("DSP_SetVideoDisp error! s32Ret = %#x\n", s32Ret);
        return -1;
    }

    /* 开启RGB sensor的录像功能 */
    s32Ret = DSP_SetRecord(SENSOR_TYPE_RGB, VIDEO_STREAM_MAIN, &stConfigParam.stRecordParam);
    if (0 != s32Ret)
    {
        printf("DSP_SetRecord error! s32Ret = %#x\n", s32Ret);
    }

    while (1)
    {
        usleep(100 * 1000);

        countTimes ++;

        if (countTimes % 200 == 0)
        {
            //printf("goto sleep!\n");
        }
    }

    return 0;
}

