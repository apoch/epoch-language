//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Functions exported by the interop DLL
//

#include "pch.h"

#include "Traverser/TraversalInterface.h"
#include "CUDA Wrapper/InvokeCode.h"
#include "CUDA Wrapper/Initialization.h"
#include "Configuration/ConfigFile.h"

#include "Code Generation/CompiledCodeManager.h"
#include "Code Generation/EASMToCUDA.h"

#include "CUDA Wrapper/Module.h"
#include "CUDA Wrapper/Naming.h"
#include "CUDA Wrapper/FunctionCall.h"

#include "Language Extensions/HandleTypes.h"
#include "Language Extensions/FunctionPointerTypes.h"

#include "Marshalling/LibraryImporting.h"

#include "Utility/Strings.h"

#include "FugueVMAccess.h"


using namespace Extensions;
using namespace Traverser;


RequestMarshalBufferPtr RequestMarshalBuffer = NULL;

bool CUDAAvailableForExecution = false;
bool CUDALibraryLoaded = false;


bool __stdcall Initialize()
{
	InitializeCUDA();
	return (CUDALibraryLoaded && CUDAAvailableForExecution);
}


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
		{
			std::vector<Extensions::ExtensionControlParamInfo> params;
			params.push_back(Extensions::ExtensionControlParamInfo());
			params[0].LocalVariableType = VM::EpochVariableType_Integer;
			params[0].CreatesLocalVariable = true;
			params.push_back(Extensions::ExtensionControlParamInfo());
			params[1].LocalVariableType = VM::EpochVariableType_Integer;
			params[1].CreatesLocalVariable = false;
			params.push_back(Extensions::ExtensionControlParamInfo());
			params[2].LocalVariableType = VM::EpochVariableType_Integer;
			params[2].CreatesLocalVariable = false;
			extensioninterface->RegisterControl(token, L"cudafor", params.size(), &params[0]);
		}
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
CodeBlockHandle __stdcall LoadSourceBlock(CompileSessionHandle sessionid, OriginalCodeHandle handle, const wchar_t* keyword)
{
	try
	{
		return Compiler::GetCompiledBlock(sessionid, handle, keyword);
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
		invoker.Execute(0, 0);
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

void __stdcall ExecuteControl(CodeBlockHandle handle, HandleType activatedscopehandle, size_t numparams, const Traverser::Payload* params)
{
	if(numparams != 2)
	{
		FugueVMAccess::Interface.Error(L"Incorrect number of parameters supplied to control block");
		return;
	}

	try
	{
		CUDACodeInvoker invoker(handle, activatedscopehandle);
		invoker.Execute(params[0].Int32Value, params[1].Int32Value);
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
// Invoking this function is mandatory to ensure that multiple compilation passes
// do not obliterate each others' intermediate files.
//
CompileSessionHandle __stdcall StartNewProgramCompilation(HandleType programhandle)
{
	try
	{
		return Compiler::StartNewCompilation(programhandle);
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while initializing the EpochASM-to-CUDA compiler");
	}

	return 0;
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
void __stdcall CommitCompilation(CompileSessionHandle sessionid)
{
	try
	{
		Compiler::CommitCompile(sessionid);
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
void __stdcall ScopeCallback(TraversalSessionHandle sessionhandle, bool toplevel, bool isghost, size_t numcontents, const Traverser::ScopeContents* contents)
{
	if(isghost)
		return;

	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->RegisterScope(toplevel, numcontents, contents);
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



void __stdcall FunctionCallback(Traverser::TraversalSessionHandle sessionhandle, const wchar_t* funcname)
{
	try
	{
		Compiler::CompilationSession* compile = reinterpret_cast<Compiler::CompilationSession*>(sessionhandle);
		compile->ExpectFunctionTraversal(funcname);
	}
	catch(std::exception& e)
	{
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		FugueVMAccess::Interface.Error(L"An unrecognized exception was thrown while attempting to compile a function");
	}
}



void __stdcall LinkToEpochVM(RegistrationTable registration, void* bindrecord)
{
	RequestMarshalBuffer = registration.RequestMarshalBuffer;

	if(registration.ShouldRegisterEverything)
	{
		registration.RegisterFunction(L"CUDAGenerateDataArray", "GenerateDataArray", NULL, 0, VM::EpochVariableType_Array, VM::EpochVariableType_Real, bindrecord);
		registration.RegisterFunction(L"CUDAGenerateEmptyArray", "GenerateEmptyArray", NULL, 0, VM::EpochVariableType_Array, VM::EpochVariableType_Real, bindrecord);
		registration.RegisterFunction(L"CUDAGetThreadIndex", "DummyExportFunction", NULL, 0, VM::EpochVariableType_Integer, VM::EpochVariableType_Error, bindrecord);
		registration.RegisterFunction(L"IsCUDAAvailable", "IsCUDAAvailable", NULL, 0, VM::EpochVariableType_Boolean, VM::EpochVariableType_Error, bindrecord);
	}
}


int __stdcall DummyExportFunction()
{
	FugueVMAccess::Interface.Error(L"Invoked dummy function; check that cross-compiler is generating correct code");
	return 0;
}


unsigned const DummyArraySize = 50000;


void* __stdcall GenerateDataArray()
{
	void* originalbuffer = RequestMarshalBuffer(sizeof(LibraryArrayReturnInfo) + (sizeof(Real) * DummyArraySize));
	void* buffer = originalbuffer;

	// Store array size and type hint
	reinterpret_cast<LibraryArrayReturnInfo*>(buffer)->TypeHint = VM::EpochVariableType_Real;
	reinterpret_cast<LibraryArrayReturnInfo*>(buffer)->ElementCount = DummyArraySize;
	buffer = reinterpret_cast<char*>(originalbuffer) + sizeof(LibraryArrayReturnInfo);

	// Fill array with simple data
	Real* values = reinterpret_cast<Real*>(buffer);
	for(size_t i = 0; i < DummyArraySize; ++i)
		values[i] = static_cast<Real>(i);

	return originalbuffer;
}


void* __stdcall GenerateEmptyArray()
{
	void* originalbuffer = RequestMarshalBuffer(sizeof(LibraryArrayReturnInfo) + (sizeof(Real) * DummyArraySize));
	void* buffer = originalbuffer;

	// Store array size and type hint
	reinterpret_cast<LibraryArrayReturnInfo*>(buffer)->TypeHint = VM::EpochVariableType_Real;
	reinterpret_cast<LibraryArrayReturnInfo*>(buffer)->ElementCount = DummyArraySize;
	buffer = reinterpret_cast<char*>(originalbuffer) + sizeof(LibraryArrayReturnInfo);

	// Fill array with blank data
	Real* values = reinterpret_cast<Real*>(buffer);
	for(size_t i = 0; i < DummyArraySize; ++i)
		values[i] = 0.0f;

	return originalbuffer;
}


bool __stdcall IsCUDAAvailable()
{
	return (CUDALibraryLoaded && CUDAAvailableForExecution);
}



void __stdcall FillSerializationBuffer(wchar_t** buffer, size_t* buffersize)
{
	std::vector<std::vector<Byte> > buffers;
	Compiler::CopyGeneratedCodeToMemoryBuffers(buffers);

	size_t totalsize = 0;
	for(std::vector<std::vector<Byte> >::const_iterator iter = buffers.begin(); iter != buffers.end(); ++iter)
		totalsize += iter->size();

	*buffer = new wchar_t[totalsize];
	*buffersize = totalsize;

	wchar_t* bufferptr = *buffer;

	for(std::vector<std::vector<Byte> >::const_iterator iter = buffers.begin(); iter != buffers.end(); ++iter)
	{
		for(size_t i = 0; i < iter->size(); ++i)
		{
			*bufferptr = widen(iter->at(i));
			++bufferptr;
		}
	}
}

void __stdcall FreeSerializationBuffer(wchar_t* buffer)
{
	delete [] buffer;
}

void __stdcall LoadDataBuffer(const char* buffer, size_t size)
{
	Compiler::LoadSerializedState(buffer, size);
}


void __stdcall PrepareBlock(CodeBlockHandle handle)
{
	std::string funcname = GenerateFunctionName(Compiler::GetOriginalCodeHandle(handle));
	std::string filename = narrow(Compiler::GetGeneratedPTXFileName(Compiler::GetAssociatedSession(handle)));
	Module::LoadCUDAModule(filename).CreateFunctionCall(funcname);
}

void __stdcall ClearEverything()
{
	Compiler::Clear();
}

