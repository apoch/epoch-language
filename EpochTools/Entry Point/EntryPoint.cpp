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
#include "DLL Access Wrappers/Runtime.h"

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
// Helper for loading self-hosting plugins
//
void LoadSelfHostingPlugin(const std::wstring& filename)
{
	std::wstring source = Files::Load(filename);
					
	DLLAccess::CompilerAccess compileraccess;
	compileraccess.FreePlugins();
	DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(filename, source);

	if(bytecodebufferhandle)
	{
		DLLAccess::RuntimeAccess runtimeaccess;
		runtimeaccess.ExecuteByteCodeAndPersist(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));
	}
}


//
// Entry point function for the program
//
int _tmain(int argc, _TCHAR* argv[])
{
	bool testmode = false;
	bool recurse = false;
	bool useplugins = false;

	unsigned testpasscount = 0;
	unsigned executioncount = 0;

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
		if(parameters[i] == L"/compile")
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
				output << UI::lightred << L"Error: " << e.what() << UI::white << std::endl;
			}
			catch(...)
			{
				output << UI::lightred << L"Unknown error!" << UI::white << std::endl;
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
						void* pdata = compileraccess.GetByteCode(bytecodebufferhandle);
						size_t size = compileraccess.GetByteCodeSize(bytecodebufferhandle);

						std::wstring intermediatefile = project.GetBinaryFileName(*iter);
						std::ofstream outfile(intermediatefile.c_str(), std::ios::binary);
						if(outfile)
							outfile.write(reinterpret_cast<const char*>(pdata), static_cast<std::streamsize>(size));
						else
						{
							output << UI::lightred << L"Error: failed to open output intermediate file\n" << intermediatefile << L"\n\n" << UI::white;
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
				output << UI::lightred << L"Error: " << e.what() << UI::white << std::endl;
			}
			catch(...)
			{
				output << UI::lightred << L"Unknown error!" << UI::white << std::endl;
			}
		}
		else if(parameters[i] == L"/pause")
		{
			output << L"\n\nPress enter to continue..." << std::endl;
			
			UI::Input input;
			input.BlockingRead();
		}
		else if(parameters[i] == L"/recurse")
		{
			recurse = true;
		}
		else if(parameters[i] == L"/runtests")
		{
			recurse = true;
			testmode = true;
		}
		else if(parameters[i] == L"/selfhost")
		{
			useplugins = true;
		}
		else
		{
			didwork = true;

			try
			{
				std::wstring filespec(parameters[i]);

				std::vector<std::wstring> files = Files::GetMatchingFiles(filespec, recurse);
				for(std::vector<std::wstring>::const_iterator iter = files.begin(); iter != files.end(); ++iter)
				{
					if(useplugins)
					{
						// TODO - make plugins configurable?
						LoadSelfHostingPlugin(L"D:\\Epoch\\Programs\\Compiler\\Compiler.epoch");
					}

					++executioncount;

					const std::wstring& filename = *iter;

					output << L"Executing: " << filename << L"\n" << std::endl;

					std::wstring source = Files::Load(filename);
					
					DLLAccess::CompilerAccess compileraccess;
					DLLAccess::CompiledByteCodeHandle bytecodebufferhandle = compileraccess.CompileSourceToByteCode(filename, source);

					if(bytecodebufferhandle)
					{
						if(useplugins)
						{
							Serialization::Serializer serializer(compileraccess, bytecodebufferhandle);
							serializer.Write(L"D:\\Epoch\\SelfHost.txt");		// TODO - hack
						}

						DLLAccess::RuntimeAccess runtimeaccess;
						runtimeaccess.FreePersisted();

						runtimeaccess.LinkTestHarness(&testpasscount);
						runtimeaccess.ExecuteByteCode(compileraccess.GetByteCode(bytecodebufferhandle), compileraccess.GetByteCodeSize(bytecodebufferhandle));

						runtimeaccess.FreeNativeCode();
					}

					output << L"\n\n" << std::endl;
				}
			}
			catch(const std::exception& e)
			{
				output << UI::lightred << L"Error: " << e.what() << UI::white << std::endl;
			}
			catch(...)
			{
				output << UI::lightred << L"Unknown error!" << UI::white << std::endl;
			}
		}
	}

	if(!didwork)
		Usage();
	else if(testmode)
	{
		unsigned failcount = (executioncount - testpasscount);
		output << UI::white << "\n\nTest pass statistics\n====================\n";
		output << "        Pass: " << (testpasscount == executioncount ? UI::lightgreen : UI::white) << testpasscount << " / " << executioncount << UI::white << "\n";
		output << "      Failed: " << (failcount ? UI::lightred : UI::white) << failcount << " / " << executioncount << UI::white << "\n\n" << std::endl;
	}

	return 0;
}


namespace
{
	void Usage()
	{
		UI::OutputStream output;
		output << L"Available options:\n";
		output << L"  /execute filename.epoch\n\tRun the specified Epoch program\n\n";
		output << L"  /compile filename.epoch out.easm\n\tCompile the specified program to the file out.easm\n\n";
		output << L"  /build project.eprj\n\tBuild the specified Epoch project\n\n";
		output << L"  /runtests folder\n\tRecursively execute all Epoch programs in folder as tests\n\n";
		output << L"  /pause\n\tPause for Enter key (can be repeated)\n\n";
		output << L"  /selfhost\n\tEnable use of self-hosting plugins\n\n";
		output << std::endl;
	}
}