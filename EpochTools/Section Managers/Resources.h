//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for writing PE resource blocks
//

#pragma once


// Dependencies
#include "Linker/Linker.h"
#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceScript.h"


//
// Wrapper class for writing resources PE resource section
//
class Resources : public LinkerSectionManager
{
// Construction
public:
	explicit Resources(Linker& linker);

// Section manager interface
public:
	virtual void Generate(Linker& linker);
	virtual void Emit(Linker& linker, LinkWriter& writer);

	virtual bool RepresentsPESection() const;

	DWORD GetSize() const;

// Internal tracking
private:
	ResourceCompiler::ResourceDirectory Directory;
	ResourceCompiler::ResourceScript Script;
};

