/// \file
/// \~chinese
/// \brief ������������
/// \example main.cpp
/// \~english
/// \brief resume connect sample
/// \example main.cpp


// *******��DemoΪ����ʾSDK��ʹ�ã�û�и����޸����IP�Ĵ��룬
// ������֮ǰ����ʹ������ͻ����޸����IP��ַ������������������һ��*********    
//********This demo is a simple demonstration of the use of the SDK. There is no additional code to modify the camera IP.
//Before running, please use the camera client to modify the network segment of the camera IP address to be consistent with that of the host.*********

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "GenICam/System.h"
#include "GenICam/Camera.h"
#include "GenICam/GigE/GigECamera.h"
#include "GenICam/GigE/GigEInterface.h"
#include "Infra/PrintLog.h"
#include "Memory/SharedPtr.h"

using namespace Dahua::GenICam;
using namespace Dahua::Infra;

#ifdef _WIN32
#define SLEEP(a)	Sleep(a);
#else
#define SLEEP(a)	usleep(a * 1000);
#endif

ICameraPtr cameraSptr;//������� 		eng//camera object
IStreamSourcePtr streamPtr;//������ 	eng://stream object
IEventSubscribePtr eventPtr;//�¼�����	eng://event object

bool isMainThreadExit = false;
char pUser[10] = "Camera";

static void onDeviceLinkNotify(const SConnectArg& conArg, void* pUser);
static void onGetFrame(const CFrame &frame, const void* pUser);
static void onStreamEvent(const SStreamArg& streamArg, void* pUser);
static void onChannelCallback(const SMsgChannelArg& msgChannelArg, void* pUser);

