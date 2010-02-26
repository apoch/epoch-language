//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Routines for invoking the compiler and managing compiled blocks of code
//

#include "pch.h"

#include "Code Generation/CompiledCodeManager.h"
#include "Code Generation/EASMToCUDA.h"

#include "FugueVMAccess.h"
#include "Exports.h"

#include "Configuration/ConfigFile.h"

#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Files/SpecialPaths.h"
#include "Utility/Process.h"

#include <fstream>


using namespace Compiler;
using namespace Extensions;


// Keep track of compile session handles
CompileSessionHandle CompileHandleCounter = 0;


// Keep track of the code handles we have generated
CodeBlockHandle CodeHandleCounter = 0;

// Track correspondence between internal code handles and the block handles from the VM
std::map<CodeBlockHandle, OriginalCodeHandle> CodeHandleMap;

// Track the known variables for each generated code block
std::map<CodeBlockHandle, std::list<Traverser::ScopeContents> > RegisteredVariablesMap;


// We need to use the config file to locate the NVCC and CL compilers
extern Config::ConfigReader Configuration;


// Helpers/tracking for creating temporary intermediate files
struct CompileSessionData
{
	CompileSessionData(TemporaryFileWriter* writer, HandleType programhandle)
		: CompilationTempFile(writer),
		  BoundProgramHandle(programhandle)
	{ }

	~CompileSessionData()
	{
		delete CompilationTempFile;
	}

	void CloseTempFile()
	{
		delete CompilationTempFile;
		CompilationTempFile = NULL;
	}

	void AttachToTempFile(TemporaryFileWriter* writer)
	{
		CloseTempFile();
		CompilationTempFile = writer;
	}

	TemporaryFileWriter* CompilationTempFile;
	std::wstring GeneratedPTXFileName;
	std::set<std::wstring> InvokedFunctionList;
	HandleType BoundProgramHandle;
};

// Track active compile sessions
std::map<CompileSessionHandle, CompileSessionData*> CompileSessionMap;

// Track which session a code block belongs to
std::map<CodeBlockHandle, CompileSessionHandle> CodeHandleToSessionMap;


//
// Given a handle to an original Epoch code block, return a handle to the compiled code
//
// This function will perform the EpochASM-to-CUDA compilation process for the given
// code block, unless the block has already been compiled, in which case we simply
// return the handle of the previously compiled code block.
//
CodeBlockHandle Compiler::GetCompiledBlock(CompileSessionHandle sessionid, OriginalCodeHandle handle)
{
	// Don't bother compiling the same block twice
	for(std::map<CodeBlockHandle, OriginalCodeHandle>::const_iterator iter = CodeHandleMap.begin(); iter != CodeHandleMap.end(); ++iter)
	{
		if(iter->second == handle)
			return iter->first;
	}

	// Verify the compile session handle
	std::map<CompileSessionHandle, CompileSessionData*>::const_iterator sessioniter = CompileSessionMap.find(sessionid);
	if(sessioniter == CompileSessionMap.end())
		throw std::exception("Invalid compile session handle");

	std::list<Traverser::ScopeContents> registeredvariables;

	// Perform the code traversal and compilation pass
	{
		CompilationSession session(*(sessioniter->second->CompilationTempFile), registeredvariables, sessionid);
		session.FunctionPreamble(handle);

		Traverser::Interface traversal;
		traversal.NodeEntryCallback = NodeEntryCallback;
		traversal.NodeExitCallback = NodeExitCallback;
		traversal.NodeTraversalCallback = LeafCallback;
		traversal.ScopeTraversalCallback = ScopeCallback;
		traversal.FunctionTraversalCallback = FunctionCallback;

		FugueVMAccess::Interface.Traverse(handle, &traversal, reinterpret_cast<HandleType>(&session));

		session.MarshalOut();
	}

	++CodeHandleCounter;
	CodeHandleMap[CodeHandleCounter] = handle;
	RegisteredVariablesMap[CodeHandleCounter].swap(registeredvariables);

	CodeHandleToSessionMap[CodeHandleCounter] = sessionid;

	return CodeHandleCounter;
}

