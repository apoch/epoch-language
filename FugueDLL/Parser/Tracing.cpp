//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility code for tracing the execution of the parser
//

#include "pch.h"
#include "Parser/Tracing.h"
#include "User Interface/Output.h"
#include "Configuration/RuntimeOptions.h"


//
// Display a trace of what the parser is currently doing
//
void Parser::Trace(const wchar_t* traceinfo)
{
#ifdef _DEBUG
	if(Config::TraceParserExecution)
	{
		UI::OutputStream out;
		UI::SetOutputColor(UI::OutputColor_LightGreen);
		out << L"Parser trace: ";
		out.Flush();
		UI::SetOutputColor(UI::OutputColor_White);
		out << traceinfo << std::endl;
	}
#endif
}

//
// Display a trace of what the parser is currently doing, including the associated code fragment
//
void Parser::Trace(const wchar_t* traceinfo, const std::wstring& identifier)
{
#ifdef _DEBUG
	if(Config::TraceParserExecution)
	{
		UI::OutputStream out;
		UI::SetOutputColor(UI::OutputColor_LightGreen);
		out << L"Parser trace: ";
		out.Flush();
		UI::SetOutputColor(UI::OutputColor_White);
		out << traceinfo << L" [" << identifier << L"]" << std::endl;
	}
#endif
}