// ���������������
// try reconnecting camera
static void TryToResumeConnect(void)
{
	if (NULL == cameraSptr.get())
	{
		printf("cameraSptr is null.\n");
		return;
	}

	// �Ͽ��������
	// disconnect cammera
	if (cameraSptr->disConnect())
	{
		printf("*******  disConnect success  *******\n");
	}
	else
	{
		printf("*******  disConnect failed  *******\n");
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
		if (!cameraSptr->connect())
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
	streamPtr.reset();
	streamPtr = CSystem::getInstance().createStreamSource(cameraSptr);
	if (NULL == streamPtr.get())
	{
		printf("create stream obj  fail.\n");
		return;
	}

	// ע��ص�
	// register callback functiohn
	if (streamPtr->attachGrabbingEx(onGetFrame, (void*)pUser))
	{
		printf("*******  attach stream callback success  *******\n");
	}
	else
	{
		printf("*******  attach stream callback fail  *******\n");
	}

	// ��ʼ����
	// start grabbing
	if (!streamPtr->startGrabbing())
	{
		printf("*******  start grab fail  *******\n");
		return;
	}

	printf("*******  start grab success  *******\n");

	return;
}

// ȡ���ص�����
// get stream callback function
static void onGetFrame(const CFrame &frame, const void* pUser)
{
	// �ж�֡����Ч��
	// Judge the validity of frame
	if (!frame.valid())
	{
		printf("frame is invalid!\n");
	}
	else
	{
		printf("pUser:%s  blockId = %lu.\n", (char*)pUser, frame.getBlockId());
	}

	return;
}

static void onDeviceLinkNotify(const SConnectArg& conArg, void* pUser)
{
	// ���ֻ��Զ���ʱ�Ĳ�����ֻҪ���յ�offLineʱ������ 
	// if it is only for the operation during disconnection, it can only be processed when receiving camera offline
	if (SConnectArg::offLine == conArg.m_event)
	{
		printf("*******  camera is offline  *******\n");

		if (NULL == streamPtr.get())
		{
			printf("streamPtr is null.\n");
			return;
		}

		// ע���ص�
		// unregister callback function
		if (streamPtr->detachGrabbingEx(onGetFrame, (void*)pUser))
		{
			printf("*******  detach stream callback success  *******\n");
		}
		else
		{
			printf("*******  detach stream callback fail  *******\n");
		}

		// ֹͣ����
		// stop grabbing
		if (streamPtr->stopGrabbing())
		{
			printf("*******  stop grab success  *******\n");
		}
		else
		{
			printf("*******  stop grab fail  *******\n");
		}
	}
	else if (SConnectArg::onLine == conArg.m_event)
	{
		printf("*******  camera is online  *******\n");

		TryToResumeConnect();
	}

	return;
}

// ������¼�֪ͨ�ص�����
// Camera stream event notification callback function
static void onStreamEvent(const SStreamArg& streamArg, void* pUser)
{
	if (streamEventStreamChannelError == streamArg.eStreamEventStatus)
	{
		printf("pUser:%s  EventStreamChannelError!\n", (char*)pUser);

		if (NULL == streamPtr.get())
		{
			printf("streamPtr is null.\n");
			return;
		}

		// ֹͣ����
		// stop grabbing
		if (!streamPtr->stopGrabbing())
		{
			printf("StreamChannelError: stopGrabbing fail.\n");
			return;
		}

		// �ж�����Ƿ�Ͽ�
		// judge whether camera offline
		if (NULL == cameraSptr.get())
		{
			printf("cameraSptr is null.\n");
			return;
		}

		if (!cameraSptr->isConnected())
		{
			printf("camera is disconnected.\n");
			return;
		}

		// ���¿�ʼ����
		// restart grabbing
		if (!streamPtr->startGrabbing())
		{
			printf("StreamChannelError: startGrabbing fail.\n");
			return;
		}
	}
	else if (streamEventLostPacket == streamArg.eStreamEventStatus)
	{
		printf("pUser:%s  EventLostPacket!\n", (char*)pUser);
	}
	else if (streamEventNormal == streamArg.eStreamEventStatus)
	{
		// do nothing
	}

	return;
}

// ��Ϣ֪ͨ�ص�����
// message notification callback function
static void onChannelCallback(const SMsgChannelArg& msgChannelArg, void* pUser)
{
	if (msgChannelArg.eventID == MSG_EVENT_ID_EXPOSURE_END)
	{
		printf("pUser:%s  Exposure End!\n", (char*)pUser);
	}
	else if (msgChannelArg.eventID == MSG_EVENT_ID_FRAME_TRIGGER)
	{
		printf("pUser:%s  Frame Trigger!\n", (char*)pUser);
	}
	else if (msgChannelArg.eventID == MSG_EVENT_ID_FRAME_START)
	{
		printf("pUser:%s  Frame Start!\n", (char*)pUser);
	}
	else
	{
		printf("eventID %d pUser:%s\n", msgChannelArg.eventID, (char*)pUser);
	}

	return;
}
// ��ʾ�豸��Ϣ
// dispaly device information
static uint32_t selectDevice(uint32_t cameraCnt)
{
	uint32_t cameraIndex = 0;
	// ��ʾ�û�ѡ�� 
	// imform user to select
	printf("Please intput the camera index:");
	while (1)
	{
		scanf("%d", &cameraIndex);
		setbuf(stdin, NULL);
		// �ж��û�ѡ��Ϸ��� 
		// judge the validity of user selection
		cameraIndex -= 1;// ��ʾ��Index�Ǵ�1��ʼ eg:cameraIndex starts at 1
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

static void displayDeviceInfo(TVector<ICameraPtr>& vCameraPtrList)
{
	ICameraPtr cameraSptr;
	// ��ӡTitle�� 
	// print title 
	printf("Idx Type Vendor     Model      S/N             DeviceUserID    IP Address    \n");
	printf("------------------------------------------------------------------------------\n");
	for (uint32_t cameraIndex = 0; cameraIndex < vCameraPtrList.size(); cameraIndex++)
	{
		cameraSptr = vCameraPtrList[cameraIndex];
		// Idx �豸�б��������� ����ʾ������3
		// index of  camera device list,display in 3 characters 
		printf("%-3d", cameraIndex + 1);

		// Type ������豸���ͣ�GigE��U3V��CL��PCIe��
		// Device type of camera 
		switch (cameraSptr->getType())
		{
		case ICamera::typeGige:
			printf(" GigE");
			break;
		case ICamera::typeU3v:
			printf(" U3V ");
			break;
		case ICamera::typeCL:
			printf(" CL  ");
			break;
		case ICamera::typePCIe:
			printf(" PCIe");
			break;
		default:
			printf("     ");
			break;
		}

		// VendorName ��������Ϣ ����ʾ������10 
		// VendorName manufacturer information  display in 10 characters 
		const char* vendorName = cameraSptr->getVendorName();
		char vendorNameCat[11];
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
		printf(" %-10.10s", cameraSptr->getModelName());
		
		// Serial Number ��������к� ����ʾ������15 
		// camera serial Number  display in 15 characters 
		printf(" %-15.15s", cameraSptr->getSerialNumber());

		// deviceUserID �Զ����û�ID ����ʾ������15 
		// camera user id, display in 15 characters 
		const char* deviceUserID = cameraSptr->getName();
		char deviceUserIDCat[16];
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
		IGigECameraPtr gigeCameraPtr = IGigECamera::getInstance(cameraSptr);
		if (NULL != gigeCameraPtr.get())
		{
			CString ip = gigeCameraPtr->getIpAddress();
			printf(" %s", ip.c_str());
		}
		printf("\n");

	}
}

int32_t main(void)
{
	CSystem &systemObj = CSystem::getInstance();
	TVector<ICameraPtr> vCameraPtrList;

	if (!systemObj.discovery(vCameraPtrList))
	{
		printf("discovery device fail.\n");
		return 0;
	}

	if (0 == vCameraPtrList.size())
	{
		printf("no camera device find.\n");
		return 0;
	}
	// ��ӡ���������Ϣ�����,����,��������Ϣ,�ͺ�,���к�,�û��Զ���ID,IP��ַ��
	// print camera info (index,Type,vendor name, model,serial number,DeviceUserID,IP Address)
	displayDeviceInfo(vCameraPtrList);
	uint32_t cameraIndex = selectDevice((uint32_t)vCameraPtrList.size());
	cameraSptr = vCameraPtrList[cameraIndex];

	// �����豸
	// connect device
	if (!cameraSptr->connect())
	{
		printf("connect cameral fail.\n");
		return 0;
	}

	// �����¼�����
	// recreate event object
	eventPtr = systemObj.createEventSubscribe(cameraSptr);
	if (NULL == eventPtr.get())
	{
		printf("create event obj  fail.\n");
		return 0;
	}

	// ע���������֪ͨ
	// register camera disconnection notice
	if (!eventPtr->subscribeConnectArgsEx(ConnectArgProcEx(&onDeviceLinkNotify), (void*)pUser))
	{
		printf("subscribe connect event fail.\n");
		return 0;
	}

	// ע��������¼�֪ͨ
	// register camera stream evet notice
	if (!eventPtr->subscribeStreamArgEx(StreamArgProcEx(&onStreamEvent), (void*)pUser))
	{
		printf("subscribe stream event fail.\n");
		return 0;
	}

	// ע����Ϣͨ���¼��ص�����
	// register message channel event callback function
	if (!eventPtr->subscribeMsgChannelEx(MsgChannelProcEx(&onChannelCallback), (void*)pUser))
	{
		printf("subscribe message channel fail.\n");
		return 0;
	}

	// ����������
	// create stream object
	streamPtr = systemObj.createStreamSource(cameraSptr);
	if (NULL == streamPtr.get())
	{
		printf("create stream obj  fail.\n");
		return 0;
	}

	// ע��ȡ���ص�����
	// Register get frame callback function
	if (!streamPtr->attachGrabbingEx(onGetFrame, (void*)pUser))
	{
		printf("attch Grabbing fail.\n");
		return 0;
	}

	// ��ʼ����
	// start grabbing
	if (!streamPtr->startGrabbing())
	{
		printf("StartGrabbing  fail.\n");
		return 0;
	}

	// ���̵߳ȴ�״̬
	// main thread wait status
	getchar();

	// ��ע��������¼�֪ͨ
	// unregistration camera streamEvent notice
	if (!eventPtr->unsubscribeStreamArgEx(StreamArgProcEx(&onStreamEvent), (void*)pUser))
	{
		printf("unsubscribe stream event fail.\n");
		return 0;
	}

	// ��ע����Ϣͨ���¼��ص�����
	// unregister message channel event callback function
	if (!eventPtr->unsubscribeMsgChannelEx(MsgChannelProcEx(&onChannelCallback), (void*)pUser))
	{
		printf("unsubscribe message channel fail.\n");
		return 0;
	}

	// ��ע�����֪ͨ
	// unregistration disconnection notice
	if (!eventPtr->unsubscribeConnectArgsEx(ConnectArgProcEx(&onDeviceLinkNotify), (void*)pUser))
	{
		printf("unsubscribe connect event fail.\n");
		return 0;
	}

	// ��ע��ȡ���ص�����
	// unregister get frame callback function
	if (!streamPtr->detachGrabbingEx(onGetFrame, (void*)pUser))
	{
		printf("detch Grabbing fail.\n");
	}

	// ֹͣ�������
	// stop camera grabbing
	if (!streamPtr->stopGrabbing())
	{
		printf("stop Grabbing fail.\n");
		return 0;
	}

	// �Ͽ��豸
	// disconnect camera
	if (!cameraSptr->disConnect())
	{
		printf("disConnect cameral fail.\n");
		return 0;
	}

	isMainThreadExit = true;
	
	return 1;
}
