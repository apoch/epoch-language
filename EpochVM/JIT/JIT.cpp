#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include <sstream>

template <typename T>
T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
{
	const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
	InstructionOffset += sizeof(T);
	return static_cast<T>(*data);
}


JITExecPtr JITByteCode(const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset)
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& context = getGlobalContext();

	std::ostringstream name;
	name << "EpochJIT_" << beginoffset;
	Module* module = new Module(name.str().c_str(), context);

	IRBuilder<> builder(context);


	void JITInvoke(char** stack, void* context, StringHandle target);

	std::vector<Type*> invokeargs;
	invokeargs.push_back(IntegerType::get(context, 32));
	invokeargs.push_back(IntegerType::get(context, 32));
	invokeargs.push_back(IntegerType::get(context, 32));
	FunctionType* invokefunctype = FunctionType::get(Type::getVoidTy(context), invokeargs, false);

	PointerType* invokefuncptrtype = PointerType::get(invokefunctype, 0);
	ConstantInt* invokefuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITInvoke));
	Constant* invokefuncptr = ConstantExpr::getCast(Instruction::IntToPtr, invokefuncptrasint, invokefuncptrtype);


	void JITRead(char** stack, void* context, StringHandle target);

	std::vector<Type*> readargs;
	readargs.push_back(IntegerType::get(context, 32));
	readargs.push_back(IntegerType::get(context, 32));
	readargs.push_back(IntegerType::get(context, 32));
	FunctionType* readfunctype = FunctionType::get(Type::getVoidTy(context), readargs, false);

	PointerType* readfuncptrtype = PointerType::get(readfunctype, 0);
	ConstantInt* readfuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITRead));
	Constant* readfuncptr = ConstantExpr::getCast(Instruction::IntToPtr, readfuncptrasint, readfuncptrtype);


	void JITWrite(char** stack, void* context);

	std::vector<Type*> writeargs;
	writeargs.push_back(IntegerType::get(context, 32));
	writeargs.push_back(IntegerType::get(context, 32));
	FunctionType* writefunctype = FunctionType::get(Type::getVoidTy(context), writeargs, false);

	PointerType* writefuncptrtype = PointerType::get(writefunctype, 0);
	ConstantInt* writefuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITWrite));
	Constant* writefuncptr = ConstantExpr::getCast(Instruction::IntToPtr, writefuncptrasint, writefuncptrtype);


	void JITBindRef(char** stack, void* context);

	std::vector<Type*> bindrefargs;
	bindrefargs.push_back(IntegerType::get(context, 32));
	bindrefargs.push_back(IntegerType::get(context, 32));
	FunctionType* bindreffunctype = FunctionType::get(Type::getVoidTy(context), bindrefargs, false);

	PointerType* bindreffuncptrtype = PointerType::get(bindreffunctype, 0);
	ConstantInt* bindreffuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITBindRef));
	Constant* bindreffuncptr = ConstantExpr::getCast(Instruction::IntToPtr, bindreffuncptrasint, bindreffuncptrtype);



	void JITBeginEntity(void* context, StringHandle target);

	std::vector<Type*> beginentityargs;
	beginentityargs.push_back(IntegerType::get(context, 32));
	beginentityargs.push_back(IntegerType::get(context, 32));
	FunctionType* beginentityfunctype = FunctionType::get(Type::getVoidTy(context), beginentityargs, false);

	PointerType* beginentityfuncptrtype = PointerType::get(beginentityfunctype, 0);
	ConstantInt* beginentityfuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITBeginEntity));
	Constant* beginentityfuncptr = ConstantExpr::getCast(Instruction::IntToPtr, beginentityfuncptrasint, beginentityfuncptrtype);


	void JITEndEntity(void* context);

	std::vector<Type*> endentityargs;
	endentityargs.push_back(IntegerType::get(context, 32));
	FunctionType* endentityfunctype = FunctionType::get(Type::getVoidTy(context), endentityargs, false);

	PointerType* endentityfuncptrtype = PointerType::get(endentityfunctype, 0);
	ConstantInt* endentityfuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITEndEntity));
	Constant* endentityfuncptr = ConstantExpr::getCast(Instruction::IntToPtr, endentityfuncptrasint, endentityfuncptrtype);


	void JITSetRegister(void* context, StringHandle variable);

	std::vector<Type*> setregargs;
	setregargs.push_back(IntegerType::get(context, 32));
	setregargs.push_back(IntegerType::get(context, 32));
	FunctionType* setregfunctype = FunctionType::get(Type::getVoidTy(context), setregargs, false);

	PointerType* setregfuncptrtype = PointerType::get(setregfunctype, 0);
	ConstantInt* setregfuncptrasint = ConstantInt::get(Type::getInt32Ty(context), (uint64_t)(JITSetRegister));
	Constant* setregfuncptr = ConstantExpr::getCast(Instruction::IntToPtr, setregfuncptrasint, setregfuncptrtype);


	std::vector<Type*> args;
	args.push_back(IntegerType::get(context, 32));
	args.push_back(IntegerType::get(context, 32));
	FunctionType* dostufffunctype = FunctionType::get(Type::getVoidTy(context), args, false);

	Function* dostufffunc = Function::Create(dostufffunctype, Function::ExternalLinkage, "dostuff", module);

	BasicBlock* block = BasicBlock::Create(context, "entry", dostufffunc);
	builder.SetInsertPoint(block);

	PointerType* stackptrtype = PointerType::get(Type::getInt32Ty(context), 0);
	PointerType* pstackptrtype = PointerType::get(stackptrtype, 0);

	Value* pstackptr = builder.CreateIntToPtr(dostufffunc->arg_begin(), pstackptrtype);

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		switch(bytecode[offset++])
		{
		case Bytecode::Instructions::BeginEntity:
			{
				// TODO - handle other entity types
				Fetch<Integer32>(bytecode, offset);

				StringHandle entityname = Fetch<StringHandle>(bytecode, offset);
				Value* valueval = ConstantInt::get(Type::getInt32Ty(context), entityname);

				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back((++iter));
				args.push_back(valueval);
				CallInst::Create(beginentityfuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back((++iter));
				CallInst::Create(endentityfuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::Read:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				Value* valueval = ConstantInt::get(Type::getInt32Ty(context), target);

				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back(iter);
				args.push_back((++iter));
				args.push_back(valueval);
				CallInst::Create(readfuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::Assign:
			{
				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back(iter);
				args.push_back((++iter));
				CallInst::Create(writefuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back(iter);
				args.push_back((++iter));
				CallInst::Create(bindreffuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				StringHandle value = Fetch<StringHandle>(bytecode, offset);
				Value* valueval = ConstantInt::get(Type::getInt32Ty(context), value);

				std::vector<Value*> args;
				Function::arg_iterator iter = dostufffunc->arg_begin();
				args.push_back((++iter));
				args.push_back(valueval);
				CallInst::Create(setregfuncptr, args, "", block);
			}
			break;

		case Bytecode::Instructions::Return:
			// TODO - handle premature rets
			break;

		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				if(target == 15)
				{
					Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

					Value* stackptr = builder.CreateLoad(pstackptr, false);
					Value* p2 = builder.CreateLoad(stackptr, false);
					Value* p1ptr = builder.CreateGEP(stackptr, offset);
					Value* p1 = builder.CreateLoad(p1ptr, false);
					Value* result = builder.CreateAdd(p1, p2);
					builder.CreateStore(result, p1ptr, false);
					builder.CreateStore(p1ptr, pstackptr, false);
				}
				else if(target == 17)
				{
					Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

					Value* stackptr = builder.CreateLoad(pstackptr, false);
					Value* p2 = builder.CreateLoad(stackptr, false);
					Value* p1ptr = builder.CreateGEP(stackptr, offset);
					Value* p1 = builder.CreateLoad(p1ptr, false);
					Value* result = builder.CreateMul(p1, p2);
					builder.CreateStore(result, p1ptr, false);
					builder.CreateStore(p1ptr, pstackptr, false);
				}
				else
				{
					Value* valueval = ConstantInt::get(Type::getInt32Ty(context), target);

					std::vector<Value*> args;
					Function::arg_iterator iter = dostufffunc->arg_begin();
					args.push_back(iter);
					args.push_back((++iter));
					args.push_back(valueval);
					CallInst::Create(invokefuncptr, args, "", block);
				}
			}
			break;

		case Bytecode::Instructions::Push:
			{
				VM::EpochTypeID type = Fetch<VM::EpochTypeID>(bytecode, offset);
				Value* valueval;

				switch(type)
				{
				case VM::EpochType_Integer:
					{
						Integer32 value = Fetch<Integer32>(bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(context), value);
					}
					break;

				case VM::EpochType_String:
					{
						StringHandle value = Fetch<StringHandle>(bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(context), value);
					}
					break;

				case VM::EpochType_Integer16:
				case VM::EpochType_Boolean:
				case VM::EpochType_Real:
				case VM::EpochType_Buffer:
				default:
					throw FatalException("Unsupported type for JIT compilation");
				}

				Constant* offset = ConstantInt::get(Type::getInt32Ty(context), -1);

				Value* stackptr = builder.CreateLoad(pstackptr, false);
				Value* newstackptr = builder.CreateGEP(stackptr, offset);
				builder.CreateStore(valueval, newstackptr, false);
				builder.CreateStore(newstackptr, pstackptr, false);
			}
			break;

		default:
			throw FatalException("Unsupported instruction for JIT compilation");
		}
	}

			
	builder.CreateRetVoid();
	verifyFunction(*dostufffunc);

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return 0;

  FunctionPassManager OurFPM(module);

  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new TargetData(*ee->getTargetData()));
  // Provide basic AliasAnalysis support for GVN.
  OurFPM.add(createBasicAliasAnalysisPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());

  OurFPM.doInitialization();

  OurFPM.run(*dostufffunc);

	module->dump();

	void* fptr = ee->getPointerToFunction(dostufffunc);
	return (JITExecPtr)fptr;
}

