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

	llvm::Function* FunctionCreate(llvm::FunctionType* fty, const char* name);

	llvm::BasicBlock* BasicBlockCreate(llvm::Function* func);
	void BasicBlockSetInsertPoint(llvm::BasicBlock* block);

	llvm::Value* CodeCreateCall(llvm::Function* target);
	void CodeCreateRetVoid();

public:
	void CreateBinaryModule();
	void RelocateBuffers(unsigned codeOffset, unsigned xDataOffset);
	void FinalizeBinaryModule(unsigned moduleBaseAddress, unsigned codeOffset);

	void* GetCodeBuffer(unsigned* outSize);
	void* GetPDataBuffer(unsigned* outSize);
	void* GetXDataBuffer(unsigned* outSize);

public:
	void DebugDump();

private:
	llvm::LLVMContext GlobalContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> LLVMModule;

	std::vector<char> CodeBuffer;
	std::vector<char> PData;

	uint64_t EmissionAddress = 0;
	size_t EmissionSize = 0;
	uint64_t EmittedPData = 0;
	size_t EmittedPDataSize = 0;
	uint64_t EmittedXData = 0;
	size_t EmittedXDataSize = 0;
	const llvm::object::ObjectFile* EmittedImage;

	llvm::ExecutionEngine* CachedExecutionEngine;
	CodeGenInternal::TrivialMemoryManager* CachedMemoryManager;
};