//
// Retrieve a list of valid variables for the given code block
//
const std::list<Traverser::ScopeContents>& Compiler::GetRegisteredVariables(Extensions::CodeBlockHandle handle)
{
	std::map<CodeBlockHandle, std::list<Traverser::ScopeContents> >::const_iterator iter = RegisteredVariablesMap.find(handle);
	if(iter == RegisteredVariablesMap.end())
		throw std::exception("Invalid code block handle");

	return iter->second;
}


void Compiler::TraverseInvokedFunctions(CompileSessionHandle session)
{
	std::map<CompileSessionHandle, CompileSessionData*>::iterator iter = CompileSessionMap.find(session);
	if(iter == CompileSessionMap.end())
		throw std::exception("Invalid session handle");

	Traverser::Interface traversal;
	traversal.NodeEntryCallback = NodeEntryCallback;
	traversal.NodeExitCallback = NodeExitCallback;
	traversal.NodeTraversalCallback = LeafCallback;
	traversal.ScopeTraversalCallback = ScopeCallback;
	traversal.FunctionTraversalCallback = FunctionCallback;

	TemporaryFileWriter headerfilewriter(std::ios_base::trunc, L"cu");
	iter->second->CompilationTempFile->OutputStream << L"#include \"" << headerfilewriter.GetFileName() << L"\"\n";

	for(std::set<std::wstring>::const_iterator funciter = iter->second->InvokedFunctionList.begin(); funciter != iter->second->InvokedFunctionList.end(); ++funciter)
	{
		CompilationSession compilesession(*(iter->second->CompilationTempFile), headerfilewriter, session);
		FugueVMAccess::Interface.TraverseFunction(funciter->c_str(), &traversal, reinterpret_cast<HandleType>(&compilesession), iter->second->BoundProgramHandle);
	}
}


//
// Retrieve the Epoch code handle of a compiled block, given the compiled block's handle
//
OriginalCodeHandle Compiler::GetOriginalCodeHandle(CodeBlockHandle handle)
{
	std::map<CodeBlockHandle, OriginalCodeHandle>::const_iterator iter = CodeHandleMap.find(handle);
	if(iter == CodeHandleMap.end())
		throw std::exception("Invalid code block handle");

	return iter->second;
}


//
// Prepare temporary intermediate files and tracking for a compile session
//
CompileSessionHandle Compiler::StartNewCompilation(HandleType programhandle)
{
	CompileSessionHandle ret = ++CompileHandleCounter;
	CompileSessionMap.insert(std::make_pair(ret, new CompileSessionData(new TemporaryFileWriter(std::ios_base::out | std::ios_base::trunc, L"cu"), programhandle)));
	return ret;
}

