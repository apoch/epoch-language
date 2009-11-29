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


// Keep track of the code handles we have generated
CodeBlockHandle CodeHandleCounter = 0;

// Track correspondence between internal code handles and the block handles from the VM
std::map<CodeBlockHandle, OriginalCodeHandle> CodeHandleMap;

// Track the known variables for each generated code block
std::map<CodeBlockHandle, std::list<Traverser::ScopeContents> > RegisteredVariablesMap;


// We need to use the config file to locate the NVCC and CL compilers
extern Config::ConfigReader Configuration;


// Helpers/tracking for creating temporary intermediate files
std::auto_ptr<TemporaryFileWriter> CompilationTempFile(NULL);
std::wstring GeneratedPTXFileName;


//
// Given a handle to an original Epoch code block, return a handle to the compiled code
//
// This function will perform the EpochASM-to-CUDA compilation process for the given
// code block, unless the block has already been compiled, in which case we simply
// return the handle of the previously compiled code block.
//
CodeBlockHandle Compiler::GetCompiledBlock(OriginalCodeHandle handle)
{
	// Don't bother compiling the same block twice
	for(std::map<CodeBlockHandle, OriginalCodeHandle>::const_iterator iter = CodeHandleMap.begin(); iter != CodeHandleMap.end(); ++iter)
	{
		if(iter->second == handle)
			return iter->first;
	}

	if(!CompilationTempFile.get())		// TODO - bug if we try to run multiple compile sessions concurrently; how should we handle the temp files in this case??
		StartNewCompilation();

	std::list<Traverser::ScopeContents> registeredvariables;

	// Perform the code traversal and compilation pass
	{
		CompilationSession session(*CompilationTempFile, registeredvariables, handle);
		session.FunctionPreamble(handle);

		Traverser::Interface traversal;
		traversal.NodeEntryCallback = NodeEntryCallback;
		traversal.NodeExitCallback = NodeExitCallback;
		traversal.NodeTraversalCallback = LeafCallback;
		traversal.ScopeTraversalCallback = ScopeCallback;

		FugueVMAccess::Interface.Traverse(handle, &traversal, reinterpret_cast<HandleType>(&session));

		session.MarshalOut();
	}

	++CodeHandleCounter;
	CodeHandleMap[CodeHandleCounter] = handle;
	RegisteredVariablesMap[CodeHandleCounter].swap(registeredvariables);
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
void Compiler::StartNewCompilation()
{
	delete CompilationTempFile.release();
	CompilationTempFile = std::auto_ptr<TemporaryFileWriter>(new TemporaryFileWriter(std::ios_base::out | std::ios_base::trunc, L"cu"));
	GeneratedPTXFileName.clear();
}

//
// Hand off the CUDA code to the NVCC compiler for generation of the final .PTX code
//
void Compiler::CommitCompile()
{
	if(!CompilationTempFile.get())
		throw std::exception("No intermediate .cu file has been generated; call Compiler::StartNewCompilation() before invoking Compiler::CommitCompile()");

	// Acquire the filename of the output .cu file and close the temp
	// file object's handle on the file so we can write to it
	std::wstring tempfilename = CompilationTempFile->GetFileName();
	delete CompilationTempFile.release();

	// Generate a filename for the intermediate .ptx file, which
	// will receive the output of NVCC's pass over the .cu file.
	{
		TemporaryFileWriter destoutfile(std::ios_base::trunc, L"ptx");
		GeneratedPTXFileName = destoutfile.GetFileName();
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
	std::wstring args = L"/c " + nvccpath + L" --compiler-bindir=" + clpath + L" --ptx " + tempfilename + L" --output-file=" + GeneratedPTXFileName;
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
const std::wstring& Compiler::GetGeneratedPTXFileName()
{
	return GeneratedPTXFileName;
}


//
// Close all open handles to temporary files; usually only needed if
// there is a catastrophic failure, such as an exception in the VM.
//
void Compiler::DestroyTempFiles()
{
	delete CompilationTempFile.release();
}

