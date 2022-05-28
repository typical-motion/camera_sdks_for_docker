#include "Dahua.h"
#include "ui_cammerwidget.h"
#include "GenICam/System.h"
#include "GenICam/Camera.h"
#include "GenICam/StreamSource.h"

#define DEFAULT_SHOW_RATE (30)
#define TIMESTAMPFREQUENCY 125000000	//�������ʱ���Ƶ�ʹ̶�Ϊ125,000,000Hz

using namespace Dahua::GenICam;
using namespace Dahua::Infra;

CammerWidget::CammerWidget(QWidget *parent) :
    QWidget(parent)
    ,ui(new Ui::CammerWidget)
	,_displayThread(CThreadLite::ThreadProc(&CammerWidget::DisplayThreadProc, this), "Display")
	, m_dDisplayInterval(0)
	, m_nTimestampFreq(TIMESTAMPFREQUENCY)
	, m_nFirstFrameTime(0)
	, m_nLastFrameTime(0)
	, m_handler(NULL)
{
    ui->setupUi(this);

	m_hWnd = (VR_HWND)this->winId();
	// Ĭ����ʾ30֡
	setDisplayFPS(30);   

	// ������ʾ�߳�
	if (!_displayThread.isThreadOver())
	{
		_displayThread.destroyThread();
	}

	if (!_displayThread.createThread())
	{
        //MessageBoxA(NULL, "Create Display Thread Failed.", "", 0);
		//return FALSE;
	}
}

CammerWidget::~CammerWidget()
{
    if(!_displayThread.isThreadOver())
    {
	_displayThread.destroyThread();
    }	
    delete ui;
}

//�ص�����
void CammerWidget::DahuaCallback(const CFrame& frame)
{
	CFrameInfo frameInfo;
	frameInfo.m_nWidth = frame.getImageWidth();
	frameInfo.m_nHeight = frame.getImageHeight();
	frameInfo.m_nBufferSize = frame.getImageSize();
	frameInfo.m_nPaddingX = frame.getImagePadddingX();
	frameInfo.m_nPaddingY = frame.getImagePadddingY();
	frameInfo.m_PixelType = frame.getImagePixelFormat();
	frameInfo.m_pImageBuf = (BYTE *)malloc(sizeof(BYTE)* frameInfo.m_nBufferSize);

	/* �ڴ�����ʧ�ܣ�ֱ�ӷ��� */
	if (frameInfo.m_pImageBuf != NULL)
	{
		memcpy(frameInfo.m_pImageBuf, frame.getImage(), frame.getImageSize());

		if (_displayFrameQueue.size() > 16)
		{
			CFrameInfo frameOld;
			_displayFrameQueue.get(frameOld);
			free(frameOld.m_pImageBuf);
		}

		_displayFrameQueue.push_back(frameInfo);
	}
}

//�����ع�
bool CammerWidget::SetExposeTime(double exposureTime)
{
	if (NULL == m_pCamera)
	{
		printf("Set ExposureTime fail. No camera or camera is not connected.\n");
		return false;
	}

	CDoubleNode nodeExposureTime(m_pCamera, "ExposureTime");

	if (false == nodeExposureTime.isValid())
	{
		printf("get ExposureTime node fail.\n");
		return false;
	}

	if (false == nodeExposureTime.isAvailable())
	{
		printf("ExposureTime is not available.\n");
		return false;
	}

	if (false == nodeExposureTime.setValue(exposureTime))
	{
		printf("set ExposureTime value = %f fail.\n", exposureTime);
		return false;
	}

	return true;
}

//��������
bool CammerWidget::SetAdjustPlus(double gainRaw)
{
	if (NULL == m_pCamera)
	{
		printf("Set GainRaw fail. No camera or camera is not connected.\n");
		return false;
	}

	CDoubleNode nodeGainRaw(m_pCamera, "GainRaw");

	if (false == nodeGainRaw.isValid())
	{
		printf("get GainRaw node fail.\n");
		return false;
	}

	if (false == nodeGainRaw.isAvailable())
	{
		printf("GainRaw is not available.\n");
		return false;
	}

	if (false == nodeGainRaw.setValue(gainRaw))
	{
		printf("set GainRaw value = %f fail.\n", gainRaw);
		return false;
	}

	return true;
}

