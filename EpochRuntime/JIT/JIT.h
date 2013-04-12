//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Just-in-time native code generation for Epoch
//

#pragma once


// Forward declarations
namespace Runtime
{
	class VirtualMachine;
}

namespace JIT
{

	//
	// Internal implementation forward declarations
	//
	namespace impl
	{
		struct LLVMData;
		class FunctionJITHelper;
	}

	//
	// Wrapper for all JIT native code generation functionality
	//
	class NativeCodeGenerator
	{
	// Construction and destruction
	public:
		NativeCodeGenerator(Runtime::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode);
		~NativeCodeGenerator();

	// Non-copyable
	private:
		NativeCodeGenerator(const NativeCodeGenerator&);
		NativeCodeGenerator& operator = (const NativeCodeGenerator&);

	// JIT code generation interface
	public:
		void AddFunction(size_t beginoffset, size_t endoffset, StringHandle alias);
		void AddGlobalEntity(size_t beginoffset, StringHandle alias);

		EPOCHRUNTIME void ExternalInvoke(JIT::JITContext& context, StringHandle alias);

		void Generate();
		void* GenerateCallbackWrapper(void* targetfunc);

	// Internal helpers
	private:
		llvm::Type* GetLLVMType(Metadata::EpochTypeID type, bool flatten = false);
		llvm::Type* GetLLVMSumType(Metadata::EpochTypeID type, bool flatten);

		llvm::Type* GetExternalType(Metadata::EpochTypeID type);

		llvm::FunctionType* GetLLVMFunctionType(StringHandle epochfunc);
		llvm::FunctionType* GetLLVMFunctionTypeFromSignature(StringHandle libraryfunc);
		llvm::FunctionType* GetLLVMFunctionTypeFromSignature(const FunctionSignature& sig);
		llvm::FunctionType* GetLLVMFunctionTypeFromEpochType(Metadata::EpochTypeID type);

		llvm::FunctionType* GetExternalFunctionType(StringHandle epochfunc);

		void AddNativeTypeMatcher(size_t beginoffset, size_t endoffset);
		void AddNativePatternMatcher(size_t beginoffset, size_t endoffset);
		
		llvm::Function* GetGeneratedFunction(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedTypeMatcher(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedPatternMatcher(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedGlobalInit(StringHandle entityname);

		llvm::Function* GetExternalFunction(StringHandle alias);

		llvm::Value* MarshalArgument(llvm::Value* arg, Metadata::EpochTypeID type);
		llvm::Value* MarshalReturn(llvm::Value* ret, Metadata::EpochTypeID type);
		void MarshalReferencePostCall(llvm::Value* ret, llvm::Value* fixuptarget, Metadata::EpochTypeID type);
		void MarshalCleanup(llvm::Value* ret, Metadata::EpochTypeID type);

		llvm::Value* GetCallbackWrapper(llvm::Value* funcptr);

	// Visible tracking
	public:
		Runtime::VirtualMachine& OwnerVM;

	// Internal tracking
	private:
		friend class impl::FunctionJITHelper;
	
		const Bytecode::Instruction* Bytecode;

		impl::LLVMData* Data;
		llvm::IRBuilder<> Builder;

		std::map<const char*, llvm::Function*> LibraryFunctionCache;
		std::map<StringHandle, llvm::Function*> ExternalFunctions;
		std::map<llvm::Value*, llvm::Function*> GeneratedCallbackWrappers;
	};


	void DestructLLVMModule();

}

