// EpochBGFXWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


using namespace bgfx;


extern "C" void bgfxInit(void* hwnd)
{
	bgfx::winSetHwnd(reinterpret_cast<HWND>(hwnd));
	init(RendererType::OpenGL, BGFX_PCI_ID_NONE, 0, nullptr, nullptr);
	::Sleep(4000);
}


extern "C" void bgfxShutdown()
{
	bgfx::shutdown();
}

