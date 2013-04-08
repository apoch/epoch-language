//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Just-in-time native code generation for Epoch
//

#pragma once


// Forward declarations
namespace VM
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
		NativeCodeGenerator(VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode);
		~NativeCodeGenerator();

	// Non-copyable
	private:
		NativeCodeGenerator(const NativeCodeGenerator&);
		NativeCodeGenerator& operator = (const NativeCodeGenerator&);

	// JIT code generation interface
	public:
		void AddFunction(size_t beginoffset, size_t endoffset, StringHandle alias);
		void AddGlobalEntity(size_t beginoffset);

		void Generate();

	// Internal helpers
	private:
		llvm::Type* GetLLVMType(Metadata::EpochTypeID type, bool flatten = false);
		llvm::Type* GetLLVMSumType(Metadata::EpochTypeID type, bool flatten);

		llvm::FunctionType* GetLLVMFunctionType(StringHandle epochfunc);
		llvm::FunctionType* GetLLVMFunctionTypeFromSignature(StringHandle libraryfunc);
		llvm::FunctionType* GetLLVMFunctionTypeFromSignature(const FunctionSignature& sig);
		llvm::FunctionType* GetLLVMFunctionTypeFromEpochType(Metadata::EpochTypeID type);

		void AddNativeTypeMatcher(size_t beginoffset, size_t endoffset);
		void AddNativePatternMatcher(size_t beginoffset, size_t endoffset);
		
		llvm::Function* GetGeneratedFunction(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedTypeMatcher(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedPatternMatcher(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedGlobalInit(StringHandle entityname);

	// Internal tracking
	private:
		friend class impl::FunctionJITHelper;
	
		VM::VirtualMachine& OwnerVM;
		const Bytecode::Instruction* Bytecode;

		impl::LLVMData* Data;
		llvm::IRBuilder<> Builder;

		std::map<const char*, llvm::Function*> LibraryFunctionCache;
	};

}

