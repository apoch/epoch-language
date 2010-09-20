//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Application entry point and dispatch to various bits of the toolkit
//

#include "pch.h"

#include "User Interface/Output.h"

#include "DLL Access Wrappers/Compiler.h"
#include "DLL Access Wrappers/VM.h"

#include "Serialization/Serializer.h"

#include "Utility/Files/Files.h"


//
// Entry point function for the program
//
int _tmain(int argc, _TCHAR* argv[])
{
	UI::OutputStream output;
	output << L"Epoch Language Project\nCommand line tools interface\n\n";
	output.Flush();

	// TODO - exception safety/handling

	// TODO - allow configuration via command line params
	std::wstring source = Files::Load(L"d:\\epoch\\Programs\\Compiler and VM Tests\\retval.epoch");
	
	DLLAccess::CompilerAccess compileraccess;
	DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(source);

	if(bytecodebufferhandle)
	{
		Serialization::Serializer serializer(compileraccess, bytecodebufferhandle);
		serializer.Write(L"d:\\foo.txt");

		DLLAccess::VMAccess vmaccess;
		vmaccess.ExecuteByteCode(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));
	}

	return 0;
}

