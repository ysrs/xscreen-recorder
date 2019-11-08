#pragma once
#include <QThread>
#include <QMutex>


class XCaptureThread : protected QThread
{
public:
	static XCaptureThread *Get()
	{
		static XCaptureThread ct;
		return &ct;
	}
	virtual ~XCaptureThread();

	void run() override;

	void Start();
	void Stop();
	// 线程安全，返回的空间由用户释放
	char *GetRGB();

public:
	// out
	int width = 1280;
	int height = 720;

	// in
	int fps = 10;
	int cacheSize = 3;

protected:
	XCaptureThread();
	QMutex mutext;
	bool isExit = false;
	std::list<char *> rgbs;
};