//�����
bool CammerWidget::CameraOpen(void)
{
	if (NULL == m_pCamera)
	{
		printf("connect camera fail. No camera.\n");
		return false;
	}

	if (true == m_pCamera->isConnected())
	{
		printf("camera is already connected.\n");
		return false;
	}

	if (false == m_pCamera->connect())
	{
		printf("connect camera fail.\n");
		return false;
	}

	return true;
}

//�ر����
bool CammerWidget::CameraClose(void)
{
	if (NULL == m_pCamera)
	{
		printf("disconnect camera fail. No camera.\n");
		return false;
	}

	if (false == m_pCamera->isConnected())
	{
		printf("camera is already disconnected.\n");
		return false;
	}

	if (false == m_pCamera->disConnect())
	{
		printf("disconnect camera fail.\n");
		return false;
	}

	return true;
}

//��ʼ�ɼ�
bool CammerWidget::CameraStart()
{
	if (m_pStreamSource != NULL)
	{
		return true;
	}

	if (NULL == m_pCamera)
	{
		printf("start camera fail. No camera.\n");
		return false;
	}

	m_pStreamSource = CSystem::getInstance().createStreamSource(m_pCamera);
	if (NULL == m_pStreamSource)
	{
        //MessageBoxA(NULL, "Get Stream Failed.", "", 0);
		return false;
	}

	bool isSuccess = m_pStreamSource->attachGrabbing(IStreamSource::Proc(&CammerWidget::DahuaCallback, this));
	if (!isSuccess)
	{
		return false;
	}

	if (!m_pStreamSource->startGrabbing())
	{
		m_pStreamSource.reset();
        //MessageBoxA(NULL, "Start Stream Grabbing Failed.", "", 0);
		return false;
	}

	return true;
}

//ֹͣ�ɼ�
bool CammerWidget::CameraStop()
{
	if (m_pStreamSource == NULL)
		return true;

	bool isSuccess = m_pStreamSource->detachGrabbing(IStreamSource::Proc(&CammerWidget::DahuaCallback, this));
	if (!isSuccess)
	{
		return false;
	}

	m_pStreamSource->stopGrabbing();
	m_pStreamSource.reset();

	/* �����ʾ���� */
	//CGuard guard(m_mutexQue);
	_displayFrameQueue.clear();

	return true;
}

