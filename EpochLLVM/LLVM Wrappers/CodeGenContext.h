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

	namespace object
	{
		class ObjectFile;
	}
}

namespace CodeGenInternal
{
	class TrivialMemoryManager;
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
		void FunctionTypePush();

		llvm::Type* TypeGetBoolean();
		llvm::Type* TypeGetInteger();
		llvm::Type* TypeGetInteger16();
		llvm::Type* TypeGetInteger64();
		llvm::Type* TypeGetPointerTo(llvm::Type* raw);
		llvm::Type* TypeGetReal();
		llvm::Type* TypeGetString();
		llvm::Type* TypeGetVoid();
		llvm::Type* TypeGetBuffer();
		llvm::Type* TypeGetArrayOfType(llvm::Type* elementtype, int arity);

		llvm::DIType* TypeGetDebugType(llvm::Type* t);

		llvm::Type* StructureTypeCreate(const char* name);
		void StructureTypeQueueMember(llvm::Type* membertype);

		llvm::Type* SumTypeCreate(const char* name, unsigned width);

	public:		// Function management interface
		llvm::Function* FunctionCreate(const char* name, llvm::FunctionType* fty);
		llvm::GlobalVariable* FunctionCreateThunk(const char* name, llvm::FunctionType* fty);
		void FunctionFinalize();
		void FunctionQueueParamType(llvm::Type* ty, const char* name);

	public:		// Instruction management interface
		llvm::AllocaInst * CodeCreateAlloca(llvm::Type* vartype, const char* varname, unsigned origin);
		llvm::BasicBlock* CodeCreateBasicBlock(llvm::Function* parent, bool setinsertpoint);
		void CodeCreateBranch(llvm::BasicBlock* target, bool setinsertpoint);
		llvm::CallInst* CodeCreateCall(llvm::Function* target);
		llvm::CallInst* CodeCreateCallIndirect(llvm::AllocaInst* target);
		llvm::CallInst* CodeCreateCallThunk(llvm::GlobalVariable* target);
		void CodeCreateCast(llvm::Type* targettype);
		void CodeCreateCondBranch(llvm::Value* cond, llvm::BasicBlock* truetarget, llvm::BasicBlock* falsetarget);
		void CodeCreateDereference();
		llvm::Value* CodeCreateGEP(unsigned index);
		llvm::GlobalVariable* CodeCreateGlobal(llvm::Type* type, const char* varname);
		void CodeCreateRead(llvm::AllocaInst* allocatarget);
		void* CodeCreateReadArray(llvm::AllocaInst* allocatarget);
		void CodeCreateReadParam(unsigned index);
		void CodeCreateReadStructure(llvm::Value* gep);
		void CodeCreateRet();
		void CodeCreateRetVoid();
		void CodeCreateWrite(llvm::AllocaInst* allocatarget);
		void CodeCreateWrite(llvm::GlobalVariable* globaltarget);
		void CodeCreateWriteIndirect(llvm::AllocaInst* allocatarget);
		void CodeCreateWriteParam(unsigned index);
		void CodeCreateWriteStructure(llvm::Value* gep);
		void CodeCreateWriteStructurePop();
		void CodeCreateWriteStructurePopSumType();

		void CodeCreateOperatorBooleanNot();
		void CodeCreateOperatorBooleanAnd();
		void CodeCreateOperatorIntegerBitwiseAnd();
		void CodeCreateOperatorIntegerEquals();
		void CodeCreateOperatorIntegerNotEquals();
		void CodeCreateOperatorIntegerGreaterThan();
		void CodeCreateOperatorIntegerLessThan();
		void CodeCreateOperatorIntegerPlus();
		void CodeCreateOperatorIntegerMinus();
		void CodeCreateOperatorIntegerDivide();
		void CodeCreateOperatorIntegerMultiply();

