//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Application entry point and dispatch to various bits of the toolkit
//

#include "pch.h"

#include "User Interface/Output.h"
#include "User Interface/Input.h"

#include "DLL Access Wrappers/Compiler.h"
#include "DLL Access Wrappers/VM.h"

#include "Serialization/Serializer.h"

#include "Utility/Files/Files.h"
#include "Utility/Files/FilesAndPaths.h"

#include "Linker/Linker.h"

#include "Project Files/Project.h"

#include <vector>
#include <fstream>


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

				output << L"Executing: " << filename << L"\n" << std::endl;

				std::wstring source = Files::Load(filename);
				
				DLLAccess::CompilerAccess compileraccess;
				DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(filename, source);

				if(bytecodebufferhandle)
				{
					DLLAccess::VMAccess vmaccess;
					vmaccess.ExecuteByteCode(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));
				}
			}
			catch(const std::exception& e)
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
				output << L"Compiling: " << infilename << L"\n";
				output << L"   Output: " << outfilename << L"\n" << std::endl;

				std::wstring source = Files::Load(infilename);
				
				DLLAccess::CompilerAccess compileraccess;
				DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(infilename, source);

				if(bytecodebufferhandle)
				{
					Serialization::Serializer serializer(compileraccess, bytecodebufferhandle);
					serializer.Write(outfilename);
				}
				else
					throw FatalException("Compilation failed!");
			}
			catch(const std::exception& e)
			{
				output << L"Error: " << e.what() << std::endl;
			}
			catch(...)
			{
				output << L"Unknown error!" << std::endl;
			}
		}
		else if(parameters[i] == L"/build")
		{
			didwork = true;

			if(++i >= parameters.size())
			{
				output << L"Error: expected a filename after /build option\n\n";
				break;
			}

			try
			{
				std::wstring filename(parameters[i]);
				output << L"Building: " << filename << L"\n" << std::endl;

				Projects::Project project(filename);

				bool compilationsuccess = true;

				const std::list<std::wstring>& sourcefiles = project.GetSourceFileList();
				for(std::list<std::wstring>::const_iterator iter = sourcefiles.begin(); iter != sourcefiles.end(); ++iter)
				{
					std::wstring source = Files::Load(*iter);
					
					DLLAccess::CompilerAccess compileraccess;
					DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(*iter, source);

					if(bytecodebufferhandle)
					{
						const void* pdata = compileraccess.GetByteCode(bytecodebufferhandle);
						size_t size = compileraccess.GetByteCodeSize(bytecodebufferhandle);

						std::wstring intermediatefile = project.GetBinaryFileName(*iter);
						std::ofstream outfile(intermediatefile.c_str(), std::ios::binary);
						if(outfile)
							outfile.write(reinterpret_cast<const char*>(pdata), static_cast<std::streamsize>(size));
						else
						{
							output << L"Error: failed to open output intermediate file\n" << intermediatefile << L"\n\n";
							compilationsuccess = false;
						}
					}
					else
						compilationsuccess = false;
				}

				if(compilationsuccess)
				{
					Linker link(project);
					link.GenerateSections();
					link.CommitFile();
				}
			}
			catch(const std::exception& e)
			{
				output << L"Error: " << e.what() << std::endl;
			}
			catch(...)
			{
				output << L"Unknown error!" << std::endl;
			}
		}
		else if(parameters[i] == L"/pause")
		{
			output << L"\n\nPress enter to continue..." << std::endl;
			
			UI::Input input;
			input.BlockingRead();
		}
		else if(parameters[i] == L"/vmdebug")
		{
			DLLAccess::VMAccess vmaccess;
			vmaccess.EnableVisualDebugger();
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
		output << L"  /build project.eprj				- Build the specified Epoch project\n";
		output << L"  /pause							- Pause for Enter key (can be repeated)\n";
		output << L"  /vmdebug							- Enable visual debug of the VM (SLOW)\n";
		output << std::endl;
	}
}