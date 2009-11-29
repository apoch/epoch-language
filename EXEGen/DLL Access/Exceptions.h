//
// The Epoch Language Project
// Win32 EXE Generator
//
// Exception wrapper class for errors arising during DLL binding processes
//

#pragma once


class DLLAccessException
{
// Construction
public:
	DLLAccessException(const wchar_t* message)
		: Message(message)
	{ }

// Information access
public:
	const wchar_t* GetMessage() const
	{ return Message; }

// Internal storage
private:
	const wchar_t* Message;
};