		void CodePushAllocate();
		void CodePushBoolean(bool value);
		void CodePushInteger(int value);
		void CodePushInteger16(short value);
		void CodePushInteger64(uint64_t value);
		void CodePushReal(float value);
		void CodePushRawAlloca(llvm::AllocaInst* alloc);
		void CodePushRawCall(llvm::CallInst* callinst);
		void CodePushRawGEP(llvm::Value* gep);
		void CodePushRawGlobal(llvm::GlobalVariable* global);
		void CodePushString(unsigned handle);
		void CodePushFunction(llvm::Function* func);
		void CodePushExtractedStructValue(unsigned memberindex);
		void CodePushNothing();

		llvm::Value* CodePopValue();

		void CodeStatementFinalize(unsigned line, unsigned column);
		void TagDebugLocation(llvm::Instruction* instr, unsigned line, unsigned column);

		void SumTypeMerge();

	public:		// Object code emission interface
		void PrepareBinaryObject();
		size_t EmitBinaryObject(char* buffer, size_t maxoutput, unsigned entrypointaddress, unsigned gcaddress);

		void RelocateBuffers(unsigned offsetxdata, unsigned offsettext);

	public:		// Miscellaneous configuration interface
		void SetEntryFunction(llvm::Function* func);

		llvm::BasicBlock* GetCurrentBasicBlock();
		void SetCurrentBasicBlock(llvm::BasicBlock* block);

	public:		// Callback configuration interface
		void SetThunkCallback(void* funcptr);
		void SetStringCallback(void* funcptr);

	public:		// Extra section handling interface
		unsigned SectionGetPDataSize() const;
		unsigned SectionGetXDataSize() const;
		unsigned SectionGetGCSize() const;
		unsigned SectionGetDebugSize() const;
		unsigned SectionGetDebugRelocSize() const;
		unsigned SectionGetDebugSymbolSize() const;

		void SectionCopyPData(void* buffer) const;
		void SectionCopyXData(void* buffer) const;
		void SectionCopyGC(void* buffer) const;
		void SectionCopyDebug(void* buffer) const;
		void SectionCopyDebugReloc(void* buffer) const;
		uint32_t SectionCopyDebugSymbols(void* buffer) const;

	private:	// Helpers
		void SetupDebugInfo(llvm::Function* function);
		void TagDebugLine(unsigned line, unsigned column);

	private:	// Internal state
		llvm::LLVMContext GlobalContext;
		std::unique_ptr<llvm::Module> LLVMModule;
		llvm::Function* InitFunction;
		llvm::Function* EntryPointFunction;

		llvm::Function* GCRootFunction;

		llvm::GlobalVariable* AllocatorThunk;

		llvm::IRBuilder<> LLVMBuilder;

		llvm::DIBuilder DebugBuilder;
		llvm::DICompileUnit* DebugCompileUnit;
		llvm::DIFile* DebugFile;

		ThunkCallbackT ThunkCallback;
		StringCallbackT StringCallback;

		std::vector<std::vector<llvm::Type*>> PendingParamTypeStack;
		std::vector<std::vector<std::string>*> PendingParamNameStack;
		std::map<llvm::FunctionType*, std::vector<std::string>*> PendingParamNamesSet;
		std::vector<llvm::Type*> PendingMemberTypes;
		std::vector<llvm::Value*> PendingValues;

		std::map<unsigned, llvm::GlobalVariable*> CachedStrings;
		std::map<std::string, llvm::GlobalVariable*> CachedThunkFunctions;

		std::vector<char> PData;
		std::vector<char> XData;
		std::vector<char> GCSection;
		std::vector<char> DebugData;
		std::vector<char> DebugRelocs;
		std::vector<char> DebugSymbols;

		uint64_t EmissionAddress;
		size_t EmissionSize;

		uint32_t DebugSymbolCount = 0;

		const llvm::object::ObjectFile* EmittedImage;
		llvm::ExecutionEngine* CachedExecutionEngine;
		CodeGenInternal::TrivialMemoryManager * CachedMemoryManager;
	};

}

