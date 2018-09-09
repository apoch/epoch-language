#pragma once


namespace CodeGenInternal
{
	class TrivialMemoryManager;
}


class CodeGenContext
{
public:
	CodeGenContext();
	~CodeGenContext();

public:
	llvm::FunctionType* TypeCreateFunction();
	void TypeQueueFunctionParameter(llvm::Type* ty);

	llvm::Type* TypeGetString();

	llvm::Function* FunctionCreate(llvm::FunctionType* fty, const char* name);
	llvm::GlobalVariable* FunctionCreateThunk(llvm::FunctionType* fty, const char* name);

	llvm::BasicBlock* BasicBlockCreate(llvm::Function* func);
	void BasicBlockSetInsertPoint(llvm::BasicBlock* block);

	llvm::Value* CodeCreateCall(llvm::Function* target);
	llvm::Value* CodeCreateCallThunk(llvm::GlobalVariable* target);
	void CodeCreateRetVoid();

	void CodePushValue();

public:
	void CreateBinaryModule();
	void RelocateBuffers(unsigned codeOffset, unsigned xDataOffset);
	void FinalizeBinaryModule(unsigned moduleBaseAddress, unsigned codeOffset);

	void* GetCodeBuffer(unsigned* outSize);
	void* GetDebugBuffer(unsigned* outSize);
	void* GetDebugRelocBuffer(unsigned* outSize);
	void* GetDebugSymbolsBuffer(unsigned* outSize, unsigned* outCount);
	void* GetPDataBuffer(unsigned* outSize);
	void* GetXDataBuffer(unsigned* outSize);

public:
	void DebugDump();

private:
	llvm::DIType* TypeGetDebugType(llvm::Type* t);

private:
	llvm::LLVMContext GlobalContext;
	std::unique_ptr<llvm::Module> LLVMModule;
	llvm::IRBuilder<> Builder;
	llvm::DIBuilder DebugBuilder;

	std::vector<char> CodeBuffer;
	std::vector<char> PData;
	std::vector<char> DebugData;
	std::vector<char> DebugRelocs;
	std::vector<char> DebugSymbols;

	std::vector<llvm::Value*> ValueStack;
	std::vector<llvm::Type*> FunctionParamTypeStack;

	uint64_t EmissionAddress = 0;
	size_t EmissionSize = 0;
	uint64_t EmittedPData = 0;
	size_t EmittedPDataSize = 0;
	uint64_t EmittedXData = 0;
	size_t EmittedXDataSize = 0;
	const llvm::object::ObjectFile* EmittedImage;

	llvm::ExecutionEngine* CachedExecutionEngine;
	CodeGenInternal::TrivialMemoryManager* CachedMemoryManager;

	llvm::DIFile* DebugFile;
	llvm::DICompileUnit* DebugCompileUnit;

	unsigned DebugSymbolCount = 0;
};

