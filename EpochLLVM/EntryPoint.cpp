#include "pch.h"

int _tmain(int argc, _TCHAR* argv[])
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& Context = getGlobalContext();

	Module* module = new Module("EpochJIT", Context);

	IRBuilder<> Builder(Context);

	std::map<std::string, Value*> NamedValues;


	std::vector<Type*> args(1, Type::getInt32Ty(Context));
	FunctionType* functype = FunctionType::get(Type::getInt32Ty(Context), args, false);

	Function* func = Function::Create(functype, Function::ExternalLinkage, "answer", module);

	std::vector<std::string> Args;
	Args.push_back("a");

	unsigned Idx = 0;
	for(Function::arg_iterator AI = func->arg_begin(); Idx != Args.size(); ++AI, ++Idx)
	{
		AI->setName(Args[Idx]);
		NamedValues[Args[Idx]] = AI;
	}

	BasicBlock* block = BasicBlock::Create(Context, "entry", func);
	Builder.SetInsertPoint(block);

	std::vector<Type*> noargs;
	FunctionType* getstufffunctype = FunctionType::get(Type::getInt32Ty(Context), noargs, false);
	Function* getstufffunc = Function::Create(getstufffunctype, Function::ExternalLinkage, "getstuff", module);

	Value* stuff = Builder.CreateCall(getstufffunc);

	Value* addition = Builder.CreateAdd(NamedValues["a"], stuff, "addtmp");

	Builder.CreateRet(addition);

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