//�л��ɼ���ʽ��������ʽ �������ɼ����ⲿ���������������
void CammerWidget::CameraChangeTrig(ETrigType trigType)
{
	if (NULL == m_pCamera)
	{
		printf("Change Trig fail. No camera or camera is not connected.\n");
		return;
	}

	if (trigContinous == trigType)
	{
		//���ô���ģʽ
		CEnumNode nodeTriggerMode(m_pCamera, "TriggerMode");
		if (false == nodeTriggerMode.isValid())
		{
			printf("get TriggerMode node fail.\n");
			return;
		}
		if (false == nodeTriggerMode.setValueBySymbol("Off"))
		{
			printf("set TriggerMode value = Off fail.\n");
			return;
		}
	}
	else if (trigSoftware == trigType)
	{
		//���ô���ԴΪ����
		CEnumNode nodeTriggerSource(m_pCamera, "TriggerSource");
		if (false == nodeTriggerSource.isValid())
		{
			printf("get TriggerSource node fail.\n");
			return;
		}
		if (false == nodeTriggerSource.setValueBySymbol("Software"))
		{
			printf("set TriggerSource value = Software fail.\n");
			return;
		}

		//���ô�����
		CEnumNode nodeTriggerSelector(m_pCamera, "TriggerSelector");
		if (false == nodeTriggerSelector.isValid())
		{
			printf("get TriggerSelector node fail.\n");
			return;
		}
		if (false == nodeTriggerSelector.setValueBySymbol("FrameStart"))
		{
			printf("set TriggerSelector value = FrameStart fail.\n");
			return;
		}

		//���ô���ģʽ
		CEnumNode nodeTriggerMode(m_pCamera, "TriggerMode");
		if (false == nodeTriggerMode.isValid())
		{
			printf("get TriggerMode node fail.\n");
			return;
		}
		if (false == nodeTriggerMode.setValueBySymbol("On"))
		{
			printf("set TriggerMode value = On fail.\n");
			return;
		}
	}
	else if (trigLine == trigType)
	{
		//���ô���ԴΪLine1����
		CEnumNode nodeTriggerSource(m_pCamera, "TriggerSource");
		if (false == nodeTriggerSource.isValid())
		{
			printf("get TriggerSource node fail.\n");
			return;
		}
		if (false == nodeTriggerSource.setValueBySymbol("Line1"))
		{
			printf("set TriggerSource value = Line1 fail.\n");
			return;
		}

		//���ô�����
		CEnumNode nodeTriggerSelector(m_pCamera, "TriggerSelector");
		if (false == nodeTriggerSelector.isValid())
		{
			printf("get TriggerSelector node fail.\n");
			return;
		}
		if (false == nodeTriggerSelector.setValueBySymbol("FrameStart"))
		{
			printf("set TriggerSelector value = FrameStart fail.\n");
			return;
		}

		//���ô���ģʽ
		CEnumNode nodeTriggerMode(m_pCamera, "TriggerMode");
		if (false == nodeTriggerMode.isValid())
		{
			printf("get TriggerMode node fail.\n");
			return;
		}
		if (false == nodeTriggerMode.setValueBySymbol("On"))
		{
			printf("set TriggerMode value = On fail.\n");
			return;
		}

		// �����ⴥ��Ϊ�����أ��½���ΪFallingEdge��
		CEnumNode nodeTriggerActivation(m_pCamera, "TriggerActivation");
		if (false == nodeTriggerActivation.isValid())
		{
			printf("get TriggerActivation node fail.\n");
			return;
		}
		if (false == nodeTriggerActivation.setValueBySymbol("RisingEdge"))
		{
			printf("set TriggerActivation value = RisingEdge fail.\n");
			return;
		}
	}
}

//ִ��һ������
void CammerWidget::ExecuteSoftTrig(void)
{
	if (NULL == m_pCamera)
	{
		printf("Set GainRaw fail. No camera or camera is not connected.\n");
		return;
	}

	CCmdNode nodeTriggerSoftware(m_pCamera, "TriggerSoftware");
	if (false == nodeTriggerSoftware.isValid())
	{
		printf("get TriggerSoftware node fail.\n");
		return;
	}
	if (false == nodeTriggerSoftware.execute())
	{
		printf("set TriggerSoftware fail.\n");
		return;
	}

	//�ȴ����һ������
	CThread::sleep(100);

	printf("ExecuteSoftTrig success.\n");
}

//�������������к�
void CammerWidget::CameraCheck(void)
{
	CSystem &systemObj = CSystem::getInstance();

	bool bRet = systemObj.discovery(m_vCameraPtrList);
	if (false == bRet)
	{
		printf("discovery fail.\n");
		return;
	}

	// ��ӡ���������Ϣ��key, ��������Ϣ, �ͺ�, ���кţ�
	for (int i = 0; i < m_vCameraPtrList.size(); i++)
	{
		ICameraPtr cameraSptr = m_vCameraPtrList[i];

		printf("Camera[%d] Info :\n", i);
		printf("    key           = [%s]\n", cameraSptr->getKey());
		printf("    vendor name   = [%s]\n", cameraSptr->getVendorName());
		printf("    model         = [%s]\n", cameraSptr->getModelName());
		printf("    serial number = [%s]\n", cameraSptr->getSerialNumber());
	}

	if (m_vCameraPtrList.size() < 1)
	{
		printf("no camera.\n");
	//	msgBoxWarn(tr("Device Disconnected."));
	}
	else
	{
		//Ĭ�������б��еĵ�һ�����Ϊ��ǰ�����������������򿪡��رա��޸��عⶼ�������������
		m_pCamera = m_vCameraPtrList[0];
	}
}

