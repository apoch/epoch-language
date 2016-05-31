// EpochBGFXWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


extern "C" void bgfxInit()
{
	bgfx_init(BGFX_RENDERER_TYPE_OPENGL, BGFX_PCI_ID_NONE, 0, nullptr, nullptr);
	::Sleep(4000);
}


extern "C" void bgfxShutdown()
{
	bgfx_shutdown();
}

