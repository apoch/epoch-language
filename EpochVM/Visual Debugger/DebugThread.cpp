//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Launch point and main thread loop for the visual debugger thread
//

#include "pch.h"

#include "Visual Debugger/VisualDebugger.h"

#include "Virtual Machine/VirtualMachine.h"


namespace
{
	// Handy constants
	static const DWORD_PTR ID_SNAPSHOT = 100;
	static const DWORD_PTR ID_REPORTBOX = 101;


	//
	// Window procedure for the visual debugger window
	//
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch(msg)
		{
		// Create child widgets
		case WM_CREATE:
			{
				CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
#pragma warning(push)			// Disable the pointer size difference warning; the windows.h header is too stupid to account
#pragma warning(disable: 4311)	// for the fact that SetWindowLongPtr is 64 and 32 bit safe either way.
				::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG>(cs->lpCreateParams));
#pragma warning(pop)

				HWND snapshotbtnwnd = ::CreateWindowEx(0, L"Button", L"Snapshot", WS_CHILD | WS_VISIBLE, 10, 10, 150, 30, hwnd, reinterpret_cast<HMENU>(ID_SNAPSHOT), 0, 0);
				::SendMessage(snapshotbtnwnd, WM_SETFONT, reinterpret_cast<WPARAM>(::GetStockObject(DEFAULT_GUI_FONT)), TRUE);
				
				HWND reportboxwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, L"Edit", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 10, 50, 300, 300, hwnd, reinterpret_cast<HMENU>(ID_REPORTBOX), 0, 0);
				::SendMessage(reportboxwnd, EM_SETREADONLY, TRUE, 0);

				HFONT font = ::CreateFont(-::MulDiv(10, ::GetDeviceCaps(::GetDC(reportboxwnd), LOGPIXELSY), 72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Consolas");
				if(font)
					::SendMessage(reportboxwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
			}
			break;

		// Manage size restrictions
		case WM_GETMINMAXINFO:
			{
				RECT clientrect, fullrect;
				::GetClientRect(hwnd, &clientrect);
				::GetWindowRect(hwnd, &fullrect);

				int xthickness = (fullrect.right - fullrect.left) - (clientrect.right - clientrect.left);
				int ythickness = (fullrect.bottom - fullrect.top) - (clientrect.bottom - clientrect.top);

				MINMAXINFO* minmaxinfo = reinterpret_cast<MINMAXINFO*>(lparam);
				minmaxinfo->ptMinTrackSize.x = 170 + xthickness;
				minmaxinfo->ptMinTrackSize.y = 360 + ythickness;
				return 0;
			}
			break;

		// Resize internal widgets
		case WM_SIZE:
			{
				RECT clientrect;
				::GetClientRect(hwnd, &clientrect);

				clientrect.top += 50;
				clientrect.left += 10;
				clientrect.bottom -= 10;
				clientrect.right -= 10;

				::MoveWindow(::GetDlgItem(hwnd, ID_REPORTBOX), clientrect.left, clientrect.top, clientrect.right - clientrect.left, clientrect.bottom - clientrect.top, TRUE);
			}
			break;

		// Handle widget clicks
		case WM_COMMAND:
			{
				switch(LOWORD(wparam))
				{
				case ID_SNAPSHOT:
					{
#pragma warning(push)			// Disable the pointer size difference warning; the windows.h header is too stupid to account
#pragma warning(disable: 4312)	// for the fact that GetWindowLongPtr is 64 and 32 bit safe either way.
						VM::VirtualMachine* vm = reinterpret_cast<VM::VirtualMachine*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
#pragma warning(pop)
						::SetWindowText(::GetDlgItem(hwnd, ID_REPORTBOX), vm->DebugSnapshot().c_str());
					}
					break;
				}
			}
			break;
		}

		return ::DefWindowProc(hwnd, msg, wparam, lparam);
	}

	//
	// Thread loop procedure for running the visual debugger
	//
	DWORD WINAPI DebuggerThreadProc(LPVOID param)
	{
		HINSTANCE hinstance = ::GetModuleHandle(NULL);

		WNDCLASSEX wc;
		::ZeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.hCursor = ::LoadCursor(0, IDC_ARROW);
		wc.hInstance = hinstance;
		wc.lpfnWndProc = WndProc;
		wc.lpszClassName = L"EpochVMVisualDebuggerWnd";
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		
		::RegisterClassEx(&wc);

		HWND hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, L"EpochVMVisualDebuggerWnd", L"Epoch VM Visual Debugger", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hinstance, param);
		if(!hwnd)
		{
			::MessageBox(0, L"Failed to create visual debugger window!", L"Epoch VM Visual Debugger Error", MB_ICONSTOP);
			return 0;
		}

		::ShowWindow(hwnd, SW_SHOW);

		MSG msg;
		while(::GetMessage(&msg, 0, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		return 0;
	}
}


//
// Access to allow the outside world to start up a debugger thread
//
// WARNING: does not automatically prevent the creation of multiple debugger threads!
// If only one debugger per process is desirable, use some form of lock outside here.
//
void VisualDebugger::ForkDebuggerThread(VM::VirtualMachine* vm)
{
	if(!::CreateThread(NULL, 0, DebuggerThreadProc, vm, 0, NULL))
		throw Exception("Failed to fork visual debugger thread; debugging will not be activated");
}

