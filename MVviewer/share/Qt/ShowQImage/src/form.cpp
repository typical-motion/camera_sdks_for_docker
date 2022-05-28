#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent) :
    QWidget(parent)
    ,ui(new Ui::Form)
{
    ui->setupUi(this);

	connect(&m_staticTimer, &QTimer::timeout, this, &Form::onTimerStreamStatistic);

	initUi();
}

Form::~Form()
{
	delete ui;
}

void Form::initUi()
{
    ui->label_Statistic->setText("");// �������֮ǰ����ʾ״̬�� | Don't show status bar before connecting camera

	CSystem &systemObj = CSystem::getInstance();
	if (false == systemObj.discovery(m_vCameraPtrList))
	{
		printf("discovery fail.\n");
		return;
	}
	if (m_vCameraPtrList.size() < 1)
	{
		ui->comboBox->setEnabled(false);
		ui->pushButton_Open->setEnabled(false);
	}
	else
	{
		ui->comboBox->setEnabled(true);
		ui->pushButton_Open->setEnabled(true);

		for (int i = 0; i < m_vCameraPtrList.size(); i++)
		{
			ui->comboBox->addItem(m_vCameraPtrList[i]->getKey());
		}

		ui->widget->SetCamera(m_vCameraPtrList[0]->getKey());
	}

	ui->pushButton_Close->setEnabled(false);
	ui->pushButton_Start->setEnabled(false);
	ui->pushButton_Stop->setEnabled(false);
}

// ����Ҫ���ӵ����
// set camera which need to connect
void Form::on_comboBox_currentIndexChanged(int nIndex)
{
	ui->widget->SetCamera(m_vCameraPtrList[nIndex]->getKey());
}

// ����
// connect
void Form::on_pushButton_Open_clicked()
{
	if (!ui->widget->CameraOpen())
	{
		return;
	}

	ui->pushButton_Open->setEnabled(false);
	ui->pushButton_Close->setEnabled(true);
	ui->pushButton_Start->setEnabled(true);
	ui->pushButton_Stop->setEnabled(false);
	ui->comboBox->setEnabled(false);

	// �������֮����ʾͳ����Ϣ������ֵΪ0
	// Show statistics after connecting camera, all values are 0
	ui->widget->resetStatistic();
	QString strStatic = ui->widget->getStatistic();
    ui->label_Statistic->setText(strStatic);
}

// �Ͽ�
// disconnect
void Form::on_pushButton_Close_clicked()
{
	on_pushButton_Stop_clicked();
	ui->widget->CameraClose();

    ui->label_Statistic->setText("");// �Ͽ�����Ժ���ʾ״̬�� | Do not display the status bar after disconnecting the camera

	ui->pushButton_Open->setEnabled(true);
	ui->pushButton_Close->setEnabled(false);
	ui->pushButton_Start->setEnabled(false);
	ui->pushButton_Stop->setEnabled(false);
	ui->comboBox->setEnabled(true);
}

// ��ʼ
// start 
void Form::on_pushButton_Start_clicked()
{
	// ������������
	// set continue grabbing
	//ui->widget->CameraChangeTrig(CammerWidget::trigContinous);

	ui->widget->CameraStart();

	ui->pushButton_Start->setEnabled(false);
	ui->pushButton_Stop->setEnabled(true);

	ui->widget->resetStatistic();
	m_staticTimer.start(100);
}

// ֹͣ
// stop
void Form::on_pushButton_Stop_clicked()
{
	m_staticTimer.stop();

    ui->widget->CameraStop();

	ui->pushButton_Start->setEnabled(true);
	ui->pushButton_Stop->setEnabled(false);
}

void Form::onTimerStreamStatistic()
{
	QString strStatic = ui->widget->getStatistic();
    ui->label_Statistic->setText(strStatic);
}

void Form::closeEvent(QCloseEvent * event)
{
	on_pushButton_Stop_clicked();
	ui->widget->CameraClose();
}

