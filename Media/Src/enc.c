#include "enc.h"
#include "media_com.h"

static ENC_PARAM_S stEncParam[MAX_ENC_CHAN];

void *Tsk_Encode(void *pParam)
{
    int s32Ret = HI_SUCCESS;
    unsigned int encChan = 0xFFFF;
    int encFd = 0;
    int maxFd = -1;
    fd_set read_fds;
    struct timeval TimeoutVal;
    VENC_CHN_STATUS_S stStat = {0};
    VENC_STREAM_S stStream = {0};

    char tskName[128] = {0};

    encChan = *((unsigned int*)pParam);

    prtMD("encChan = %d\n", encChan);

    snprintf(tskName, 128, "TSK_ENC_%d", encChan);

    prctl(PR_SET_NAME, tskName, 0,0,0);

    encFd = HI_MPI_VENC_GetFd(encChan);
    if (encFd < 0)
    {
        prtMD("HI_MPI_VENC_GetFd error! encFd = %d, return\n", encFd);
        return NULL;
    }

    if (maxFd <= encFd)
    {
        maxFd = encFd;
    }

    prtMD("mxaFd = %d\n", maxFd);

    while (1)
    {
        if (stEncParam[encChan].bOpen == false)
        {
            usleep(20 * 1000);
            continue;
        }


        FD_ZERO(&read_fds);
        FD_SET(encFd, &read_fds);

        TimeoutVal.tv_sec = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            continue;
        }
        else if (s32Ret == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(encFd, &read_fds))
            {
                /*******************************************************
                 step 2.1 : query how many packs in one-frame stream.
                *******************************************************/
                memset(&stStream, 0, sizeof(stStream));

                s32Ret = HI_MPI_VENC_QueryStatus(encChan, &stStat);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("HI_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", encChan, s32Ret);
                    continue;
                }

                if (0 == stStat.u32CurPacks)
                {
                    prtMD("NOTE: Current frame is NULL!\n");
                    continue;
                }

                /*******************************************************
                 step 2.3 : malloc corresponding number of pack nodes.
                *******************************************************/
                stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    prtMD("malloc stream pack failed!\n");
                    continue;
                }

                /*******************************************************
                 step 2.4 : call mpi to get one-frame stream
                *******************************************************/
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = HI_MPI_VENC_GetStream(encChan, &stStream, HI_TRUE);

                if (HI_SUCCESS != s32Ret)
                {
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    prtMD("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                    continue;
                }

                /*******************************************************
                step 2.6 : release stream
                *******************************************************/

                s32Ret = HI_MPI_VENC_ReleaseStream(encChan, &stStream);
                if (HI_SUCCESS != s32Ret)
                {
                    prtMD("HI_MPI_VENC_ReleaseStream failed!\n");
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    continue;
                }

                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }
}

/* 初始化编码模块 */
int Media_Enc_StartTsk(unsigned int encChan)
{
    int ret = 0;
    pthread_t pThrd;
    unsigned int chan = 0;

    chan = encChan;

    if (stEncParam[chan].bTskCreat == true)
    {
        return 0;
    }

    stEncParam[chan].bTskCreat = true;

    /* 创建线程实时读取编码之后的码流 */
    pthread_create(&pThrd, NULL, Tsk_Encode, (void *)&stEncParam[chan].vencChan);

    return ret;
}

