// EpochBGFXWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


using namespace bgfx;


extern "C" void bgfxInit(void* hwnd)
{
	bgfx::winSetHwnd(reinterpret_cast<HWND>(hwnd));
	init(RendererType::OpenGL, BGFX_PCI_ID_NONE, 0, nullptr, nullptr);

	
	
	// TODO - factor out into separate API calls

	bgfx::reset(560, 560, BGFX_RESET_VSYNC);

	// Enable debug text.
	bgfx::setDebug(BGFX_DEBUG_TEXT);

	// Set view 0 clear state.
	bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
	);
}


extern "C" void bgfxShutdown()
{
	bgfx::shutdown();
}

extern "C" void bgfxFrame()
{
			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(560), uint16_t(560) );

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 0, 0x4f, "Epoch BGFX integration demo");

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();
}

