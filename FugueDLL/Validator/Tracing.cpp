#include "pch.h"
#include "Validator/Tracing.h"
#include "User Interface/Output.h"
#include "Configuration/RuntimeOptions.h"


void Validator::TraceScopeEntry(const VM::ScopeDescription& scope)
{
#ifdef _DEBUG
	if(Config::TraceValidatorExecution)
	{
		UI::OutputStream out;
		out << UI::lightgreen << L"Validator trace: ";
		out << UI::resetcolor << L"Begin processing scope " << (&scope) << std::endl;
	}
#endif
}


void Validator::TraceScopeExit(const VM::ScopeDescription& scope)
{
#ifdef _DEBUG
	if(Config::TraceValidatorExecution)
	{
		UI::OutputStream out;
		out << UI::lightgreen << L"Validator trace: ";
		out << UI::resetcolor << L"Finished processing scope " << (&scope) << std::endl;
	}
#endif
}

