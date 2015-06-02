//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// CODEGENCONTEXT.H
// Declaration for the code emission context wrapper
//


#pragma once


// Forward declarations
namespace llvm
{
	class Module;
	class Function;
	class FunctionType;
	class Value;

	class BasicBlock;
	class CallInst;
}


namespace CodeGen
{

	typedef size_t (__stdcall *ThunkCallbackT)(const wchar_t* thunkname);
	typedef size_t (__stdcall *StringCallbackT)(size_t stringhandle);


	class Context
	{
	public:		// Construction and destruction
		Context();
		~Context();

	public:		// Type management interface
		llvm::FunctionType* FunctionTypeCreate(llvm::Type* rettype);

		llvm::Type* TypeGetBoolean();
		llvm::Type* TypeGetInteger();
		llvm::Type* TypeGetInteger16();
		llvm::Type* TypeGetPointerTo(llvm::Type* raw);
		llvm::Type* TypeGetReal();
		llvm::Type* TypeGetString();
		llvm::Type* TypeGetVoid();

		llvm::Type* StructureTypeCreate();
		void StructureTypeQueueMember(llvm::Type* membertype);

	public:		// Function management interface
		llvm::Function* FunctionCreate(const char* name, llvm::FunctionType* fty);
		llvm::GlobalVariable* FunctionCreateThunk(const char* name, llvm::FunctionType* fty);
		void FunctionQueueParamType(llvm::Type* ty);

	public:		// Instruction management interface
		llvm::AllocaInst * CodeCreateAlloca(llvm::Type* vartype, const char* varname);
		llvm::BasicBlock* CodeCreateBasicBlock(llvm::Function* parent, bool setinsertpoint);
		void CodeCreateBranch(llvm::BasicBlock* target, bool setinsertpoint);
		llvm::CallInst* CodeCreateCall(llvm::Function* target);
		llvm::CallInst* CodeCreateCallThunk(llvm::GlobalVariable* target);
		void CodeCreateCondBranch(llvm::BasicBlock* truetarget, llvm::BasicBlock* falsetarget);
		void CodeCreateDereference();
		llvm::Value* CodeCreateGEP(unsigned index);
		void CodeCreateRead(llvm::AllocaInst* allocatarget);
		void CodeCreateReadParam(unsigned index);
		void CodeCreateReadStructure(llvm::Value* gep);
		void CodeCreateRet();
		void CodeCreateRetVoid();
		void CodeCreateWrite(llvm::AllocaInst* allocatarget);
		void CodeCreateWriteStructure(llvm::Value* gep);

		void CodeCreateOperatorBooleanNot();
		void CodeCreateOperatorIntegerEquals();

		void CodePushBoolean(bool value);
		void CodePushInteger(int value);
		void CodePushRawAlloca(llvm::AllocaInst* alloc);
		void CodePushRawGEP(llvm::Value* gep);
		void CodePushString(unsigned handle);

	public:		// Object code emission interface
		size_t EmitBinaryObject(char* buffer, size_t maxoutput);

	public:		// Miscellaneous configuration interface
		void SetEntryFunction(llvm::Function* func);

		llvm::BasicBlock* GetCurrentBasicBlock();
		void SetCurrentBasicBlock(llvm::BasicBlock* block);

	public:		// Callback configuration interface
		void SetThunkCallback(void* funcptr);
		void SetStringCallback(void* funcptr);

	private:	// Internal state
		std::unique_ptr<llvm::Module> LLVMModule;
		llvm::Function* InitFunction;
		llvm::Function* EntryPointFunction;

		llvm::IRBuilder<> LLVMBuilder;

		ThunkCallbackT ThunkCallback;
		StringCallbackT StringCallback;

		std::vector<llvm::Type*> PendingParamTypes;
		std::vector<llvm::Value*> PendingValues;

		std::map<unsigned, llvm::GlobalVariable*> CachedStrings;
		std::map<std::string, llvm::GlobalVariable*> CachedThunkFunctions;
	};

}

