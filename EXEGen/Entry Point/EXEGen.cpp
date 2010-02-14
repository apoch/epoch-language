//
// The Epoch Language Project
// Win32 EXE Generator
//
// Entry point and main EXE generation logic
//

#include "pch.h"

#include "Linker/Linker.h"

#include "Project Files/Project.h"

#include "Resource Compiler/ResourceScript.h"
#include "Resource Compiler/ResourceDirectory.h"

#include "User Interface/Output.h"

#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Strings.h"

#include "DLL Access/Exceptions.h"
#include "DLL Access/FugueVMDLL.h"
#include "DLL Access/FugueASMDLL.h"

#include "Configuration/RuntimeOptions.h"

#include "Debugging/EXEDebugger.h"


// Prototypes
namespace
{
	void Usage();
	void ExecuteCommandLine(std::vector<std::wstring>& params, FugueVMDLLAccess& vmaccess, FugueASMDLLAccess& asmaccess);
}


//
// Entry point for the generator utility
//
int _tmain(int argc, _TCHAR* argv[])
{
	std::wcout << L"EXEGEN - Epoch development toolkit\n\n";

	if(argc <= 1)
	{
		Usage();
		return 0;
	}

	Config::LoadFromConfigFile();

	std::vector<std::wstring> commandlineparams(argc);
	for(int i = 0; i < argc; ++i)
		commandlineparams[i] = argv[i];

	try
	{
		FugueVMDLLAccess vmaccess;
		FugueASMDLLAccess asmaccess;

		ExecuteCommandLine(commandlineparams, vmaccess, asmaccess);
	}
	catch(DLLAccessException& e)
	{
		::MessageBox(0, e.GetMessage(), L"Epoch Subsystem", MB_ICONERROR);
	}
	catch(std::exception& e)
	{
		std::wcout << L"Error: " << e.what() << std::endl;
	}
	catch(...)
	{
		std::wcout << L"Unknown exception!" << std::endl;
	}

	return 0;
}


namespace
{

	//
	// Helper function for displaying the utility's command-line usage
	//
	void Usage()
	{
		UI::OutputStream output;

		output << L"\nWORKING WITH PROJECTS\n";
		output << L"   Exegen.exe /buildproject c:\\path\\to\\project.eprj\n";

		output << L"\nEXECUTING EPOCH PROGRAMS\n";
		output << L"   Exegen.exe /execsource c:\\path\\to\\source.epoch\n";
		output << L"   Exegen.exe /execbinary c:\\path\\to\\binary.epb\n";

		output << L"\nCOMPILING EPOCH PROGRAMS\n";
		output << L"   Exegen.exe /compile c:\\path\\to\\source.epoch c:\\path\\to\\output.easm\n";
		output << L"   Exegen.exe /makeexe c:\\path\\to\\source.epoch c:\\path\\to\\output.exe\n";
		
		output << L"\nEPOCH ASSEMBLY LANGUAGE UTILITIES\n";
		output << L"   Exegen.exe /assemble c:\\path\\to\\assembly.easm c:\\path\\to\\output.epb\n";
		output << L"   Exegen.exe /disassemble c:\\path\\to\\binary.epb c:\\path\\to\\output.easm\n";

		output << L"\nExegen can also be passed wildcards to process all matching files in a\n";
		output << L"directory. In this mode, the output parameter must be a path to a directory\n";
		output << L"rather than a single file.\n";

		output << std::endl;
	}


	//
	// Helper for determining if a parameter contains wildcards
	//
	bool HasWildcards(const std::wstring& str)
	{
		return str.find_first_of(L"*?") != std::wstring::npos;
	}