/* 创建编码通道 */
int Media_Enc_CreatChn(unsigned int encChan, VIDEO_PARAM_S *pVideoParam)
{
    int ret = 0;
    PAYLOAD_TYPE_E enType = PT_BUTT;
    VENC_CHN_ATTR_S stVencChnAttr = {0};
    VENC_RC_MODE_E enRcMode = VENC_RC_MODE_BUTT;
    unsigned int profile = 0;
    VENC_H264_CBR_S stH264Cbr = {0};
    VENC_H265_CBR_S stH265Cbr = {0};
    VENC_MJPEG_FIXQP_S stMjpegeFixQp = {0};
    VENC_GOP_ATTR_S stGopAttr = {0};

    if (pVideoParam == NULL)
    {
        prtMD("invald input pVideoParam = %p\n", pVideoParam);
        return -1;
    }

    if (pVideoParam->enType == ENCODE_TYPE_H264)
    {
        enType = PT_H264;
        profile = 0;        //0~3
        enRcMode = VENC_RC_MODE_H264CBR;

        stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        stGopAttr.stNormalP.s32IPQpDelta = 2;

        stVencChnAttr.stRcAttr.enRcMode = enRcMode;
        stH264Cbr.u32Gop = pVideoParam->gop;
        stH264Cbr.u32StatTime = 1;
        stH264Cbr.u32SrcFrameRate = pVideoParam->fps;
        stH264Cbr.fr32DstFrameRate = pVideoParam->fps;
        stH264Cbr.u32BitRate = pVideoParam->bitRate;

        memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(VENC_H264_CBR_S));
    }
    else if (pVideoParam->enType == ENCODE_TYPE_H265)
    {
        enType = PT_H265;
        profile = 0;        //0~1
        enRcMode = VENC_RC_MODE_H265CBR;

        stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        stGopAttr.stNormalP.s32IPQpDelta = 2;

        stVencChnAttr.stRcAttr.enRcMode = enRcMode;
        stH265Cbr.u32Gop = pVideoParam->gop;
        stH265Cbr.u32StatTime = 1;
        stH265Cbr.u32SrcFrameRate = pVideoParam->fps;
        stH265Cbr.fr32DstFrameRate = pVideoParam->fps;
        stH265Cbr.u32BitRate = pVideoParam->bitRate;

        memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(VENC_H265_CBR_S));
    }
    else if (pVideoParam->enType == ENCODE_TYPE_JPEG)
    {
        enType = PT_MJPEG;
        profile = 0;
        enRcMode = VENC_RC_MODE_MJPEGFIXQP;

        stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        stGopAttr.stNormalP.s32IPQpDelta = 0;

        stVencChnAttr.stRcAttr.enRcMode = enRcMode;

        stMjpegeFixQp.u32Qfactor = 95;
        stMjpegeFixQp.u32SrcFrameRate = pVideoParam->fps;
        stMjpegeFixQp.fr32DstFrameRate = pVideoParam->fps;

        memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegeFixQp, sizeof(VENC_MJPEG_FIXQP_S));
    }
    else
    {
        prtMD("unsupported enType = %d\n", pVideoParam->enType);
        return -1;
    }


    memcpy(&stVencChnAttr.stGopAttr, &stGopAttr, sizeof(VENC_GOP_ATTR_S));

    stVencChnAttr.stVencAttr.enType = enType;
    stVencChnAttr.stVencAttr.u32MaxPicWidth = pVideoParam->w;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = pVideoParam->h;
    stVencChnAttr.stVencAttr.u32PicWidth = pVideoParam->w;
    stVencChnAttr.stVencAttr.u32PicHeight = pVideoParam->h;
    stVencChnAttr.stVencAttr.u32BufSize = pVideoParam->w * pVideoParam->h * 2;
    stVencChnAttr.stVencAttr.u32Profile = profile;      /* 编码级别 */
    stVencChnAttr.stVencAttr.bByFrame = HI_TRUE;

    ret = HI_MPI_VENC_CreateChn(encChan, &stVencChnAttr);
    if (HI_SUCCESS != ret)
    {
        prtMD("HI_MPI_VENC_CreateChn error! encChan = %d, ret = %#x\n", encChan, ret);
        return ret;
    }

    return ret;
}

int Media_Enc_StartChn(unsigned int encChan)
{
    int ret = 0;

    VENC_RECV_PIC_PARAM_S stRecvParam = {0};

    stRecvParam.s32RecvPicNum = -1;

    ret = HI_MPI_VENC_StartRecvFrame(encChan, &stRecvParam);
    if (HI_SUCCESS != ret)
    {
        prtMD("HI_MPI_VENC_StartRecvFrame error! ret = %#x\n", ret);
        return ret;
    }

    return ret;
}

/* 设置视频编码参数 */
int Media_Enc_StartVideo(unsigned int encChan, VIDEO_PARAM_S *pVideoParam)
{
    int ret = 0;
    ENC_PARAM_S *pEncParam = NULL;

    if (encChan >= MAX_ENC_CHAN)
    {
        prtMD("unsupported encChan = %d\n", encChan);
        return -1;
    }

    pEncParam = stEncParam + encChan;

    if (pVideoParam == NULL)
    {
        prtMD("invalid input pVideoParam = %p\n", pVideoParam);
        return -1;
    }

    /* 创建编码通道 */
    ret = Media_Enc_CreatChn(encChan, pVideoParam);
    if (0 != ret)
    {
        prtMD("Media_Enc_CreatChn error! ret = %#x\n", ret);
        return ret;
    }

    /* 开启编码通道 */
    ret = Media_Enc_StartChn(encChan);
    if (0 != ret)
    {
        prtMD("Media_Enc_StartChn error! ret = %#x\n", ret);
        return  ret;
    }

    pEncParam->bOpen = true;
    pEncParam->vencChan = encChan;

    return ret;
}

/* 设置音频编码参数 */
int Media_Enc_SetAudio(SENSOR_TYPE_E sensorIdx, VIDEO_STREAM_E videoStreamType, AUDIO_PARAM_S *pAudioParam)
{
    int ret = 0;

    return ret;
}


