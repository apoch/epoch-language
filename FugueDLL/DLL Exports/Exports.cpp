//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Functions exported by the VM DLL
//

#include "pch.h"

#include "Parser/Parse.h"
#include "Parser/Parser State Machine/ParserState.h"

#include "User Interface/Output.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/VMExceptions.h"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"

#include "Bytecode/Services.h"

#include "Configuration/RuntimeOptions.h"


namespace
{
	//
	// Dump a list of errors detected by the static validation system
	//
	void ReportValidationErrors(const std::list<Validator::ValidationError>& errors, const Parser::ParserState& state)
	{
		UI::OutputStream output;

		for(std::list<Validator::ValidationError>::const_iterator iter = errors.begin(); iter != errors.end(); ++iter)
		{
			output << L"Validation error: " << iter->ErrorText << L"\n";
			const FileLocationInfo& location = state.DebugInfo.GetInstructionLocation(iter->Operation);
					
			output << L"\nFile: " << location.FileName << L" Line: " << location.Line << L" Column: " << location.Column << L"\n";
			output << std::endl;
			state.DumpCodeLine(location.Line, location.Column, Config::TabWidth);

			output << L"\n" << std::endl;
		}
	}
}


//
// Execute a program from raw Epoch source code
//
bool __stdcall ExecuteSourceCode(const char* filename)
{
	UI::OutputStream output;

	try
	{
		Parser::ParserState state;
		std::vector<Byte> codememorybuffer;

		if(!Parser::ParseFile(filename, state, codememorybuffer))
		{
			output << UI::lightred << L"ERROR: " << UI::resetcolor;
			output << L"parsing failed" << std::endl;
			return false;
		}

		output << L"Performing static safety validations..." << std::endl;
		Validator::ValidationTraverser walker;
		state.GetParsedProgram()->Traverse(walker);
		if(!walker.IsValid())
		{
			ReportValidationErrors(walker.GetErrorList(), state);
			throw VM::ExecutionException("Program failed validation.");
		}

		output << L"Executing program..." << std::endl;
		Extensions::PrepareForExecution();
		state.GetParsedProgram()->Execute();
		return true;
	}
	catch(const Exception& e)
	{
		std::ostringstream text;
		text << e.GetErrorPrologue() << "\n\nError details: " << e.what();
		::MessageBoxA(0, text.str().c_str(), Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
	catch(const std::exception& e)
	{
		output << UI::lightred << L"ERROR: " << UI::resetcolor;
		output << e.what() << std::endl;
		::MessageBoxA(0, e.what(), Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
	catch(...)
	{
		output << UI::lightred << L"ERROR: UNKNOWN EXCEPTION" << UI::resetcolor << std::endl;
		::MessageBoxA(0, "Unknown error", Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
}

//
// Execute a binary compiled Epoch program
//
bool __stdcall ExecuteBinaryFile(const char* filename)
{
	try
	{
		return BinaryServices::ExecuteFile(filename);
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
}

//
// Execute a binary compiled Epoch program which is already in memory
//
bool __stdcall ExecuteBinaryBuffer(const Byte* buffer)
{
	try
	{
		return BinaryServices::ExecuteMemoryBuffer(buffer);
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
}


//
// Convert raw source code into assembly language format
//
bool __stdcall SerializeSourceCode(const char* filename, const char* outputfilename, bool usesconsole)
{
	UI::OutputStream output;
	output << L"Fugue - Epoch Compiler" << std::endl;

	try
	{
		Parser::ParserState state;
		std::vector<Byte> codememorybuffer;

		if(!Parser::ParseFile(filename, state, codememorybuffer))
		{
			output << UI::lightred << L"ERROR: " << UI::resetcolor;
			output << L"parsing failed" << std::endl;
			return false;
		}

		output << L"Performing static safety validations..." << std::endl;
		Validator::ValidationTraverser walker;
		state.GetParsedProgram()->Traverse(walker);
		if(!walker.IsValid())
		{
			ReportValidationErrors(walker.GetErrorList(), state);
			throw VM::ExecutionException("Program failed validation.");
		}

		output << L"Compiling program..." << std::endl;

		Extensions::PrepareForExecution();
		
		if(usesconsole)
			state.GetParsedProgram()->SetUsesConsole();

		Serialization::SerializationTraverser serializer(outputfilename);
		state.GetParsedProgram()->Traverse(serializer);
		output << L"Compiled successfully!\n" << std::endl;
		return true;
	}
	catch(const std::exception& e)
	{
		output << UI::lightred << L"ERROR: " << UI::resetcolor;
		output << e.what() << std::endl;
		::MessageBoxA(0, e.what(), Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
	catch(...)
	{
		output << UI::lightred << L"ERROR: UNKNOWN EXCEPTION" << UI::resetcolor << std::endl;
		::MessageBoxA(0, "Unknown error", Strings::WindowTitle, MB_ICONERROR);
		return false;
	}
}

