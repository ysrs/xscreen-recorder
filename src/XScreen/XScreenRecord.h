#pragma once
#include <QThread>
#include <QMutex>


class XScreenRecord : protected QThread
{
public:
	static XScreenRecord *Get()
	{
		static XScreenRecord ct;
		return &ct;
	}
	
	virtual ~XScreenRecord();

	void run() override;

	bool Start(const char *filename);
	void Stop();

public:
	// in
	int fps = 10;
	// out
	int outWidth = 1280;
	int outHeight = 720;

protected:
	XScreenRecord();

	QMutex mutext;
	bool isExit = false;
};

