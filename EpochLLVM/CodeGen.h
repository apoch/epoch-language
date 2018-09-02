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

	llvm::Function* FunctionCreate(llvm::FunctionType* fty);

	llvm::BasicBlock* BasicBlockCreate(llvm::Function* func);
	void BasicBlockSetInsertPoint(llvm::BasicBlock* block);

	void CodeCreateRetVoid();

public:
	void CreateBinaryModule();
	void FinalizeBinaryModule(unsigned codeBaseAddress);

	void* GetCodeBuffer(unsigned* outSize);

public:
	void DebugDump();

private:
	llvm::LLVMContext GlobalContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> LLVMModule;

	std::vector<char> CodeBuffer;
	std::vector<char> PData;
	std::vector<char> XData;
	std::vector<char> DebugData;

	uint64_t EmissionAddress = 0;
	size_t EmissionSize = 0;
	const llvm::object::ObjectFile* EmittedImage;

	llvm::ExecutionEngine* CachedExecutionEngine;
	CodeGenInternal::TrivialMemoryManager* CachedMemoryManager;
};

