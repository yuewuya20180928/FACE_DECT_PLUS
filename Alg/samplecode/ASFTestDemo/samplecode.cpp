#include "arcsoft_face_sdk.h"
#include "amcomdef.h"
#include "asvloffscreen.h"
#include "merror.h"
#include <iostream>  
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace std;

#define APPID  "BxV8LNusqXi2NJbcRzXC2cBwY3xUt95JwGwx1E1kvhC8"
#define SDKKEY "EK9DCw5txTGAu25jbmyUuYc2LuJ7fKGi11T55eZ4ytyC"

#define NSCALE 16 
#define FACENUM	5

#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }
#define SafeArrayDelete(p) { if ((p)) delete [] (p); (p) = NULL; } 
#define SafeDelete(p) { if ((p)) delete (p); (p) = NULL; } 

//将时间戳转换成时间
void timestampToTime(char* timeStamp, char* dateTime, int dateTimeSize)
{
	time_t tTimeStamp = atoll(timeStamp);
	struct tm* pTm = gmtime(&tTimeStamp);
	strftime(dateTime, dateTimeSize, "%Y-%m-%d %H:%M:%S", pTm);
}

//色彩空间转换
int ColorSpaceConversion(MInt32 width, MInt32 height, MInt32 format, MUInt8* imgData, ASVLOFFSCREEN& offscreen)
{
	offscreen.u32PixelArrayFormat = (unsigned int)format;
	offscreen.i32Width = width;
	offscreen.i32Height = height;
	
	switch (offscreen.u32PixelArrayFormat)
	{
		case ASVL_PAF_RGB24_B8G8R8:
			offscreen.pi32Pitch[0] = offscreen.i32Width * 3;
			offscreen.ppu8Plane[0] = imgData;
			break;
		case ASVL_PAF_I420:
			offscreen.pi32Pitch[0] = width;
			offscreen.pi32Pitch[1] = width >> 1;
			offscreen.pi32Pitch[2] = width >> 1;
			offscreen.ppu8Plane[0] = imgData;
			offscreen.ppu8Plane[1] = offscreen.ppu8Plane[0] + offscreen.i32Height*offscreen.i32Width;
			offscreen.ppu8Plane[2] = offscreen.ppu8Plane[0] + offscreen.i32Height*offscreen.i32Width * 5 / 4;
			break;
		case ASVL_PAF_NV12:
		case ASVL_PAF_NV21:
			offscreen.pi32Pitch[0] = offscreen.i32Width;
			offscreen.pi32Pitch[1] = offscreen.pi32Pitch[0];
			offscreen.ppu8Plane[0] = imgData;
			offscreen.ppu8Plane[1] = offscreen.ppu8Plane[0] + offscreen.pi32Pitch[0] * offscreen.i32Height;
			break;
		case ASVL_PAF_YUYV:
		case ASVL_PAF_DEPTH_U16:
			offscreen.pi32Pitch[0] = offscreen.i32Width * 2;
			offscreen.ppu8Plane[0] = imgData;
			break;
		case ASVL_PAF_GRAY:
			offscreen.pi32Pitch[0] = offscreen.i32Width;
			offscreen.ppu8Plane[0] = imgData;
			break;
		default:
			printf("unsupported u32PixelArrayFormat = %d\n", offscreen.u32PixelArrayFormat);
			return 0;
	}
	return 1;
}