// ��ʾ
bool CammerWidget::ShowImage(uint8_t* pRgbFrameBuf, int nWidth, int nHeight, uint64_t pixelFormat)
{
	if (NULL == pRgbFrameBuf ||
		nWidth == 0 ||
		nHeight == 0)
	{
		printf("%s image is invalid.\n", __FUNCTION__);
		return false;
	}

	if (NULL == m_handler)
	{
		open((uint32_t)nWidth, (uint32_t)nHeight);
		if (NULL == m_handler)
		{
			return false;
		}
	}

	uint32_t width = (uint32_t)nWidth;
	uint32_t height = (uint32_t)nHeight;
	if (m_params.nWidth != width || m_params.nHeight != height)
	{
		close();
		open(width, height);
	}

	VR_FRAME_S renderParam = { 0 };
	renderParam.data[0] = pRgbFrameBuf;
	renderParam.stride[0] = nWidth;
	renderParam.nWidth = nWidth;
	renderParam.nHeight = nHeight;
	if (pixelFormat == Dahua::GenICam::gvspPixelMono8)
	{
		renderParam.format = VR_PIXEL_FMT_MONO8;
	}
	else
	{
		renderParam.format = VR_PIXEL_FMT_RGB24;
	}

	if (VR_SUCCESS == VR_RenderFrame(m_handler, &renderParam, NULL))
	{
		return true;
	}
	return false;
}

// ��ʾ�߳�
void CammerWidget::DisplayThreadProc(Dahua::Infra::CThreadLite& lite)
{
	while (_displayThread.looping())
	{
		CFrameInfo frameInfo;

		if (false == _displayFrameQueue.get(frameInfo, 500))
		{
			continue;
		}

		// �ж��Ƿ�Ҫ��ʾ��������ʾ���ޣ�30֡�����Ͳ���ת�롢��ʾ����
		if (!this->isTimeToDisplay(frameInfo.m_nTimeStamp))
		{
			/* �ͷ��ڴ� */
			free(frameInfo.m_pImageBuf);
			continue;
		}

		/* mono8��ʽ�ɲ���ת�룬ֱ����ʾ��������ʽ��Ҫ����ת�������ʾ */
		if (Dahua::GenICam::gvspPixelMono8 == frameInfo.m_PixelType)
		{
			/* ��ʾ */
			if (false == ShowImage(frameInfo.m_pImageBuf, frameInfo.m_nWidth, frameInfo.m_nHeight, frameInfo.m_PixelType))
			{
				printf("_render.display failed.\n");
			}
			/* �ͷ��ڴ� */
			free(frameInfo.m_pImageBuf);
		}
		else
		{
			/* ת�� */
			uint8_t *pRGBbuffer = NULL;
			int nRgbBufferSize = 0;
			nRgbBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 4;
			pRGBbuffer = (uint8_t *)malloc(nRgbBufferSize);
			if (pRGBbuffer == NULL)
			{
				/* �ͷ��ڴ� */
				free(frameInfo.m_pImageBuf);
				printf("RGBbuffer malloc failed.\n");
				continue;
			}

			IMGCNV_SOpenParam openParam;
			openParam.width = frameInfo.m_nWidth;
			openParam.height = frameInfo.m_nHeight;
			openParam.paddingX = frameInfo.m_nPaddingX;
			openParam.paddingY = frameInfo.m_nPaddingY;
			openParam.dataSize = frameInfo.m_nBufferSize;
			openParam.pixelForamt = frameInfo.m_PixelType;

			IMGCNV_EErr status = IMGCNV_ConvertToBGRA32_Ex(frameInfo.m_pImageBuf, &openParam, pRGBbuffer, &nRgbBufferSize, IMGCNV_DEMOSAIC_BILINEAR);
			if (IMGCNV_SUCCESS != status)
			{
				/* �ͷ��ڴ� */
				printf("IMGCNV_ConvertToBGRA32_Ex failed.\n");
				free(frameInfo.m_pImageBuf);
				free(pRGBbuffer);
				return;
			}

			/* �ͷ��ڴ� */
			free(frameInfo.m_pImageBuf);

			/* ��ʾ */
			if (false == ShowImage(pRGBbuffer, openParam.width, openParam.height, openParam.pixelForamt))
			{
				printf("_render.display failed.");
			}
			free(pRGBbuffer);
		}
	}
}

