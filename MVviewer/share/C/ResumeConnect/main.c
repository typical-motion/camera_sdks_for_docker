/// \file
/// \~chinese
/// \brief ������������
/// \example main.c
/// \~english
/// \brief resume connect sample
/// \example main.c

// *******��DemoΪ����ʾSDK��ʹ�ã�û�и����޸����IP�Ĵ��룬
// ������֮ǰ����ʹ������ͻ����޸����IP��ַ������������������һ��*********    
// ********This demo is a simple demonstration of the use of the SDK. There is no additional code to modify the camera IP.
// Before running, please use the camera client to modify the network segment of the camera IP address to be consistent with that of the host.*********
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "GenICam/CAPI/SDK.h"

#ifdef _WIN32
#define SLEEP(a)	Sleep(a);
#else
#define SLEEP(a)	usleep(a * 1000);
#endif

GENICAM_Camera*			g_pCamera = NULL;
GENICAM_EventSubscribe* g_pEventSubscribe = NULL;
GENICAM_StreamSource*	g_pStreamSource = NULL;// ������ //stream object
GENICAM_System*			g_pSystem = NULL;

int32_t isMainThreadExit = 0;
char pUser[10] = "Camera";

static void onDeviceLinkNotify(const GENICAM_SConnectArg* pConnectArg, void* pUser);
static void onChannelCallback(const GENICAM_SMsgChannelArg* pMsgChannelArg, void* pUser);
static void releaseSource(void);

// ȡ���ص�����
// get frame callback function
static void onGetFrame(GENICAM_Frame* pFrame, void* pUser)
{
	if (0 != pFrame->valid(pFrame))
	{
		printf("frame is invalid!\n");
	}
	else
	{
		printf("pUser:%s  blockId = %lu.\n", (char*)pUser, pFrame->getBlockId(pFrame));
	}

	// ע�⣺ʹ�ø�֡����Ҫ��ʾ�ͷ�
	// Attention: should release pFrame object at the end of use
	pFrame->release(pFrame);
	return;
}


// ���������������
// try reconnecting camera
static void TryToResumeConnect(void)
{
	GENICAM_StreamSourceInfo streamSourceInfo;

	if (g_pCamera == NULL)
	{
		printf("g_pCamera is NULL.\n");
		return;
	}

	// �Ͽ��������
	// disconnect cammera
	if (0 != g_pCamera->disConnect(g_pCamera))
	{
		printf("*******  disConnect failed  *******\n");
	}
	else
	{
		printf("*******  disConnect success  *******\n");
	}

	while (1)
	{
		// �˳�����ʱ�������������
		// When you exit the program, you do not have to connect the camera
		if(isMainThreadExit)
		{
			printf("Main thread has been exited\n");
			return;		
		}

		// �����������
		// reconnecting camera
		if (g_pCamera->connect(g_pCamera, accessPermissionControl) < 0)
		{
			printf("*******  Retry connect fail  *******\n");
			SLEEP(500);
			continue;
		}
		else
		{
			printf("*******  Retry connect success  *******\n");
			break;
		}
	}

	// ���´���������
	// Recreate stream object
	g_pStreamSource->release(g_pStreamSource);
	g_pStreamSource = NULL;
	memset(&streamSourceInfo, 0, sizeof(GENICAM_StreamSourceInfo));
	streamSourceInfo.channelId = 0;
	streamSourceInfo.pCamera = g_pCamera;
	if (0 != GENICAM_createStreamSource(&streamSourceInfo, &g_pStreamSource))
	{
		printf("create stream obj  fail.\n");
		return;
	}

	// ע��ص�
	// register callback functiohn
	if (g_pStreamSource->attachGrabbingEx(g_pStreamSource, onGetFrame, (void*)pUser) < 0)
	{
		printf("*******  attach stream callback fail  *******\n");
	}
	else
	{
		printf("*******  attach stream callback success  *******\n");
	}

	// ��ʼ����
	// start grabbing
	if (g_pStreamSource->startGrabbing(g_pStreamSource, 0, grabStrartegySequential) < 0)
	{
		printf("*******  start grab fail  *******\n");
		return;
	}

	printf("*******  start grab success  *******\n");

	return;
}
// �豸����״̬֪ͨ
// device connection status notification
static void onDeviceLinkNotify(const GENICAM_SConnectArg* pConnectArg, void* pUser)
{
	// ���ֻ��Զ���ʱ�Ĳ�����ֻҪ���յ�offLineʱ������ 
	// if it is only for the operation during disconnection, it can only be processed when receiving camera offline
	if (offLine == pConnectArg->m_event)
	{
		printf("*******  camera is offline  *******\n");

		if (g_pStreamSource == NULL)
		{
			printf("g_pStreamSource is NULL.\n");
			return;
		}

		// ע���ص�
		// unregister callback function
		if (g_pStreamSource->detachGrabbingEx(g_pStreamSource, onGetFrame, (void*)pUser) < 0)
		{
			printf("*******  detach stream callback fail  *******\n");
		}
		else
		{
			printf("*******  detach stream callback success  *******\n");
		}

		// ֹͣ����
		// stop grabbing
		if (g_pStreamSource->stopGrabbing(g_pStreamSource) < 0)
		{
			printf("*******  stop grab fail  *******\n");
		}
		else
		{
			printf("*******  stop grab success  *******\n");
		}
	}
	else if (onLine == pConnectArg->m_event)
	{
		printf("*******  camera is online  *******\n");

		TryToResumeConnect();
	}

	return;
}