int main()
{
	printf("\n************* ArcFace SDK Info *****************\n");
	MRESULT res = MOK;
	unsigned int i = 0;
	ASF_ActiveFileInfo activeFileInfo = { 0 };
	res = ASFGetActiveFileInfo(&activeFileInfo);
	if (res != MOK)
	{
		printf("ASFGetActiveFileInfo fail: %ld\n", res);
	}
	else
	{
		//打印时间差
		char startDateTime[32];
		timestampToTime(activeFileInfo.startTime, startDateTime, 32);
		printf("startTime: %s\n", startDateTime);
		char endDateTime[32];
		timestampToTime(activeFileInfo.endTime, endDateTime, 32);
		printf("endTime: %s\n", endDateTime);
	}

	//信息打印
	const ASF_VERSION version = ASFGetVersion();
	printf("\nVersion:%s\n", version.Version);
	printf("BuildDate:%s\n", version.BuildDate);
	printf("CopyRight:%s\n", version.CopyRight);

	printf("\n************* Face Recognition *****************\n");

	res = ASFOnlineActivation(APPID, SDKKEY);
	if (MOK != res && MERR_ASF_ALREADY_ACTIVATED != res)
	{
		printf("ASFOnlineActivation fail: %ld\n", res);
	}
	else
	{
		printf("ASFOnlineActivation sucess: %ld\n", res);
	}

	//初始化引擎模块ASF_LIVENESS
	MHandle handle = NULL;
	MInt32 mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_IR_LIVENESS;
	res = ASFInitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, NSCALE, FACENUM, mask, &handle);
	if (res != MOK)
		printf("ASFInitEngine fail: %ld\n", res);
	else
		printf("ASFInitEngine sucess: %ld\n", res);

	//测试1.nv21
	#if 0
	char picPath1[64] = {0};
	snprintf(picPath1, 64, "./images/640x480_1.NV21");
	int Width1 = 640;
	int Height1 = 480;
	int Format1 = ASVL_PAF_NV21;
	MUInt8* imageData1 = (MUInt8*)malloc(Height1*Width1*3/2);
	FILE* fp1 = fopen(picPath1, "rb");
	#else
	char picPath1[64] = {0};
	snprintf(picPath1, 64, "./images/1212x1406.NV21");
	int Width1 = 1212;
	int Height1 = 1406;
	int Format1 = ASVL_PAF_NV21;
	MUInt8* imageData1 = (MUInt8*)malloc(Height1*Width1*3/2);
	FILE* fp1 = fopen(picPath1, "rb");
	#endif

	//测试2.nv21
	char picPath2[64] = {0};
	snprintf(picPath2, 64, "./images/640x480_2.NV21");
	int Width2 = 640;
	int Height2 = 480;
	int Format2 = ASVL_PAF_NV21;
	MUInt8* imageData2 = (MUInt8*)malloc(Height1*Width1*3/2);
	FILE* fp2 = fopen(picPath2, "rb");

	//测试3.nv21
	char picPath3[64] = {0};
	snprintf(picPath3, 64, "./images/640x480_3.NV21");
	int Width3 = 640;
	int Height3 = 480;
	int Format3 = ASVL_PAF_GRAY;
	MUInt8* imageData3 = (MUInt8*)malloc(Height2*Width2);
	FILE* fp3 = fopen(picPath3, "rb");

	if (fp1 && fp2 && fp3)
	{
		fread(imageData1, 1, Height1*Width1*3/2, fp1);
		fclose(fp1);
		fread(imageData2, 1, Height1*Width1*3/2, fp2);
		fclose(fp2);
		fread(imageData3, 1, Height3*Width3, fp3);
		fclose(fp3);

		ASVLOFFSCREEN offscreen1 = { 0 };
		ColorSpaceConversion(Width1, Height1, ASVL_PAF_NV21, imageData1, offscreen1);

		ASF_MultiFaceInfo detectedFaces1 = { 0 };
		ASF_SingleFaceInfo SingleDetectedFaces = { 0 };
		ASF_FaceFeature feature1 = { 0 };
		ASF_FaceFeature copyfeature1 = { 0 };

		/* face detect */
		res = ASFDetectFacesEx(handle, &offscreen1, &detectedFaces1);
		if (res != MOK && detectedFaces1.faceNum > 0)
		{
			printf("%s ASFDetectFaces 1 fail: %ld\n", picPath1, res);
		}
		else
		{
			SingleDetectedFaces.faceRect.left = detectedFaces1.faceRect[0].left;
			SingleDetectedFaces.faceRect.top = detectedFaces1.faceRect[0].top;
			SingleDetectedFaces.faceRect.right = detectedFaces1.faceRect[0].right;
			SingleDetectedFaces.faceRect.bottom = detectedFaces1.faceRect[0].bottom;
			SingleDetectedFaces.faceOrient = detectedFaces1.faceOrient[0];

			for (i = 0; i < detectedFaces1.faceNum; i++)
			{
				printf("Func:%s, Line:%d, faceNum = %d, i = %d, face area! (%d, %d)->(%d, %d)\n",
					__FUNCTION__,
					__LINE__,
					detectedFaces1.faceNum,
					i,
					detectedFaces1.faceRect[i].left,
					detectedFaces1.faceRect[i].top,
					detectedFaces1.faceRect[i].right,
					detectedFaces1.faceRect[i].bottom);
			}

			/* face feature extract */
			res = ASFFaceFeatureExtractEx(handle, &offscreen1, &SingleDetectedFaces, &feature1);
			if (res != MOK)
			{
				printf("%s ASFFaceFeatureExtractEx 1 fail: %ld\n", picPath1, res);
			}
			else
			{
				copyfeature1.featureSize = feature1.featureSize;
				copyfeature1.feature = (MByte *)malloc(feature1.featureSize);
				memset(copyfeature1.feature, 0, feature1.featureSize);
				memcpy(copyfeature1.feature, feature1.feature, feature1.featureSize);
			}
		}
		
		ASVLOFFSCREEN offscreen2 = { 0 };
		ColorSpaceConversion(Width2, Height2, ASVL_PAF_NV21, imageData2, offscreen2);
		
		ASF_MultiFaceInfo detectedFaces2 = { 0 };
		ASF_FaceFeature feature2 = { 0 };
		
		/* 人脸检测 */
		res = ASFDetectFacesEx(handle, &offscreen2, &detectedFaces2);
		if (res != MOK && detectedFaces2.faceNum > 0)
		{
			printf("%s ASFDetectFacesEx 2 fail: %ld\n", picPath2, res);
		}
		else
		{
			SingleDetectedFaces.faceRect.left = detectedFaces2.faceRect[0].left;
			SingleDetectedFaces.faceRect.top = detectedFaces2.faceRect[0].top;
			SingleDetectedFaces.faceRect.right = detectedFaces2.faceRect[0].right;
			SingleDetectedFaces.faceRect.bottom = detectedFaces2.faceRect[0].bottom;
			SingleDetectedFaces.faceOrient = detectedFaces2.faceOrient[0];
			for (i = 0; i < detectedFaces2.faceNum; i++)
			{
				printf("Func:%s, Line:%d, faceNum = %d, i = %d, face area! (%d, %d)->(%d, %d)\n",
					__FUNCTION__,
					__LINE__,
					detectedFaces2.faceNum,
					i,
					detectedFaces2.faceRect[i].left,
					detectedFaces2.faceRect[i].top,
					detectedFaces2.faceRect[i].right,
					detectedFaces2.faceRect[i].bottom);
			}

			res = ASFFaceFeatureExtractEx(handle, &offscreen2, &SingleDetectedFaces, &feature2);
			if (res != MOK)
			{
				printf("%s ASFFaceFeatureExtractEx 2 fail: %ld\n", picPath2, res);
			}
			else
			{
				printf("%s ASFFaceFeatureExtractEx 2 sucess: %ld\n", picPath2, res);
			}
		}

		MFloat confidenceLevel;
		/* 人脸特征比对 */
		res = ASFFaceFeatureCompare(handle, &copyfeature1, &feature2, &confidenceLevel);
		if (res != MOK)
		{
			printf("ASFFaceFeatureCompare fail: %ld\n", res);
		}
		else
		{
			printf("ASFFaceFeatureCompare sucess: %lf\n", confidenceLevel);
		}

		printf("\n************* Face Process *****************\n");
		ASF_LivenessThreshold threshold = { 0 };
		threshold.thresholdmodel_BGR = 0.5;
		threshold.thresholdmodel_IR = 0.7;
		res = ASFSetLivenessParam(handle, &threshold);
		if (res != MOK)
		{
			printf("ASFSetLivenessParam fail: %ld\n", res);
		}
		else
		{
			printf("RGB Threshold: %f\nIR Threshold: %f\n", threshold.thresholdmodel_BGR, threshold.thresholdmodel_IR);
		}

		//
		MInt32 processMask = ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS;
		res = ASFProcessEx(handle, &offscreen2, &detectedFaces2, processMask);
		if (res != MOK)
		{
			printf("ASFProcessEx fail: %ld\n", res);
		}
		else
		{
			printf("ASFProcessEx sucess: %ld\n", res);
		}

		ASF_AgeInfo ageInfo = { 0 };
		res = ASFGetAge(handle, &ageInfo);
		if (res != MOK)
		{
			printf("%s ASFGetAge fail: %ld\n", picPath2, res);
		}
		else
		{
			printf("%s First face age: %d\n", picPath2, ageInfo.ageArray[0]);
		}

		//
		ASF_GenderInfo genderInfo = { 0 };
		res = ASFGetGender(handle, &genderInfo);
		if (res != MOK)
		{
			printf("%s ASFGetGender fail: %ld\n", picPath2, res);
		}
		else
		{
			printf("%s First face gender: %d\n", picPath2, genderInfo.genderArray[0]);
		}

		//
		ASF_Face3DAngle angleInfo = { 0 };
		res = ASFGetFace3DAngle(handle, &angleInfo);
		if (res != MOK)
		{
			printf("%s ASFGetFace3DAngle fail: %ld\n", picPath2, res);
		}
		else
		{
			printf("%s First face 3dAngle: roll: %lf yaw: %lf pitch: %lf\n", picPath2, angleInfo.roll[0], angleInfo.yaw[0], angleInfo.pitch[0]);
		}

		//
		ASF_LivenessInfo rgbLivenessInfo = { 0 };
		res = ASFGetLivenessScore(handle, &rgbLivenessInfo);
		if (res != MOK)
		{
			printf("ASFGetLivenessScore fail: %ld\n", res);
		}
		else
		{
			printf("ASFGetLivenessScore sucess: %d\n", rgbLivenessInfo.isLive[0]);
		}

		printf("\n**********IR LIVENESS*************\n");

		//
		ASVLOFFSCREEN offscreen3 = { 0 };
		ColorSpaceConversion(Width3, Height3, ASVL_PAF_GRAY, imageData3, offscreen3);

		ASF_MultiFaceInfo detectedFaces3 = { 0 };
		res = ASFDetectFacesEx(handle, &offscreen3, &detectedFaces3);
		if (res != MOK)
		{
			printf("ASFDetectFacesEx fail: %ld\n", res);
		}
		else
		{
			printf("Face num: %d\n", detectedFaces3.faceNum);
		}

		//
		MInt32 processIRMask = ASF_IR_LIVENESS;
		res = ASFProcessEx_IR(handle, &offscreen3, &detectedFaces3, processIRMask);
		if (res != MOK)
		{
			printf("ASFProcessEx_IR fail: %ld\n", res);
		}
		else
		{
			printf("ASFProcessEx_IR sucess: %ld\n", res);
		}

		//
		ASF_LivenessInfo irLivenessInfo = { 0 };
		res = ASFGetLivenessScore_IR(handle, &irLivenessInfo);
		if (res != MOK)
		{
			printf("ASFGetLivenessScore_IR fail: %ld\n", res);
		}
		else
		{
			printf("IR Liveness: %d\n", irLivenessInfo.isLive[0]);
		}

		//
		SafeFree(copyfeature1.feature);
		SafeArrayDelete(imageData1);
		SafeArrayDelete(imageData2);
		SafeArrayDelete(imageData3);

		//
		res = ASFUninitEngine(handle);
		if (res != MOK)
		{
			printf("ASFUninitEngine fail: %ld\n", res);
		}
		else
		{
			printf("ASFUninitEngine sucess: %ld\n", res);
		}
	}
	else
	{
		printf("No pictures found.\n");
	}

	printf("finished demo!\n");

    return 0;
}