	//
	// Helper functions for analyzing the command line and validating the input
	//
	bool VerifyCommandLine(std::vector<std::wstring>& params, bool multipleparams)
	{
		size_t expectedparams = 4;

		if(!multipleparams)
			expectedparams = 3;

		// Barf if we don't have enough params
		if(params.size() < expectedparams)
		{
			Usage();
			return false;
		}

		if(multipleparams)
		{
			DWORD attributes = ::GetFileAttributes(params[3].c_str());

			// If input has wildcards, ensure the output is a directory and not a file
			if(HasWildcards(params[2]))
			{
				if(attributes == 0xFFFFFFFF)
				{
					std::ostringstream msg;
					msg << "Output path does not exist:\n" << narrow(params[3]);
					throw Exception(msg.str());
				}

				if(!(attributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					std::ostringstream msg;
					msg << "Input contains wildcards; output must be a valid directory:\n" << narrow(params[3]);
					throw Exception(msg.str());
				}

				if(*params[3].rbegin() != L'\\')
					params[3] += L'\\';
			}
			else
			{
				if((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					std::ostringstream msg;
					msg << "Specified output location is a directory:\n" << narrow(params[3]) << "\nPlease specify a filename for the output file.";
					throw Exception(msg.str());
				}
			}
		}

		return true;
	}

	//
	// Helper for batching calls to /execsource
	//
	void ExecuteSource(const std::wstring& inpath, FugueVMDLLAccess& vmaccess)
	{
		unsigned success = 0;
		unsigned count = 0;

		WIN32_FIND_DATA data;

		HANDLE resulthandle = ::FindFirstFile(inpath.c_str(), &data);
		if(resulthandle == INVALID_HANDLE_VALUE)
		{
			std::ostringstream msg;
			msg << "Input file not found:\n" << narrow(inpath);
			throw Exception(msg.str());
		}

		std::wstring inpathstripped;
		std::wstring::size_type pos = inpath.find_last_of(L'\\');
		if(pos == std::wstring::npos)
			inpathstripped = L".\\";
		else
			inpathstripped = inpath.substr(0, pos + 1);

		do
		{
			std::wstring filename = inpathstripped + data.cFileName;
			if(vmaccess.ExecuteSourceCode(narrow(filename).c_str()))
				++success;

			++count;
		} while(::FindNextFile(resulthandle, &data));

		::FindClose(resulthandle);

		UI::OutputStream output;
		output << success << L" of " << count << L" programs executed successfully.\n\n";
	}

	//
	// Helper for batching calls to /execbinary
	//
	void ExecuteBinary(const std::wstring& inpath, FugueVMDLLAccess& vmaccess)
	{
		unsigned success = 0;
		unsigned count = 0;

		WIN32_FIND_DATA data;

		HANDLE resulthandle = ::FindFirstFile(inpath.c_str(), &data);
		if(resulthandle == INVALID_HANDLE_VALUE)
		{
			std::ostringstream msg;
			msg << "Input file not found:\n" << narrow(inpath);
			throw Exception(msg.str());
		}

		std::wstring inpathstripped;
		std::wstring::size_type pos = inpath.find_last_of(L'\\');
		if(pos == std::wstring::npos)
			inpathstripped = L".\\";
		else
			inpathstripped = inpath.substr(0, pos + 1);

		do
		{
			std::wstring filename = inpathstripped + data.cFileName;
			if(vmaccess.ExecuteBinaryFile(narrow(filename).c_str()))
				++success;
			++count;
		} while(::FindNextFile(resulthandle, &data));

		::FindClose(resulthandle);

		UI::OutputStream output;
		output << success << L" of " << count << L" programs executed successfully.\n\n";
	}

	//
	// Helper for batching calls to /compile
	//
	bool Compile(const std::wstring& inpath, const std::wstring& outpath, FugueVMDLLAccess& vmaccess, bool usesconsole)
	{
		unsigned success = 0;
		unsigned count = 0;

		WIN32_FIND_DATA data;

		HANDLE resulthandle = ::FindFirstFile(inpath.c_str(), &data);
		if(resulthandle == INVALID_HANDLE_VALUE)
		{
			std::ostringstream msg;
			msg << "Input file not found:\n" << narrow(inpath);
			throw Exception(msg.str());
		}

		std::wstring inpathstripped;
		std::wstring::size_type pos = inpath.find_last_of(L'\\');
		if(pos == std::wstring::npos)
			inpathstripped = L".\\";
		else
			inpathstripped = inpath.substr(0, pos + 1);

		do
		{
			std::wstring filename = inpathstripped + data.cFileName;
			std::wstring outfilename = (HasWildcards(inpath) ? outpath + StripExtension(data.cFileName) + L".easm" : outpath);
			if(vmaccess.SerializeSourceCode(narrow(filename).c_str(), narrow(outfilename).c_str(), usesconsole))
				++success;
			++count;
		} while(::FindNextFile(resulthandle, &data));

		::FindClose(resulthandle);

		UI::OutputStream output;
		output << success << L" of " << count << L" files compiled successfully.\n\n";

		return (success == count);
	}

	//
	// Helper for batching assemble commands
	//
	bool Assemble(const std::wstring& inpath, const std::wstring& outpath, FugueASMDLLAccess& asmaccess)
	{
		unsigned success = 0;
		unsigned count = 0;

		WIN32_FIND_DATA data;

		HANDLE resulthandle = ::FindFirstFile(inpath.c_str(), &data);
		if(resulthandle == INVALID_HANDLE_VALUE)
		{
			std::ostringstream msg;
			msg << "Input file not found:\n" << narrow(inpath);
			throw Exception(msg.str());
		}

		std::wstring inpathstripped;
		std::wstring::size_type pos = inpath.find_last_of(L'\\');
		if(pos == std::wstring::npos)
			inpathstripped = L".\\";
		else
			inpathstripped = inpath.substr(0, pos + 1);

		do
		{
			std::wstring filename = inpathstripped + data.cFileName;
			std::wstring outfilename = (HasWildcards(inpath) ? outpath + StripExtension(data.cFileName) + L".epb" : outpath);
			if(asmaccess.Assemble(narrow(filename).c_str(), narrow(outfilename).c_str()))
				++success;
			++count;
		} while(::FindNextFile(resulthandle, &data));

		::FindClose(resulthandle);

		UI::OutputStream output;
		output << success << L" of " << count << L" files assembled successfully.\n\n";
		
		return (success == count);
	}

	//
	// Helper for batching disassemble commands
	//
	void Disassemble(const std::wstring& inpath, const std::wstring& outpath, FugueASMDLLAccess& asmaccess)
	{
		unsigned success = 0;
		unsigned count = 0;

		WIN32_FIND_DATA data;

		HANDLE resulthandle = ::FindFirstFile(inpath.c_str(), &data);
		if(resulthandle == INVALID_HANDLE_VALUE)
		{
			std::ostringstream msg;
			msg << "Input file not found:\n" << narrow(inpath);
			throw Exception(msg.str());
		}

		std::wstring inpathstripped;
		std::wstring::size_type pos = inpath.find_last_of(L'\\');
		if(pos == std::wstring::npos)
			inpathstripped = L".\\";
		else
			inpathstripped = inpath.substr(0, pos + 1);

		do
		{
			std::wstring filename = inpathstripped + data.cFileName;
			std::wstring outfilename = (HasWildcards(inpath) ? outpath + StripExtension(data.cFileName) + L".easm" : outpath);
			if(asmaccess.Disassemble(narrow(filename).c_str(), narrow(outfilename).c_str()))
				++success;
			++count;
		} while(::FindNextFile(resulthandle, &data));

		::FindClose(resulthandle);

		UI::OutputStream output;
		output << success << L" of " << count << L" files disassembled successfully.\n\n";
	}

	//
	// Helper function for taking a project wrapper object and generating the corresponding .EXE
	//
	void BuildProject(Projects::Project& project, FugueVMDLLAccess& vmaccess, FugueASMDLLAccess& asmaccess)
	{
		const std::list<std::wstring>& sourcefiles = project.GetSourceFileList();
		for(std::list<std::wstring>::const_iterator iter = sourcefiles.begin(); iter != sourcefiles.end(); ++iter)
		{
			if(!Compile(*iter, project.GetIntermediatesPath() + StripPath(StripExtension(*iter)) + L".easm", vmaccess, project.GetUsesConsoleFlag()))
				return;

			if(!Assemble(project.GetIntermediatesPath() + StripPath(StripExtension(*iter)) + L".easm", project.GetIntermediatesPath() + StripPath(StripExtension(*iter)) + L".epb", asmaccess))
				return;
		}
		
		Linker link(project);
		link.GenerateSections();
		link.CommitFile();
	}

	//
	// Helper function for doing a complete build of a project
	//
	void BuildProject(const std::wstring& projectfile, FugueVMDLLAccess& vmaccess, FugueASMDLLAccess& asmaccess)
	{
		BuildProject(Projects::Project(projectfile), vmaccess, asmaccess);
	}

	//
	// Helper function for building stand-alone EXE file from a single Epoch source file
	//
	void MakeExe(const std::wstring& codefile, const std::wstring& outputfile, bool useconsole, FugueVMDLLAccess& vmaccess, FugueASMDLLAccess& asmaccess)
	{
		BuildProject(Projects::Project(codefile, outputfile, useconsole), vmaccess, asmaccess);	
	}

	//
	// Helper function for debugging a compiled Epoch EXE
	//
	void DebugExe(const std::wstring& exefilename, FugueVMDLLAccess& vmaccess)
	{
		vmaccess.ExecuteBinaryBuffer(Debugger::EXEDebugger(exefilename).GetBinaryCodeBuffer());
	}

	//
	// Helper function for validating command line and performing the requested action
	//
	void ExecuteCommandLine(std::vector<std::wstring>& params, FugueVMDLLAccess& vmaccess, FugueASMDLLAccess& asmaccess)
	{
		if(params.size() <= 1)
		{
			Usage();
			return;
		}

		const std::wstring& commandswitch = params[1];

		if(commandswitch == L"/buildproject")
		{
			if(!VerifyCommandLine(params, false))
				return;

			BuildProject(params[2], vmaccess, asmaccess);
		}
		else if(commandswitch == L"/execsource")
		{
			if(!VerifyCommandLine(params, false))
				return;

			ExecuteSource(params[2], vmaccess);
		}
		else if(commandswitch == L"/execbinary")
		{
			if(!VerifyCommandLine(params, false))
				return;

			ExecuteBinary(params[2], vmaccess);
		}
		else if(commandswitch == L"/compile")
		{
			if(!VerifyCommandLine(params, true))
				return;

			bool consolemode = false;
			if(params.size() > 4)
			{
				if(params[4] == L"/console")
					consolemode = true;
				else if(params[4] == L"/noconsole")
					consolemode = false;
				else
				{
					throw Exception("Invalid option; please specify /console or /noconsole");
				}
			}
			else
				throw Exception("Please specify either the /console or /noconsole option.");

			Compile(params[2], params[3], vmaccess, consolemode);
		}
		else if(commandswitch == L"/makeexe")
		{
			if(!VerifyCommandLine(params, true))
				return;

			if(HasWildcards(params[2]))
				throw Exception("/makeexe switch only supports one input file at a time");

			bool consolemode = false;
			if(params.size() > 4)
			{
				if(params[4] == L"/console")
					consolemode = true;
				else if(params[4] == L"/noconsole")
					consolemode = false;
				else
				{
					throw Exception("Invalid option; please specify /console or /noconsole");
				}
			}
			else
				throw Exception("Please specify either the /console or /noconsole option.");

			MakeExe(params[2], params[3], consolemode, vmaccess, asmaccess);
		}
		else if(commandswitch == L"/assemble")
		{
			if(!VerifyCommandLine(params, true))
				return;

			Assemble(params[2], params[3], asmaccess);
		}
		else if(commandswitch == L"/disassemble")
		{
			if(!VerifyCommandLine(params, true))
				return;

			Disassemble(params[2], params[3], asmaccess);
		}
		else if(commandswitch == L"/debug")
		{
			if(!VerifyCommandLine(params, false))
				return;

			DebugExe(params[2], vmaccess);
		}
		else
		{
			Usage();
		}
	}

}

