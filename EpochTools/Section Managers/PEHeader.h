//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for emitting the Portable Executable header set
//

#pragma once

// Dependencies
#include "Linker/Linker.h"


//
// This class manages the PE headers for the output file.
//
class PEHeaderSection : public LinkerSectionManager
{
// Construction
public:
	PEHeaderSection()
		: ConsoleMode(false)
	{ }

// Section manager interface
public:
	virtual void Generate(Linker& linker);
	virtual void Emit(Linker& linker, LinkWriter& writer) const;

	virtual bool RepresentsPESection() const;

// Header management interface
public:
	DWORD GetHeaderSize() const;

// Options
public:
	void SetConsoleMode()			{ ConsoleMode = true; }

// Internal options
protected:
	bool ConsoleMode;
};

