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
		out << UI::lightgreen << L"Parser trace: ";
		out << UI::resetcolor << traceinfo << std::endl;
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
		out << UI::lightgreen << L"Parser trace: ";
		out << UI::resetcolor << traceinfo << L" [" << identifier << L"]" << std::endl;
	}
#endif
}

//
// Display a trace of a newly created lexical scope descriptor.
//
// This is primarily useful for debugging issues with scope resolution
// and bindings in the parser. Note that not all code in the parser
// that creates scopes will call the trace; if you expect a trace output
// from this function but see none, ensure that the function is called
// within the code path in question.
//
void Parser::TraceScopeCreation(const VM::ScopeDescription* newscope, const VM::ScopeDescription* displacedscope)
{
#ifdef _DEBUG
	if(Config::TraceParserExecution)
	{
		UI::OutputStream out;
		out << UI::lightgreen << L"Scope created: ";
		out << UI::resetcolor << newscope;

		if(displacedscope)
			out << L" (displacing scope " << displacedscope << L")";

		out << std::endl;
	}
#endif
}