// ��Ϣ֪ͨ�ص�����
// message notification callback function
static void onChannelCallback(const GENICAM_SMsgChannelArg* pMsgChannelArg, void* pUser)
{
	if (pMsgChannelArg->eventID == MSG_EVENT_ID_EXPOSURE_END)
	{
		printf("pUser:%s  Exposure End!\n", (char*)pUser);
	}
	else if (pMsgChannelArg->eventID == MSG_EVENT_ID_FRAME_TRIGGER)
	{
		printf("pUser:%s  Frame Trigger!\n", (char*)pUser);
	}
	else if (pMsgChannelArg->eventID == MSG_EVENT_ID_FRAME_START)
	{
		printf("pUser:%s  Frame Start!\n", (char*)pUser);
	}
	else
	{
		printf("eventID %d pUser:%s\n", pMsgChannelArg->eventID, (char*)pUser);
	}

	return;
}
// ��ʾ�豸��Ϣ
// dispaly device information
static void displayDeviceInfo(DeviceInfo *pCameraInfoList, uint32_t cameraCnt)
{
	uint32_t cameraIndex;
	const char *vendorName = NULL;
	char vendorNameCat[11];
	const char* deviceUserID = NULL;
	char deviceUserIDCat[16];

	// ��ӡTitle�� 
	// print title 
	printf("\nIdx Type Vendor     Model      S/N             DeviceUserID    IP Address    \n");
	printf("------------------------------------------------------------------------------\n");

	for (cameraIndex = 0; cameraIndex < cameraCnt; cameraIndex++)
	{
		DeviceInfo *pDeviceInfo = &pCameraInfoList[cameraIndex];

		// Idx �豸�б��������� ����ʾ������3
		// index of  camera device list,display in 3 characters 
		printf("%-3d", cameraIndex + 1);

		// Type ������豸���ͣ�GigE��U3V��CL��PCIe��
		// Device type of camera 
		switch (pDeviceInfo->nType)
		{
		case typeGige:
			printf(" GigE");
			break;
		case typeUsb3:
			printf(" U3V ");
			break;
		case typeCL:
			printf(" CL  ");
			break;
		case typePCIe:
			printf(" PCIe");
			break;
		default:
			printf("     ");
			break;
		}

		// VendorName ��������Ϣ  ����ʾ������10 
		// VendorName manufacturer information  display in 10 characters 
		vendorName = pDeviceInfo->strVendor;
		if (strlen(vendorName) > 10)
		{
			strncpy(vendorNameCat, vendorName, 7);
			vendorNameCat[7] = '\0';
			strcat(vendorNameCat, "...");
			printf(" %-10.10s", vendorNameCat);
		}
		else
		{
			printf(" %-10.10s", vendorName);
		}

		// ModeName ������ͺ���Ϣ ����ʾ������10 
		// ModeName camera model information  display in 10 characters 
		printf(" %-10.10s", pDeviceInfo->strModel);

		// Serial Number ��������к� ����ʾ������15 
		// camera serial Number  display in 15 characters 
		printf(" %-15.15s", pDeviceInfo->strSerialNumber);

		// deviceUserID �Զ����û�ID ����ʾ������15 
		// camera user id, display in 15 characters 
		deviceUserID = pDeviceInfo->strUserDefinedName;
		if (strlen(deviceUserID) > 15)
		{
			strncpy(deviceUserIDCat, deviceUserID, 12);
			deviceUserIDCat[12] = '\0';
			strcat(deviceUserIDCat, "...");
			printf(" %-15.15s", deviceUserIDCat);
		}
		else
		{
			memcpy(deviceUserIDCat, deviceUserID, sizeof(deviceUserIDCat));
			printf(" %-15.15s", deviceUserIDCat);
		}

		// IPAddress GigE���ʱ��ȡIP��ַ 
		// IP address of GigE camera 
		if (pDeviceInfo->nType == typeGige)
		{
			printf(" %s", pDeviceInfo->DeviceSpecificInfo.stGigEInfo.strIPAddr);
		}

		printf("\n");
	}
	return;
}