//
// Hand off the CUDA code to the NVCC compiler for generation of the final .PTX code
//
void Compiler::CommitCompile(CompileSessionHandle sessionid)
{
	std::map<CompileSessionHandle, CompileSessionData*>::iterator iter = CompileSessionMap.find(sessionid);

	if(iter == CompileSessionMap.end())
		throw std::exception("No intermediate .cu file has been generated; call Compiler::StartNewCompilation() before invoking Compiler::CommitCompile()");

	// Acquire the filename of the output .cu file and close the temp
	// file object's handle on the file so we can write to it
	std::wstring tempfilename = iter->second->CompilationTempFile->GetFileName();
	iter->second->CloseTempFile();

	// Generate the file that contains all functions invoked by the root CUDA code
	std::wstring functionsfilename;
	{
		std::auto_ptr<TemporaryFileWriter> destoutfile(new TemporaryFileWriter(std::ios_base::trunc, L"cu"));
		functionsfilename = destoutfile->GetFileName();
		iter->second->AttachToTempFile(destoutfile.release());

		TraverseInvokedFunctions(sessionid);

		iter->second->CloseTempFile();
	}

	// Generate the "master" file which #includes all of the generated CUDA code
	std::wstring masterfilename;
	{
		TemporaryFileWriter destoutfile(std::ios_base::trunc, L"cu");
		destoutfile.OutputStream << L"#include \"" << functionsfilename << "\"\n";
		destoutfile.OutputStream << L"#include \"" << tempfilename << "\"\n";
		masterfilename = destoutfile.GetFileName();
	}

	// Generate a filename for the intermediate .ptx file, which
	// will receive the output of NVCC's pass over the .cu file.
	{
		TemporaryFileWriter destoutfile(std::ios_base::trunc, L"ptx");
		iter->second->GeneratedPTXFileName = destoutfile.GetFileName();
	}


	// Prepare the command line for launching the NVCC compiler
	//
	// Note that we launch indirectly by invoking CMD.EXE; this is the
	// only way I could find to get the stdout and stderr outputs to
	// display correctly when using the Epoch command line tools.
	// Simply trying to redirect the handles via CreateProcess doesn't
	// work; something internally in NVCC changes and only minimal
	// error/status output is produced. It would be nice if we could
	// just invoke the compiler directly.
	std::wstring clpath = ShortenPathName(Configuration.ReadConfig<std::wstring>(L"cl"));
	std::wstring nvccpath = ShortenPathName(Configuration.ReadConfig<std::wstring>(L"nvcc"));
	std::wstring args = L"/c " + nvccpath + L" --compiler-bindir=" + clpath + L" --ptx " + masterfilename + L" --output-file=" + iter->second->GeneratedPTXFileName;
	std::wstring cmdpath = SpecialPaths::GetSystemPath() + L"\\cmd.exe";


	// Invoke the compiler
	unsigned exitcode;
	try
	{
		exitcode = LaunchProcessSynchronous(cmdpath, args);
	}
	catch(ProcessLaunchException&)
	{
		throw std::exception("Could not locate or launch CMD.EXE; unable to invoke NVCC compiler");
	}

	if(exitcode != 0)
		throw std::exception("NVCC compiler encountered errors");
}


//
// Retrieve the name of the .PTX file generated by the current compile session
//
const std::wstring& Compiler::GetGeneratedPTXFileName(CompileSessionHandle sessionid)
{
	std::map<CompileSessionHandle, CompileSessionData*>::const_iterator iter = CompileSessionMap.find(sessionid);
	if(iter == CompileSessionMap.end())
		throw std::exception("Invalid compile session handle");

	return iter->second->GeneratedPTXFileName;
}


//
// Close all open handles to temporary files; usually only needed if
// there is a catastrophic failure, such as an exception in the VM.
//
void Compiler::DestroyTempFiles()
{
	for(std::map<CompileSessionHandle, CompileSessionData*>::iterator iter = CompileSessionMap.begin(); iter != CompileSessionMap.end(); ++iter)
		delete iter->second;

	CompileSessionMap.clear();
}


CompileSessionHandle Compiler::GetAssociatedSession(CodeBlockHandle codehandle)
{
	std::map<CodeBlockHandle, CompileSessionHandle>::const_iterator iter = CodeHandleToSessionMap.find(codehandle);
	if(iter == CodeHandleToSessionMap.end())
		throw std::exception("Invalid compile session ID");

	return iter->second;
}


void Compiler::RecordInvokedFunction(CompileSessionHandle session, const std::wstring& functionname)
{
	std::map<CompileSessionHandle, CompileSessionData*>::const_iterator iter = CompileSessionMap.find(session);
	if(iter == CompileSessionMap.end())
		throw std::exception("Invalid compile session handle");

	iter->second->InvokedFunctionList.insert(functionname);	
}

