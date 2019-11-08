#include "XCaptureThread.h"
#include <QTime>

#include <d3d9.h>
#include <iostream>


#pragma comment(lib, "d3d9.lib")

using namespace std;


// 截取全屏
void CaptureScreen(void *data)
{
	// 1. 创建directx3d对象
	static IDirect3D9 *d3d = nullptr;
	if (!d3d)
	{
		d3d = Direct3DCreate9(D3D_SDK_VERSION);
	}
	if (!d3d)
	{
		return;
	}

	// 2. 创建显卡设备对象
	static IDirect3DDevice9 *device = nullptr;
	if (!device)
	{
		D3DPRESENT_PARAMETERS pa;
		ZeroMemory(&pa, sizeof(pa));
		pa.Windowed = true;
		pa.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		pa.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pa.hDeviceWindow = GetDesktopWindow();
		d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &pa, &device
		);
	}

	if (!device)
	{
		return;
	}

	// 3. 创建离屏表面 
	static IDirect3DSurface9 *sur = nullptr;
	static int w = 0;
	static int h = 0;
	if (!sur)
	{
		w = GetSystemMetrics(SM_CXSCREEN);
		h = GetSystemMetrics(SM_CYSCREEN);
		device->CreateOffscreenPlainSurface(w, h,
			D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &sur, nullptr);
	}
	if (!sur)
	{
		return;
	}
	if (!data)
	{
		return;
	}

	// 4. 抓屏
	device->GetFrontBufferData(0, sur);

	// 5. 取出数据
	D3DLOCKED_RECT rect;
	ZeroMemory(&rect, sizeof(rect));
	if (sur->LockRect(&rect, nullptr, 0) != S_OK)
	{
		return;
	}
	memcpy(data, rect.pBits, w*h * 4);
	sur->UnlockRect();

	//cout << ".";
}

XCaptureThread::XCaptureThread()
{
}


XCaptureThread::~XCaptureThread()
{
}


void XCaptureThread::run()
{
	QTime t;
	while (!isExit)
	{
		t.restart();
		mutext.lock();
		int s = 1000 / fps;
		if (rgbs.size() < cacheSize)
		{
			char *data = new char[width*height * 4];
			CaptureScreen(data);
			rgbs.push_back(data);
		}
		mutext.unlock();
		s = s - t.restart();
		if (s<=0 || s>10000)
		{
			s = 10;
		}

		cout << "[" << s << "]";

		msleep(s);
	}
}

void XCaptureThread::Start()
{
	Stop();

	mutext.lock();
	isExit = false;
	CaptureScreen(nullptr);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	mutext.unlock();

	start();
}

void XCaptureThread::Stop()
{
	mutext.lock();
	isExit = true;
	while (!rgbs.empty())
	{
		delete[] rgbs.front();
		rgbs.pop_front();
	}
	mutext.unlock();
	wait();
}

char *XCaptureThread::GetRGB()
{
	mutext.lock();
	if (rgbs.empty())
	{
		mutext.unlock();
		return nullptr;
	}
	char *re = rgbs.front();
	rgbs.pop_front();
	mutext.unlock();

	//cout << "V";

	return re;
}


