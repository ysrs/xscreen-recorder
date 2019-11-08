#include "xscreen.h"
#include <iostream>
#include <QTime>

#include "XScreenRecord.h"


#define RECORDQSS "\
QPushButton:!hover\
{\
	background-image: url(:/XScreen/record_normal.png);\
}\
QPushButton:hover\
{\
	background-image: url(:/XScreen/record_hot.png);\
}\
QPushButton:pressed\
{\
	background-image: url(:/XScreen/record_pressed.png);\
	background-color: rgba(255, 255, 255, 0);\
}\
"

static bool isRecord = false;
static QTime rtime;

using namespace std;
XScreen::XScreen(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
}


void XScreen::timerEvent(QTimerEvent *e)
{
	if (isRecord)
	{
		int es = rtime.elapsed() / 1000;
		char buf[1024] = { 0 };
		sprintf_s(buf, "%03d:%02d", es / 60, es % 60);
		ui.timeLabel->setText(buf);
	}
}

void XScreen::Record()
{
	isRecord = !isRecord;
	if (isRecord)
	{
		startTimer(100);
		rtime.restart();
		ui.recordButton->setStyleSheet("background-image: url(:/XScreen/stop.png);\
			background-color: rgba(255, 255, 255, 0)");

		QDateTime t = QDateTime::currentDateTime();
		QString filename = t.toString("yyyyMMdd_hhmmss");
		filename = "xscreen_" + filename;
		filename += ".mp4";
		filename = ui.urlEdit->text() + "/" + filename;

		XScreenRecord::Get()->outWidth = ui.widthEdit->text().toInt();
		XScreenRecord::Get()->outHeight = ui.heightEdit->text().toInt();
		XScreenRecord::Get()->fps = ui.fpsEdit->text().toInt();
		if (XScreenRecord::Get()->Start(filename.toLocal8Bit()))
		{
			return;
		}
	}
	ui.recordButton->setStyleSheet(RECORDQSS);
	XScreenRecord::Get()->Stop();
}
