#pragma once

#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include <vector>


typedef const char* SemanticErrorDef;


class CompileErrors;


struct CompileErrorContextualizer
{
	virtual void UpdateContext(CompileErrors& errors) const = 0;
	virtual void UpdateFromContext(CompileErrors& errors, const AST::IdentifierT& context) const = 0;
};


class CompileErrors
{
// Construction
public:
	CompileErrors();

// Context establishment
public:
	void SetLocation(const std::wstring& file, size_t line, size_t column, const std::wstring& source);
	void GetContextFrom(CompileErrorContextualizer* contextualizer);
	
	template<typename T>
	void SetContext(const T& contextsource)
	{
		if(Contextualizer)
			Contextualizer->UpdateFromContext(*this, contextsource);
	}

// Error recording
public:
	void SemanticError(SemanticErrorDef error);

// Error examination
public:
	bool HasErrors() const;
	void DumpErrors() const;

// Internal state
private:
	struct Context
	{
		std::wstring File;
		size_t Line;
		size_t Column;
		std::wstring Source;
	};

	struct SemanticErrorT
	{
		Context ErrorContext;
		SemanticErrorDef Error;
	};

	Context CurrentContext;
	std::vector<SemanticErrorT> SemanticErrors;
	CompileErrorContextualizer* Contextualizer;
};

