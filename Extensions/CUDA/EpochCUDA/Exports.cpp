//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Functions exported by the interop DLL
//

#include "pch.h"

#include "Traverser/TraversalInterface.h"
#include "CUDA Wrapper/InvokeCode.h"
#include "Configuration/ConfigFile.h"

#include "Code Generation/CompiledCodeManager.h"
#include "Code Generation/EASMToCUDA.h"

#include "Language Extensions/HandleTypes.h"
#include "Language Extensions/FunctionPointerTypes.h"

#include "Utility/Strings.h"

#include "FugueVMAccess.h"


using namespace Extensions;
using namespace Traverser;


//
// Register the language extension library with the Fugue VM
//
// This procedure accomplishes two major tasks: setting up the function-pointer-based interface
// between the VM and the extension DLL, and informing the VM of all the keywords introduced by
// this extension. A code block prefixed by one of these keywords will be passed to the library
// that registered the keyword; the extension library is then able to traverse the code tree of
// the block, performing tasks such as cross-compilation to other languages, etc.
//
void __stdcall Register(const Extensions::ExtensionInterface* extensioninterface, ExtensionLibraryHandle token)
{
	try
	{
		FugueVMAccess::Interface = *extensioninterface;
		extensioninterface->Register(token, L"cuda");
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown during library registration");
	}
}



//
// Request a handle to the compiled language-extended code generated from
// the original Epoch code, as specified in the provided handle.
//
CodeBlockHandle __stdcall LoadSourceBlock(OriginalCodeHandle handle)
{
	try
	{
		return Compiler::GetCompiledBlock(handle);
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while trying to compile and load a code block");
	}

	return 0;
}

//
// Invoke execution of a compiled block of language-extended code
//
void __stdcall ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle)
{
	try
	{
		CUDACodeInvoker invoker(handle, activatedscopehandle);
		invoker.Execute();
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while trying to execute a CUDA code block");
	}
}


//
// Compiler callback: initialize the compiler and prepare for a compilation pass
//
// Note that clients do not need to call this function if they only perform a
// single compilation session; the new compilation will be set up automatically
// by the library. However, any subsequent compile sessions should be started
// via this function, in order to prevent corrupted code in the intermediary
// files used by the compiler.
//
void __stdcall StartNewProgramCompilation()
{
	try
	{
		Compiler::StartNewCompilation();
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while initializing the EpochASM-to-CUDA compiler");
	}
}

//
// Compiler callback: take all accumulated code and compile it
//
// During the parsing phase, the VM will hand off chunks of code to the extension
// library, as per the defined extension keywords. Once all of these chunks are
// provided to the extension, we can batch-compile them into a form that is more
// suitable for execution. For instance, this extension library compiles from the
// original EpochASM code into CUDA; during the commit process, we pass the CUDA
// code to NVCC in order to produce a .PTX file. The CUDA drivers then load the
// .PTX, assemble it to appropriate bytecode for the available CUDA device, and
// then execute the generated code on the CUDA device itself.
//
void __stdcall CommitCompilation()
{
	try
	{
		Compiler::CommitCompile();
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while compiling EpochASM code to CUDA");
	}
}



//
// Compiler callback: register the contents of a lexical scope
//
// This information is provided to allow seamless integration of variables
// from the Epoch code into the code produced by the extension library.
//
void __stdcall ScopeCallback(TraversalSessionHandle sessionhandle, size_t numcontents, const Traverser::ScopeContents* contents)
{
	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->RegisterScope(numcontents, contents);
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while processing a lexical scope");
	}
}

//
// Compiler callback: enter a node in the code tree
//
// Nodes generally correspond to lexical scopes; "entering" a node means that the
// compiler should update its state to reflect the newly entered scope.
//
void __stdcall NodeEntryCallback(TraversalSessionHandle sessionhandle)
{
	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->EnterNode();
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while performing entry processing on a node in the code tree");
	}
}

//
// Compiler callback: exit a node in the code tree
//
// Exiting a node generally corresponds to closing of a lexical scope.
//
void __stdcall NodeExitCallback(TraversalSessionHandle sessionhandle)
{
	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->ExitNode();
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while performing exit processing on a node in the code tree");
	}
}

//
// Compiler callback: register a leaf on the code tree
//
// Leaves generally correspond to individual EpochASM instructions, as generated by the
// Epoch compiler. The instruction's serialization token is provided; some instructions
// may also carry a "payload" such as a target variable name or a literal value.
//
void __stdcall LeafCallback(TraversalSessionHandle sessionhandle, const wchar_t* token, const Traverser::Payload* payload)
{
	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->RegisterLeaf(token, payload);
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while attempting to compile a leaf in the code tree");
	}
}

