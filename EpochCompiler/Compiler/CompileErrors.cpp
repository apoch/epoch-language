#include "pch.h"

#include "Compiler/CompileErrors.h"

#include "User Interface/Output.h"


CompileErrors::CompileErrors()
	: Contextualizer(NULL)
{
	CurrentContext.File = L"<unknown>";
	CurrentContext.Line = 0;
	CurrentContext.Column = 0;
}

void CompileErrors::SetLocation(const std::wstring& file, size_t line, size_t column, const std::wstring& source)
{
	CurrentContext.File = file;
	CurrentContext.Line = line;
	CurrentContext.Column = column;
	CurrentContext.Source = source;
}


void CompileErrors::SemanticError(SemanticErrorDef error)
{
	if(Contextualizer)
		Contextualizer->UpdateContext(*this);

	SemanticErrorT se;
	se.ErrorContext = CurrentContext;
	se.Error = error;

	SemanticErrors.push_back(se);
}


bool CompileErrors::HasErrors() const
{
	return !SemanticErrors.empty();
}

void CompileErrors::DumpErrors() const
{
	UI::OutputStream output;
	for(std::vector<SemanticErrorT>::const_iterator iter = SemanticErrors.begin(); iter != SemanticErrors.end(); ++iter)
	{
		output << UI::lightred;
		output << iter->Error << L" in " << iter->ErrorContext.File;
		output << L" line " << iter->ErrorContext.Line;
		output << L" column " << iter->ErrorContext.Column << std::endl;
		output << UI::white;
		output << iter->ErrorContext.Source << std::endl;
		output << UI::lightgreen;
		for(size_t i = 1; i < iter->ErrorContext.Column; ++i)
			output << L"-";
		output << L"^" << std::endl;
		output << UI::white;
	}
}

void CompileErrors::GetContextFrom(CompileErrorContextualizer* contextualizer)
{
	Contextualizer = contextualizer;
}

