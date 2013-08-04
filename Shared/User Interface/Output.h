//
// The Epoch Language Project
// Shared Library Code
//
// User interface output routines. This structure is designed to decouple
// the bulk of the application from the actual type of user interface.
// This should make it trivial to port the code from one UI framework to
// another, between console and GUI formats, etc.
//

#pragma once


// Dependencies
#include <string>


namespace UI
{

	// String output functions
	void OutputMessage(const wchar_t* message);
	void OutputMessage(const std::wstring& message);


	// Colors supported by the output formatter
	enum OutputColor
	{
		OutputColor_Red,
		OutputColor_Green,
		OutputColor_Blue,
		OutputColor_LightRed,
		OutputColor_LightGreen,
		OutputColor_LightBlue,
		OutputColor_White
	};

	// Color control functions
	void SetOutputColor(OutputColor color);
}

// Template class implementations
#include "User Interface/OutputStream.inl"

