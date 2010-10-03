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

	try
	{
		// TODO - allow configuration via command line params
		std::wstring filename(L"d:\\epoch\\Programs\\Compiler and VM Tests\\errors.epoch");
		std::wstring source = Files::Load(filename);
		
		DLLAccess::CompilerAccess compileraccess;
		DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(filename, source);

		if(bytecodebufferhandle)
		{
			Serialization::Serializer serializer(compileraccess, bytecodebufferhandle);
			serializer.Write(L"d:\\foo.txt");

			DLLAccess::VMAccess vmaccess;
			vmaccess.ExecuteByteCode(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));
		}
	}
	catch(std::exception& e)
	{
		output << L"Error: " << e.what() << std::endl;
	}

	return 0;
}

