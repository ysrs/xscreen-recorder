#include "xscreen.h"
#include <iostream>

#include <QtWidgets/QApplication>
#include <QDateTime>

#include "XCaptureThread.h"
#include "XAudioThread.h"
#include "XScreenRecord.h"


using namespace std;
int main(int argc, char *argv[])
{
	/////////////////////////////////////////////////////////
	///²âÊÔXScreenRecord
	//QDateTime t = QDateTime::currentDateTime();
	//QString filename = t.toString("yyyyMMdd_hhmmss");
	//filename = "xscreen_" + filename;
	//filename += ".mp4";
	//XScreenRecord::Get()->Start(filename.toLocal8Bit());
	//getchar();
	//XScreenRecord::Get()->Stop();
	/////////////////////////////////////////////////////////





	/////////////////////////////////////////////////////////
	///²âÊÔÒôÆµÂ¼ÖÆÀà
	//XAudioThread::Get()->Start();
	//for (;;)
	//{
	//	char *pcm = XAudioThread::Get()->GetPCM();
	//	if (pcm)
	//	{
	//		cout << "*";
	//	}
	//}
	//cout << endl;
	/////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////
	///²âÊÔÆÁÄ»Â¼ÖÆ
	//XCaptureThread::Get()->Start();
	//for (;;)
	//{
	//	char *data = XCaptureThread::Get()->GetRGB();
	//	if (data)
	//	{
	//		cout << "+";
	//	}
	//	else
	//	{
	//		//cout << "-";
	//	}
	//}
	//cout << endl;
	//XCaptureThread::Get()->Stop();
	/////////////////////////////////////////////////////////


	//getchar();

	QApplication a(argc, argv);
	XScreen w;
	w.show();
	return a.exec();
}
