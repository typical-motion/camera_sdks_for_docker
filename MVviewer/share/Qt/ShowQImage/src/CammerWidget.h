#ifndef CAMMERWIDGET_H
#define CAMMERWIDGET_H

#include <QWidget>
#include "GenICam/System.h"
#include "Media/ImageConvert.h"
#include "MessageQue.h"
#include <QElapsedTimer>
#include <QMutex>

// ״̬��ͳ����Ϣ 
// Status bar statistics
struct FrameStatInfo
{
	quint32     m_nFrameSize;       // ֡��С, ��λ: �ֽ� | frame size ,length :byte
	qint64      m_nPassTime;         // ���յ���֡ʱ������������ |  The number of nanoseconds passed when the frame was received
    FrameStatInfo(int nSize, qint64 nTime) : m_nFrameSize(nSize), m_nPassTime(nTime)
	{
	}
};

// ֡��Ϣ 
// frame imformation
class CFrameInfo : public Dahua::Memory::CBlock
{
public:
	CFrameInfo()
	{
		m_pImageBuf = NULL;
		m_nBufferSize = 0;
		m_nWidth = 0;
		m_nHeight = 0;
        m_ePixelType = Dahua::GenICam::gvspPixelMono8;
		m_nPaddingX = 0;
		m_nPaddingY = 0;
		m_nTimeStamp = 0;
	}

	~CFrameInfo()
	{
	}

public:
	uint8_t		*m_pImageBuf;
	int			m_nBufferSize;
	int			m_nWidth;
	int			m_nHeight;
	Dahua::GenICam::EPixelType	m_ePixelType;
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

	// ö�ٴ�����ʽ
	// Enumeration trigger mode
	enum ETrigType
	{
		trigContinous = 0,	// �������� | continue grabbing
		trigSoftware = 1,	// ������� | software trigger
		trigLine = 2,		// �ⲿ����	| external trigger
	};

    // �����
	// open cmaera
    bool CameraOpen(void);
    // �ر����
	// close camera
    bool CameraClose(void);
    // ��ʼ�ɼ�
	// start grabbing
    bool CameraStart(void);
    // ֹͣ�ɼ�
	// stop grabbing
    bool CameraStop(void);
	// ȡ���ص�����
	// get frame callback function
	void FrameCallback(const Dahua::GenICam::CFrame & frame);
    // �л��ɼ���ʽ��������ʽ �������ɼ����ⲿ���������������
	// Switch acquisition mode and triggering mode (continuous acquisition, external triggering and software triggering)
    bool CameraChangeTrig(ETrigType trigType = trigContinous);
    // ִ��һ������        
	// Execute a soft trigger
    bool ExecuteSoftTrig(void);
	// �����ع�
	// set exposuse time
	bool SetExposeTime(double dExposureTime);
	// ��������
	// set gain
	bool SetAdjustPlus(double dGainRaw);
	// ���õ�ǰ���
	// set current camera
	void SetCamera(const QString& strKey);

	// ״̬��ͳ����Ϣ
	// Status bar statistics
    void resetStatistic();
	QString getStatistic();

private:

	// ��ʾ�߳�
	// display thread 
	void DisplayThreadProc(Dahua::Infra::CThreadLite& lite);

	// ������ʾƵ�ʣ�Ĭ��һ������ʾ30֡
	// Set the display frequency to display 30 frames in one second by default
	void setDisplayFPS(int nFPS);

	// �����֡�Ƿ���ʾ
	// Calculate whether the frame is displayed
	bool isTimeToDisplay();
	
	// ���ڹر���Ӧ����
	// Window close response function
	void closeEvent(QCloseEvent * event);

	// ״̬��ͳ����Ϣ
    // Status bar statistics	
	void recvNewFrame(const CFrame& pBuf);
	void updateStatistic();

private slots:
    // ��ʾһ֡ͼ��
	// display a frame image 
    bool ShowImage(uint8_t* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);
signals:
    // ��ʾͼ����źţ���displayThread�з��͸��źţ������߳�����ʾ
	// Display the signal of the image, send the signal in displaythread, and display it in the main thread
	bool signalShowImage(uint8_t* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);

private:
    Ui::CammerWidget *ui;
	
	Dahua::GenICam::ICameraPtr m_pCamera;							// ��ǰ��� | current camera 
	Dahua::GenICam::IStreamSourcePtr m_pStreamSource;				// ������   |  stream object

	Dahua::Infra::CThreadLite           m_thdDisplayThread;			// ��ʾ�߳�      | diaplay thread 
	TMessageQue<CFrameInfo>				m_qDisplayFrameQueue;		// ��ʾ����      | diaplay queue

    // ������ʾ֡��   | Control display frame rate
	Dahua::Infra::CMutex				m_mxTime;
	int									m_nDisplayInterval;         // ��ʾ��� | diaplay time internal
	uintmax_t							m_nFirstFrameTime;          // ��һ֡��ʱ��� | Time stamp of the first frame
	uintmax_t							m_nLastFrameTime;           // ��һ֡��ʱ��� | Timestamp of previous frame
	QElapsedTimer						m_elapsedTimer;				// ������ʱ��������ʾ֡�� | Used to time and control the display frame rate

	// ״̬��ͳ����Ϣ 
	// Status bar statistics	
	typedef std::list<FrameStatInfo> FrameList;
	FrameList   m_listFrameStatInfo;
	QMutex      m_mxStatistic;
	quint64     m_nTotalFrameCount;		// �յ�����֡�� | recieve all frames
    QString     m_strStatistic;			// ͳ����Ϣ, ����������  | Statistics, excluding errors
	bool		m_bNeedUpdate;
};

#endif // CAMMERWIDGET_H
