#include "XAudioThread.h"
#include <iostream>

#include <QAudioInput>
#include <QIODevice>


static QAudioInput *input = nullptr;
static QIODevice *io = nullptr;

using namespace std;
XAudioThread::XAudioThread()
{
}


XAudioThread::~XAudioThread()
{
}


void XAudioThread::run()
{
	int size = nbSample*channels*sampleByte;
	while (!isExit)
	{
		mutext.lock();
		if (!input)
		{
			msleep(1);
			continue;
		}

		if (pcms.size() > cacheSize)
		{
			mutext.unlock();
			msleep(5);
			continue;
		}
		char *data = new char[size];
		int readedSize = 0;
		while (readedSize < size)
		{
			int br = input->bytesReady();
			if (br < 1024)
			{
				msleep(1);
				continue;
			}
			int s = 1024;
			// 最后一次
			if (size - readedSize < s)
			{
				s = size - readedSize;
			}
			int len = io->read(data + readedSize, s);
			readedSize += len;
		}
		pcms.push_back(data);

		mutext.unlock();
	}
}

void XAudioThread::Start()
{
	Stop();

	mutext.lock();
	isExit = false;
	QAudioFormat fmt;
	fmt.setSampleRate(sampleRate);
	fmt.setChannelCount(channels);
	fmt.setSampleSize(sampleByte*8);
	fmt.setSampleType(QAudioFormat::UnSignedInt);
	fmt.setByteOrder(QAudioFormat::LittleEndian);
	fmt.setCodec("audio/pcm");
	input = new QAudioInput(fmt);
	io = input->start();

	mutext.unlock();

	start();
}

void XAudioThread::Stop()
{
	mutext.lock();
	isExit = true;
	while (!pcms.empty())
	{
		delete[] pcms.front();
		pcms.pop_front();
	}
	if (input)
	{
		io->close();
		input->stop();
		delete input;
		input = nullptr;
		io = nullptr;
	}
	mutext.unlock();
	wait();
}

char *XAudioThread::GetPCM()
{
	mutext.lock();
	if (pcms.empty())
	{
		mutext.unlock();
		return nullptr;
	}
	char *re = pcms.front();
	pcms.pop_front();
	mutext.unlock();

	//std::cout << "A";

	return re;
}