static uint32_t selectDevice(uint32_t cameraCnt)
{
	uint32_t cameraIndex = 0;
	// ��ʾ�û�ѡ�� 
	// inform user to select
	printf("Please intput the camera index:");
	while (1)
	{
		scanf("%d", &cameraIndex);
		setbuf(stdin, NULL);
		// �ж��û�ѡ��Ϸ��� 
		// judge the validity of user selection
		cameraIndex -= 1;// ��ʾ��Index�Ǵ�1��ʼ english: the index displayed starts from 1
		if ((cameraIndex < 0) || (cameraIndex >= cameraCnt))
		{
			printf("Invaild!\n");
		}
		else
		{
			break;
		}
	}
	return cameraIndex;
}

static void releaseSource(void)
{
	if (g_pEventSubscribe != NULL)
	{
		g_pEventSubscribe->release(g_pEventSubscribe);
		g_pEventSubscribe = NULL;
	}

	if (g_pStreamSource != NULL)
	{
		g_pStreamSource->release(g_pStreamSource);
		g_pStreamSource = NULL;
	}

	if (g_pCamera != NULL)
	{
		g_pCamera->release(g_pCamera);
		g_pCamera = NULL;
	}

	if (g_pSystem != NULL)
	{
		g_pSystem->release(g_pSystem);
		g_pSystem = NULL;
	}

	return;
}