bool CammerWidget::isTimeToDisplay(uintmax_t nCurTime)
{
	CGuard guard(m_mxTime);

	// ����ʾ
	if (m_dDisplayInterval <= 0)
	{
		return false;
	}

	// ʱ���Ƶ�ʻ�ȡʧ��, Ĭ��ȫ��ʾ. ������������ϲ������
	if (m_nTimestampFreq <= 0)
	{
		return true;
	}

	// ��һ֡������ʾ
	if (m_nFirstFrameTime == 0 || m_nLastFrameTime == 0)
	{
		m_nFirstFrameTime = nCurTime;
		m_nLastFrameTime = nCurTime;

		//warnf("set m_nFirstFrameTime: %I64d\n", m_nFirstFrameTime);
		return true;
	}

	// ��ǰʱ�����֮ǰ�����С
	if (nCurTime < m_nFirstFrameTime)
	{
		m_nFirstFrameTime = nCurTime;
		m_nLastFrameTime = nCurTime;
		warnf("reset m_nFirstFrameTime: %I64d\n", m_nFirstFrameTime);
		return true;
	}

	// ��ǰ֡����һ֡�ļ��
	uintmax_t nDif = nCurTime - m_nLastFrameTime;
	double dTimstampInterval = 1.0 / m_nTimestampFreq;

	double dAcquisitionInterval = nDif * 1000 * dTimstampInterval;

	if (dAcquisitionInterval >= m_dDisplayInterval)
	{
		// �������һ֡��ʱ���
		m_nLastFrameTime = nCurTime;
		return true;
	}

	// ��ǰ֡����ڵ�һ֡��ʱ����
	uintmax_t nDif2 = nCurTime - m_nFirstFrameTime;
	double dCurrentFrameTime = nDif2 * 1000 * dTimstampInterval;

	if (dCurrentFrameTime > 1000 * 60 * 30) // ÿ��һ��ʱ�������ʼʱ��
	{
		m_nFirstFrameTime = nCurTime;
		warnf("reset m_nFirstFrameTime in period: %I64d\n", m_nFirstFrameTime);
	}
	// �������һ֡��ʱ���
	m_nLastFrameTime = nCurTime;

	dCurrentFrameTime = fmod(dCurrentFrameTime, m_dDisplayInterval);

	if ((dCurrentFrameTime * 2 < dAcquisitionInterval)
		|| ((m_dDisplayInterval - dCurrentFrameTime) * 2 <= dAcquisitionInterval))
	{
		return true;
	}

	return false;
}

void CammerWidget::setDisplayFPS(int nFPS)
{
	if (nFPS > 0)
	{
		CGuard guard(m_mxTime);
		m_dDisplayInterval = 1000.0 / nFPS;
	}
	else
	{
		CGuard guard(m_mxTime);
		m_dDisplayInterval = 0;
	}
}

bool CammerWidget::open(uint32_t width, uint32_t height)
{
	if (NULL != m_handler ||
		0 == width ||
		0 == height)
	{
		return false;
	}

	memset(&m_params, 0, sizeof(m_params));
#ifndef __linux__
	m_params.eVideoRenderMode = VR_MODE_GDI;
#else
	m_params.eVideoRenderMode = VR_MODE_OPENGLX;
#endif
	m_params.hWnd = m_hWnd/*(VR_HWND)this->winId()*/;
	m_params.nWidth = width;
	m_params.nHeight = height;

	VR_ERR_E ret = VR_Open(&m_params, &m_handler);
	if (ret == VR_NOT_SUPPORT)
	{
		printf("%s cant't display BGR on this computer", __FUNCTION__);
		return false;
	}

	printf("%s open failed. error code[%d]", __FUNCTION__, ret);
	return false;
}

bool CammerWidget::close()
{
	if (m_handler != NULL)
	{
		VR_Close(m_handler);
		m_handler = NULL;
	}
	return true;
}

void CammerWidget::closeEvent(QCloseEvent * event)
{
	if (NULL != m_pStreamSource && m_pStreamSource->isGrabbing())
		m_pStreamSource->stopGrabbing();
	if (NULL != m_pCamera && m_pCamera->isConnected())
		m_pCamera->disConnect();
}
