//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Service routines for working with raw bytecode
//

#include "pch.h"

#include "Bytecode/Services.h"
#include "Bytecode/Loading.h"

#include "Virtual Machine/Core Entities/Program.h"

#include "User Interface/Output.h"

#include "Utility/Files/Files.h"


//
// Load a binary file into memory and execute it
//
bool BinaryServices::ExecuteFile(const char* filename)
{
	std::vector<Byte> memory;
	Files::Load(filename, memory);
	return ExecuteMemoryBuffer(&memory[0]);
}

//
// Execute a binary program from a memory buffer
//
bool BinaryServices::ExecuteMemoryBuffer(const void* buffer)
{
	std::auto_ptr<VM::Program> program(new VM::Program);
	std::auto_ptr<FileLoader> loader(NULL);

	try
	{
		loader.reset(new FileLoader(buffer, *program.get()));
		loader->GetProgram()->Execute();
	}
	catch(const std::exception& e)
	{
		UI::OutputStream output;
		output << UI::lightred << L"ERROR: " << UI::resetcolor;
		output << e.what() << std::endl;
		::MessageBoxA(0, e.what(), Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
	catch(...)
	{
		UI::OutputStream output;
		output << UI::lightred << L"ERROR: UNKNOWN EXCEPTION" << UI::resetcolor << std::endl;
		::MessageBoxA(0, "Unknown error", Strings::WindowTitle, MB_ICONERROR);
		return false;
	}

	return true;
}