int32_t main(void)
{
	DeviceInfo* pDeviceInfoList = NULL;

	uint32_t cameraCnt = 0;
	int32_t cameraIndex = -1;
	GENICAM_EventSubscribeInfo eventSubscribeInfo;
	GENICAM_StreamSourceInfo streamSourceInfo;

	if (GENICAM_getSystemInstance(&g_pSystem) < 0)
	{
		printf("pSystem is null.\r\n");
		return 0;
	}

	if (g_pSystem->enumDevicesInfo(g_pSystem, &pDeviceInfoList, &cameraCnt, typeAll) < 0)
	{
		printf("enumDevicesInfo fail\n");
		return 0;
	}

	printf("enumDevicesInfo success.  size:[%d]\n", cameraCnt);
	// ��ӡ���������Ϣ�����,����,��������Ϣ,�ͺ�,���к�,�û��Զ���ID,IP��ַ��
	// print camera info (index,Type,vendor name, model,serial number,DeviceUserID,IP Address)
	displayDeviceInfo(pDeviceInfoList, cameraCnt);

	// ѡ����Ҫ���ӵ����
	// Select the camera 
	cameraIndex = selectDevice(cameraCnt);

	// �����豸����
	// create device object
	if (g_pSystem->createDevice(g_pSystem, &pDeviceInfoList[cameraIndex], &g_pCamera) < 0)
	{
		printf("createDevice fail\n");
		return 0;
	}

	printf("createDevice success   CamKey:[%s]\n", g_pCamera->getKey(g_pCamera));

	// �����豸
	// connect device
	if (g_pCamera->connect(g_pCamera, accessPermissionControl) < 0)
	{
		printf("connect camera failed.\n");
		releaseSource();
		return 0;
	}

	memset(&eventSubscribeInfo, 0, sizeof(GENICAM_EventSubscribeInfo));
	eventSubscribeInfo.pCamera = g_pCamera;
	if (GENICAM_createEventSubscribe(&eventSubscribeInfo, &g_pEventSubscribe) < 0)
	{
		printf("create event obj  fail.\n");
		releaseSource();
		return 0;
	}

	if (g_pEventSubscribe->subscribeConnectArgsEx(g_pEventSubscribe, onDeviceLinkNotify, (void*)pUser) < 0)
	{
		printf("subscribe connect event fail.\n");
		releaseSource();
		return 0;
	}

	// ע����Ϣͨ���¼��ص�����
	// register message channel event callback function
	if (g_pEventSubscribe->subscribeMsgChannelEx(g_pEventSubscribe, onChannelCallback, (void*)pUser) < 0)
	{
		printf("subscribe message channel fail.\n");
		releaseSource();
		return 0;
	}

	memset(&streamSourceInfo, 0, sizeof(GENICAM_StreamSourceInfo));
	streamSourceInfo.pCamera = g_pCamera;
	streamSourceInfo.channelId = 0;
	if (GENICAM_createStreamSource(&streamSourceInfo, &g_pStreamSource) < 0)
	{
		printf("create stream obj  fail.\n");
		releaseSource();
		return 0;
	}

	// ע��ȡ���ص�����
	// Register get frame callback function
	if (0 != g_pStreamSource->attachGrabbingEx(g_pStreamSource, onGetFrame, (void*)pUser))
	{
		printf("attch Grabbing fail.\n");
		releaseSource();
		return 0;
	}

	// ��ʼ����
	// start grabbing
	if (0 != g_pStreamSource->startGrabbing(g_pStreamSource, 0, grabStrartegySequential))
	{
		printf("StartGrabbing  fail.\n");
		releaseSource();
		return 0;
	}

	// ���̵߳ȴ�״̬
	// main thread wait status
	getchar();

	// ��ע����Ϣͨ���¼��ص�����
	// unregister message channel event callback function
	if (g_pEventSubscribe->unsubscribeMsgChannelEx(g_pEventSubscribe, onChannelCallback, (void*)pUser) < 0)
	{
		printf("unsubscribe message channel fail.\n");
		releaseSource();
		return 0;
	}

	// ��ע�����֪ͨ
	// unregistration disconnection notice
	if (g_pEventSubscribe->unsubscribeConnectArgsEx(g_pEventSubscribe, onDeviceLinkNotify, (void*)pUser) < 0)
	{
		printf("unsubscribe connect event fail.\n");
		releaseSource();
		return 0;
	}

	// ��ע��ȡ���ص�����
	// unregister get frame callback function
	if (g_pStreamSource->detachGrabbingEx(g_pStreamSource, onGetFrame, (void*)pUser) < 0)
	{
		printf("detch Grabbing fail.\n");
	}

	// ֹͣ�������
	// stop camera grabbing
	if (g_pStreamSource->stopGrabbing(g_pStreamSource) < 0)
	{
		printf("stop Grabbing fail.\n");
		releaseSource();
		return 0;
	}

	// �Ͽ��豸
	// disconnect camera
	if (g_pCamera->disConnect(g_pCamera) < 0)
	{
		printf("disConnect cameral fail.\n");
		releaseSource();
		return 0;
	}

	releaseSource();

	isMainThreadExit = 1;

	return 1;
}