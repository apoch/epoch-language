//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper object for generating the Epoch launcher stub
//

#pragma once


// Dependencies
#include "Linker/Linker.h"


//
// Wrapper class for generating Epoch launcher stub
//
class CodeGenerator : public LinkerSectionManager
{
// Section manager interface
public:
	virtual void Generate(Linker& linker);
	virtual void Emit(Linker& linker, LinkWriter& writer) const;

	virtual bool RepresentsPESection() const;
	DWORD GetSectionSize() const;

	DWORD GetEntryPointOffset() const;
};

