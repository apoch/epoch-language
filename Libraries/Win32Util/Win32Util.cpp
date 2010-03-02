//
// The Epoch Language Project
// Auxiliary Libraries
//
// Library for general commonly used Win32 functionality
//

#include "stdlib.h"
#include "windows.h"
#include <vector>

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Marshalling/LibraryImporting.h"


//
// Main entry/exit point for the DLL - initialization and cleanup should be done here
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    return TRUE;
}


//
// Callback that is invoked when a program starts running which loads this library.
// Our goal here is to register all of the functions in the library with the Epoch
// virtual machine, so that we can call the library from the program.
//
void __stdcall LinkToEpochVM(RegistrationTable registration, void* bindrecord)
{
	// Constants
#define REGCONSTANT_INTEGER(strname, value)	{ DWORD v = value; registration.RegisterConstant(strname, VM::EpochVariableType_Integer, &v, bindrecord); }

	REGCONSTANT_INTEGER(L"IDC_ARROW", 32512);

	REGCONSTANT_INTEGER(L"COLOR_WINDOWFRAME", COLOR_WINDOWFRAME);

	REGCONSTANT_INTEGER(L"CW_USEDEFAULT", CW_USEDEFAULT);

	REGCONSTANT_INTEGER(L"WS_OVERLAPPEDWINDOW", WS_OVERLAPPEDWINDOW);

	REGCONSTANT_INTEGER(L"SW_SHOW", SW_SHOW);

	REGCONSTANT_INTEGER(L"MB_ICONINFORMATION", MB_ICONINFORMATION);
	
	REGCONSTANT_INTEGER(L"WM_PAINT", WM_PAINT);
	REGCONSTANT_INTEGER(L"WM_DESTROY", WM_DESTROY);
	REGCONSTANT_INTEGER(L"WM_COMMAND", WM_COMMAND);

#undef REGCONSTANT_INTEGER

	// Function signatures
	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"message", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"wparam", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"lparam", VM::EpochVariableType_Integer));
		registration.RegisterSignature(L"wndprocsignature", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	// Structure IDs - we need to track these so we can set up nested structs later
	IDType StructureID_Point;
	IDType StructureID_Rect;
	IDType StructureID_WndClass;
	IDType StructureID_MessageInfo;
	IDType StructID_PaintInfo;

	// Structures
	{
		std::vector<ParamData> members;
		members.push_back(ParamData(L"x", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"y", VM::EpochVariableType_Integer));
		StructureID_Point = registration.RegisterStructure(L"Point", &members[0], members.size(), bindrecord);
	}

	{
		std::vector<ParamData> members;
		members.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"message", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"wparam", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"lparam", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"time", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"pt", VM::EpochVariableType_Structure, StructureID_Point));
		StructureID_MessageInfo = registration.RegisterStructure(L"MessageInfo", &members[0], members.size(), bindrecord);
	}

	{
		std::vector<ParamData> members;
		members.push_back(ParamData(L"left", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"top", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"right", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"bottom", VM::EpochVariableType_Integer));
		StructureID_Rect = registration.RegisterStructure(L"Rect", &members[0], members.size(), bindrecord);
	}

	{
		std::vector<ParamData> members;
		members.push_back(ParamData(L"size", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"style", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"wndproc", VM::EpochVariableType_Function, 0, false, L"wndprocsignature"));
		members.push_back(ParamData(L"classextra", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"wndextra", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"hinstance", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"hicon", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"hcursor", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"backgroundbrush", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"menuname", VM::EpochVariableType_String));
		members.push_back(ParamData(L"classname", VM::EpochVariableType_String));
		members.push_back(ParamData(L"hiconsmall", VM::EpochVariableType_Integer));
		StructureID_WndClass = registration.RegisterStructure(L"WindowClass", &members[0], members.size(), bindrecord);
	}

	{
		std::vector<ParamData> members;
		members.push_back(ParamData(L"hdc", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"erase", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"paintrect", VM::EpochVariableType_Structure, StructureID_Rect));
		members.push_back(ParamData(L"restore", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"incupdate", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved1", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved2", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved3", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved4", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved5", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved6", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved7", VM::EpochVariableType_Integer));
		members.push_back(ParamData(L"reserved8", VM::EpochVariableType_Integer));
		StructID_PaintInfo = registration.RegisterStructure(L"PaintInfo", &members[0], members.size(), bindrecord);
	}


	// Functions implemented in this library
	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"value", VM::EpochVariableType_Integer));
		registration.RegisterFunction(L"hiword", "GetHighWord", &params[0], params.size(), VM::EpochVariableType_Integer, VM::EpochVariableType_Error, bindrecord);
		registration.RegisterFunction(L"loword", "GetLowWord", &params[0], params.size(), VM::EpochVariableType_Integer, VM::EpochVariableType_Error, bindrecord);
	}

	// Functions implemented in other libraries
	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hinstance", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"name", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"LoadCursor", "LoadCursorW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"message", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"wparam", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"lparam", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"DefWindowProc", "DefWindowProcW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"exitcode", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"PostQuitMessage", "PostQuitMessage", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Null, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"ignored", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"GetModuleHandle", "GetModuleHandleW", L"kernel32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"wc", VM::EpochVariableType_Structure, StructureID_WndClass));
		registration.RegisterExternal(L"RegisterClassEx", "RegisterClassExW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"msg", VM::EpochVariableType_Structure, StructureID_MessageInfo));
		registration.RegisterExternal(L"TranslateMessage", "TranslateMessage", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Boolean, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"msg", VM::EpochVariableType_Structure, StructureID_MessageInfo));
		registration.RegisterExternal(L"DispatchMessage", "DispatchMessageW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"msg", VM::EpochVariableType_Structure, StructureID_MessageInfo, true));
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"filtermin", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"filtermax", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"GetMessage", "GetMessageW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"cmd", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"ShowWindow", "ShowWindow", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Boolean, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"exstyle", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"classname", VM::EpochVariableType_String));
		params.push_back(ParamData(L"caption", VM::EpochVariableType_String));
		params.push_back(ParamData(L"style", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"x", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"y", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"width", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"height", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"hwndparent", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"hmenu", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"hinstance", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"param", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"CreateWindowEx", "CreateWindowExW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"ps", VM::EpochVariableType_Structure, StructID_PaintInfo, true));
		registration.RegisterExternal(L"BeginPaint", "BeginPaint", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"ps", VM::EpochVariableType_Structure, StructID_PaintInfo, true));
		registration.RegisterExternal(L"EndPaint", "EndPaint", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hinstance", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"menuid", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"LoadMenu", "LoadMenuW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}

	{
		std::vector<ParamData> params;
		params.push_back(ParamData(L"hwnd", VM::EpochVariableType_Integer));
		params.push_back(ParamData(L"message", VM::EpochVariableType_String));
		params.push_back(ParamData(L"caption", VM::EpochVariableType_String));
		params.push_back(ParamData(L"typeflags", VM::EpochVariableType_Integer));
		registration.RegisterExternal(L"MessageBox", "MessageBoxW", L"user32.dll", &params[0], params.size(), VM::EpochVariableType_Integer, bindrecord);
	}
}



//-------------------------------------------------------------------------------
// Routines exported by the DLL
//-------------------------------------------------------------------------------

Integer32 __stdcall GetHighWord(Integer32 value)
{
	return HIWORD(value);
}

Integer32 __stdcall GetLowWord(Integer32 value)
{
	return LOWORD(value);
}
