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

// Type shortcut
typedef void (__cdecl *EpochToJITWrapperFunc)(char** pstack, void* context);


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

		void Generate();

	// Internal helpers
	private:
		llvm::Type* GetLLVMType(Metadata::EpochTypeID type, bool flatten = false);
		llvm::Type* GetLLVMSumType(Metadata::EpochTypeID type, bool flatten);

		llvm::FunctionType* GetLLVMFunctionType(StringHandle epochfunc);

		void AddNativeTypeMatcher(size_t beginoffset, size_t endoffset);
		
		llvm::Function* GetGeneratedFunction(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedTypeMatcher(StringHandle funcname, size_t beginoffset);
		llvm::Function* GetGeneratedBridge(size_t beginoffset);

	// Internal tracking
	private:
		friend class impl::FunctionJITHelper;
	
		VM::VirtualMachine& OwnerVM;
		const Bytecode::Instruction* Bytecode;

		impl::LLVMData* Data;
		llvm::IRBuilder<> Builder;
	};

}

