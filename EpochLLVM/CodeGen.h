#pragma once


class CodeGenContext
{
public:
	CodeGenContext();
	~CodeGenContext();

public:
	void DebugDump();

private:
	llvm::LLVMContext GlobalContext;
	std::unique_ptr<llvm::Module> LLVMModule;
};

