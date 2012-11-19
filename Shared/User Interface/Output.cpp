//
// The Epoch Language Project
// Shared Library Code
//
// Implementation of user interface input routines.
//

#include "pch.h"

#include "User Interface/Output.h"

#include "Utility/Threading/Synchronization.h"

#include <iostream>


using namespace UI;


Threads::CriticalSection ConsoleCriticalSection;


//
// Display text messages
//
void UI::OutputMessage(const wchar_t* message)
{
#ifdef BOOST_WINDOWS
	Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
	WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE), message, static_cast<DWORD>(wcslen(message)), NULL, NULL);
#else
#warning Implement for *NIX
#endif
}

void UI::OutputMessage(const std::wstring& message)
{
#ifdef BOOST_WINDOWS
	Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
	WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE), message.c_str(), static_cast<DWORD>(message.length()), NULL, NULL);
#else
#warning Implement for *NIX
#endif
}

//
// Set the text color for subsequent text output
//
void UI::SetOutputColor(OutputColor color)
{
#ifdef BOOST_WINDOWS
    WORD bits;

	CONSOLE_SCREEN_BUFFER_INFO bufferinfo;
	::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &bufferinfo);
	bits = bufferinfo.wAttributes & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);

	switch(color)
	{
	case OutputColor_Red:			bits |= FOREGROUND_RED;								break;
	case OutputColor_Green:			bits |= FOREGROUND_GREEN;							break;
	case OutputColor_Blue:			bits |= FOREGROUND_BLUE;							break;
	case OutputColor_LightRed:		bits |= FOREGROUND_RED | FOREGROUND_INTENSITY;		break;
	case OutputColor_LightGreen:	bits |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;	break;
	case OutputColor_LightBlue:		bits |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;		break;
	case OutputColor_White:
	default:
		bits |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	}

	{
		Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), bits);
	}
#else
#warning Implement for *NIX
#endif
}

