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

#include <vector>


// Prototypes
namespace
{
	void Usage();
}


//
// Entry point function for the program
//
int _tmain(int argc, _TCHAR* argv[])
{
	UI::OutputStream output;
	output << L"Epoch Language Project\nCommand line tools interface\n\n";
	output.Flush();

	std::vector<std::wstring> parameters(&argv[1], &argv[argc]);

	if(parameters.empty())
	{
		Usage();
		return 0;
	}

	bool didwork = false;
	for(size_t i = 0; i < parameters.size(); ++i)
	{
		if(parameters[i] == L"/execute")
		{
			didwork = true;

			if(++i >= parameters.size())
			{
				output << L"Error: expected a filename after /execute option\n\n";
				break;
			}

			try
			{
				std::wstring filename(parameters[i]);
				std::wstring source = Files::Load(filename);
				
				DLLAccess::CompilerAccess compileraccess;
				DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(filename, source);

				if(bytecodebufferhandle)
				{
					DLLAccess::VMAccess vmaccess;
					vmaccess.ExecuteByteCode(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));
				}
			}
			catch(std::exception& e)
			{
				output << L"Error: " << e.what() << std::endl;
			}
			catch(...)
			{
				output << L"Unknown error!" << std::endl;
			}
		}
		else if(parameters[i] == L"/compile")
		{
			didwork = true;

			std::wstring infilename, outfilename;
			if(++i >= parameters.size())
			{
				output << L"Error: expected two filenames after /compile option\n\n";
				break;
			}
			infilename = parameters[i];

			if(++i >= parameters.size())
			{
				output << L"Error: expected two filenames after /compile option\n\n";
				break;
			}
			outfilename = parameters[i];

			try
			{
				std::wstring source = Files::Load(infilename);
				
				DLLAccess::CompilerAccess compileraccess;
				DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(infilename, source);

				if(bytecodebufferhandle)
				{
					Serialization::Serializer serializer(compileraccess, bytecodebufferhandle);
					serializer.Write(outfilename);
				}
			}
			catch(std::exception& e)
			{
				output << L"Error: " << e.what() << std::endl;
			}
			catch(...)
			{
				output << L"Unknown error!" << std::endl;
			}
		}
	}

	if(!didwork)
		Usage();

	return 0;
}


namespace
{
	void Usage()
	{
		UI::OutputStream output;
		output << L"Available options:\n";
		output << L"  /execute filename.epoch           - Run the specified Epoch program\n";
		output << L"  /compile filename.epoch out.easm  - Compile the specified program to the file out.easm\n";
		output << std::endl;
	}
}