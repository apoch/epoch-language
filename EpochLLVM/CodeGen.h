#pragma once


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
	void DebugDump();

private:
	llvm::LLVMContext GlobalContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> LLVMModule;
};

