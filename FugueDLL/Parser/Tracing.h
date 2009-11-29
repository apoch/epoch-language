//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility code for tracing the execution of the parser
//

#pragma once


namespace Parser
{
	void Trace(const wchar_t* traceinfo);
	void Trace(const wchar_t* traceinfo, const std::wstring& identifier);
}

