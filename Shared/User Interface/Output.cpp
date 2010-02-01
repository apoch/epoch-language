//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of user interface input routines.
//

#include "pch.h"

#include "User Interface/Output.h"

#include "Utility/Threading/Threads.h"
#include "Utility/Threading/Synchronization.h"

#include <iostream>


using namespace UI;


Threads::CriticalSection ConsoleCriticalSection;


//
// Display text messages
//
void UI::OutputMessage(const wchar_t* message)
{
	Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
	WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE), message, static_cast<DWORD>(wcslen(message)), NULL, NULL);
}

void UI::OutputMessage(const std::wstring& message)
{
	Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
	WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE), message.c_str(), static_cast<DWORD>(message.length()), NULL, NULL);
}

//
// Set the text color for subsequent text output
//
void UI::SetOutputColor(OutputColor color)
{
	WORD bits;

	switch(color)
	{
	case OutputColor_Red:			bits = FOREGROUND_RED;							break;
	case OutputColor_Green:			bits = FOREGROUND_GREEN;						break;
	case OutputColor_Blue:			bits = FOREGROUND_BLUE;							break;
	case OutputColor_LightRed:		bits = FOREGROUND_RED | FOREGROUND_INTENSITY;	break;
	case OutputColor_LightGreen:	bits = FOREGROUND_GREEN | FOREGROUND_INTENSITY;	break;
	case OutputColor_LightBlue:		bits = FOREGROUND_BLUE | FOREGROUND_INTENSITY;	break;
	case OutputColor_White:
	default:
		bits = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	}

	{
		Threads::CriticalSection::Auto mutex(ConsoleCriticalSection);
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), bits);
	}
}

