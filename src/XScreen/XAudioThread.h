#pragma once
#include <QThread>
#include <QMutex>


class XAudioThread : protected QThread
{
public:
	static XAudioThread *Get()
	{
		static XAudioThread ct;
		return &ct;
	}
	virtual ~XAudioThread();

	void run() override;

	void Start();
	void Stop();
	// 返回空间由用户清理
	char *GetPCM();

public:
	int cacheSize = 10;

	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;
	int nbSample = 1024;

protected:
	XAudioThread();
	QMutex mutext;
	bool isExit = false;
	std::list<char *> pcms;
};

