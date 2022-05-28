#ifndef DAHUA_H
#define DAHUA_H

#include <math.h>
#include <QWidget>
#include <QMessageBox>
#include <QMutex>
#include "GenICam/System.h"
#include "Media/VideoRender.h"
#include "Media/ImageConvert.h"
#include "MessageQue.h"

class CFrameInfo : public Dahua::Memory::CBlock
{
public:
	CFrameInfo()
	{
		m_pImageBuf = NULL;
		m_nBufferSize = 0;
		m_nWidth = 0;
		m_nHeight = 0;
		m_PixelType = Dahua::GenICam::gvspPixelMono8;
		m_nPaddingX = 0;
		m_nPaddingY = 0;
		m_nTimeStamp = 0;
	}

	~CFrameInfo()
	{
	}

public:
	BYTE		*m_pImageBuf;
	int			m_nBufferSize;
	int			m_nWidth;
	int			m_nHeight;
	Dahua::GenICam::EPixelType	m_PixelType;
	int			m_nPaddingX;
	int			m_nPaddingY;
	uint64_t	m_nTimeStamp;
};

namespace Ui {
class CammerWidget;
}

class CammerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CammerWidget(QWidget *parent = 0);
    ~CammerWidget();

	//ö�ٴ�����ʽ
	enum ETrigType
	{
		trigContinous = 0,	//��������
		trigSoftware = 1,	//�������
		trigLine = 2,		//�ⲿ����		
	};

	//�ص�����
	void DahuaCallback(const Dahua::GenICam::CFrame & frame);
	//�����ع�
	bool SetExposeTime(double exposureTime);
	//��������
	bool SetAdjustPlus(double gainRaw);
	//�����
	bool CameraOpen(void);
	//�ر����
	bool CameraClose(void);
	//��ʼ�ɼ�
	bool CameraStart(void);
	//ֹͣ�ɼ�
	bool CameraStop(void);
	//�л��ɼ���ʽ��������ʽ �������ɼ����ⲿ���������������
	void CameraChangeTrig(ETrigType trigType = trigContinous);
	//ִ��һ���������ýӿ�Ϊ����ӣ�����ִ��������            
	void ExecuteSoftTrig(void);
	//�������������к�
	void CameraCheck(void);
	/// \brief ��ʾһ֡ͼ��
	/// \param [in] pRgbFrameBuf Ҫ��ʾ��ͼ������
	/// \param [in] nWidth ͼ��Ŀ�
	/// \param [in] nHeight ͼ��ĸ�
	/// \param [in] pixelFormat ͼ��ĸ�ʽ
	/// \retval true ��ʾʧ��
	/// \retval false  ��ʾ�ɹ� 
	bool ShowImage(uint8_t* pRgbFrameBuf, int nWidth, int nHeight, uint64_t pixelFormat);

private:
    Ui::CammerWidget *ui;
	
	Dahua::Infra::TVector<Dahua::GenICam::ICameraPtr> m_vCameraPtrList;	// ���ֵ�����б�
	Dahua::GenICam::ICameraPtr m_pCamera;								// ��ǰ�����Ĭ��Ϊ�б��еĵ�һ�����	
	Dahua::GenICam::IStreamSourcePtr m_pStreamSource;					// ������

	//CRender                             _render;          // ��ʾ����	
	Dahua::Infra::CThreadLite           _displayThread;   // ��ʾ�߳�      
	TMessageQue<CFrameInfo>				_displayFrameQueue;// ��ʾ����

	Dahua::Infra::CMutex				m_mxTime;
	double								m_dDisplayInterval;         // ��ʾ���
	uintmax_t							m_nTimestampFreq;           // ʱ���Ƶ��
	uintmax_t							m_nFirstFrameTime;          // ��һ֡��ʱ���
	uintmax_t							m_nLastFrameTime;           // ��һ֡��ʱ���

	VR_HANDLE          m_handler;           /* ��ͼ��� */
	VR_OPEN_PARAM_S    m_params;            /* ��ʾ���� */

	// �����߳�
	void GrabFrameThreadProc(Dahua::Infra::CThreadLite& lite);

	// ��ʾ�߳�
	void DisplayThreadProc(Dahua::Infra::CThreadLite& lite);

	// ������ʾƵ�ʣ�Ĭ��һ������ʾ30֡
	void setDisplayFPS(int nFPS);

	// �����֡�Ƿ���ʾ
	bool isTimeToDisplay(uintmax_t nCurTime);

	/* ��ʾ��غ��� */
	bool open(uint32_t width, uint32_t height);
	bool close();

	void closeEvent(QCloseEvent * event);

	VR_HWND		m_hWnd;
};

#endif // DAHUA_H
