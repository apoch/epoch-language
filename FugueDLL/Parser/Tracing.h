//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility code for tracing the execution of the parser
//

#pragma once


// Forward declarations
namespace VM
{
	class ScopeDescription;
}


namespace Parser
{
	void Trace(const wchar_t* traceinfo);
	void Trace(const wchar_t* traceinfo, const std::wstring& identifier);

	void TraceScopeCreation(const VM::ScopeDescription* newscope, const VM::ScopeDescription* displacedscope);
}

