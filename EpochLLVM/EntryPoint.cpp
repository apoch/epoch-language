#include "pch.h"

int _tmain(int argc, _TCHAR* argv[])
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& context = getGlobalContext();

	Module* module = new Module("EpochJIT", context);

	IRBuilder<> builder(context);

	std::vector<Type*> args(1, Type::getInt32Ty(context));
	FunctionType* functype = FunctionType::get(Type::getInt32Ty(context), args, false);

	Function* func = Function::Create(functype, Function::ExternalLinkage, "answer", module);

	func->arg_begin()->setName("a");

	BasicBlock* block = BasicBlock::Create(context, "entry", func);
	builder.SetInsertPoint(block);

	std::vector<Type*> noargs;
	FunctionType* getstufffunctype = FunctionType::get(Type::getInt32Ty(context), noargs, false);
	Function* getstufffunc = Function::Create(getstufffunctype, Function::ExternalLinkage, "getstuff", module);

	Value* stuff = builder.CreateCall(getstufffunc, "stuff");

	Value* addition = builder.CreateAdd(func->arg_begin(), stuff, "addition");

	builder.CreateRet(addition);

	verifyFunction(*func);

	module->dump();

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
	{
		std::cout << ErrStr << std::endl;
		return 1;
	}

	void* fptr = ee->getPointerToFunction(func);
	int (*fp)(int) = (int (*)(int))fptr;

	std::wcout << fp(2) << std::endl;

	return 0;
}
