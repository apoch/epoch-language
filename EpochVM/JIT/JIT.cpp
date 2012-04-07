#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"

#include <sstream>
#include <map>
#include <stack>

template <typename T>
T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
{
	const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
	InstructionOffset += sizeof(T);
	return static_cast<T>(*data);
}


JITExecPtr JITByteCode(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset)
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& context = getGlobalContext();

	std::ostringstream name;
	name << "EpochJIT_" << beginoffset;
	Module* module = new Module(name.str().c_str(), context);

	IRBuilder<> builder(context);


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
	
	std::map<StringHandle, Value*> variablemap;

	std::stack<Value*> valuesonstack;
	std::stack<StringHandle> refsonstack;
	std::stack<Bytecode::EntityTag> entitytypes;
	
	Value* retval = NULL;
	unsigned numparams = 0;

	// TODO - genericize
	BasicBlock* chaincheck = BasicBlock::Create(context, "chaincheck", dostufffunc);
					BasicBlock* entityblock = BasicBlock::Create(context, "entitybegin", dostufffunc);
					BasicBlock* entityexit = BasicBlock::Create(context, "entityend", dostufffunc);

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		switch(bytecode[offset++])
		{
		case Bytecode::Instructions::BeginEntity:
			{
				Bytecode::EntityTag entitytype = Fetch<Integer32>(bytecode, offset);
				entitytypes.push(entitytype);

				if(entitytype == 1)
				{
					StringHandle entityname = Fetch<StringHandle>(bytecode, offset);
				
					Value* stackptr = builder.CreateLoad(pstackptr, false);

					const ScopeDescription& scope = ownervm.GetScopeDescription(entityname);
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						switch(scope.GetVariableOrigin(i))
						{
						case VARIABLE_ORIGIN_LOCAL:
						case VARIABLE_ORIGIN_RETURN:
							variablemap[scope.GetVariableNameHandle(i)] = builder.CreateAlloca(Type::getInt32Ty(context));
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparams);
								++numparams;

								Value* local = builder.CreateAlloca(Type::getInt32Ty(context));
								variablemap[scope.GetVariableNameHandle(i)] = local;
								Value* newstackptr = builder.CreateGEP(stackptr, offset);
								Value* stackval = builder.CreateLoad(newstackptr, false);
								builder.CreateStore(stackval, local, false);
							}
							break;
						}
					}
				}
				else if(entitytype == 20)
				{
					Fetch<StringHandle>(bytecode, offset);
					builder.CreateCondBr(valuesonstack.top(), entityblock, entityexit);
					valuesonstack.pop();
					builder.SetInsertPoint(entityblock);
				}
				else
					throw std::runtime_error("Oops");
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				Bytecode::EntityTag tag = entitytypes.top();
				entitytypes.pop();
				if(tag == 1)
				{
					// TODO - this is probably fucked up for void returns
					if(!numparams)
						throw std::runtime_error("Buggery.");

					//Constant* poplocalsoffset = ConstantInt::get(Type::getInt32Ty(context), numparams);
					Value* stackptr = builder.CreateLoad(pstackptr, false);
					//Value* restorestack = builder.CreateGEP(stackptr, poplocalsoffset);

					Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparams - 1));
					Value* stackptr2 = builder.CreateGEP(stackptr, offset);
					if(retval)
						builder.CreateStore(retval, stackptr2, false);
					builder.CreateStore(stackptr2, pstackptr, false);
				}
				else
				{
					builder.CreateBr(chaincheck);
					builder.SetInsertPoint(entityexit);
				}
			}
			break;

		case Bytecode::Instructions::BeginChain:
			builder.CreateBr(chaincheck);
			builder.SetInsertPoint(chaincheck);
			break;

		case Bytecode::Instructions::EndChain:
			// TODO - close out the current basic block
			break;

		case Bytecode::Instructions::Read:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(-1));

				//Value* stackptr = builder.CreateLoad(pstackptr, false);
				//Value* newstackptr = builder.CreateGEP(stackptr, offset);
				Value* varvalue = builder.CreateLoad(variablemap[target], false);
				//builder.CreateStore(varvalue, newstackptr, false);
				//builder.CreateStore(newstackptr, pstackptr, false);
				valuesonstack.push(varvalue);
			}
			break;

		case Bytecode::Instructions::Assign:
			{
				// TODO
				//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

				//Value* stackptr = builder.CreateLoad(pstackptr, false);
				builder.CreateStore(valuesonstack.top(), variablemap[refsonstack.top()], false);
				//Value* newstackptr = builder.CreateGEP(stackptr, offset);
				//builder.CreateStore(newstackptr, pstackptr, false);
				valuesonstack.pop();
				refsonstack.pop();
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				// TODO
				//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

				//Value* stackptr = builder.CreateLoad(pstackptr, false);
				//Value* newstackptr = builder.CreateGEP(stackptr, offset);
				//builder.CreateStore(newstackptr, pstackptr, false);

				Value* c = valuesonstack.top();

				ConstantInt* cint = dyn_cast<ConstantInt>(c);
				StringHandle vartarget = static_cast<StringHandle>(cint->getValue().getLimitedValue());
				refsonstack.push(vartarget);

				valuesonstack.pop();
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				StringHandle value = Fetch<StringHandle>(bytecode, offset);
				retval = builder.CreateLoad(variablemap[value], false);
			}
			break;

		case Bytecode::Instructions::Return:
			// TODO - handle premature rets
			break;

		case Bytecode::Instructions::Invoke:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				if(target == 41)
				{
					Value* p2 = valuesonstack.top();
					valuesonstack.pop();
					Value* p1 = valuesonstack.top();
					valuesonstack.pop();
					Value* flag = builder.CreateICmp(CmpInst::ICMP_SLT, p1, p2);
					valuesonstack.push(flag);
				}
				else
					throw std::runtime_error("Invalid function to invoke from native code");
			}
			break;

		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				if(target == 15)
				{
					//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

					Value* p2 = valuesonstack.top();
					valuesonstack.pop();
					Value* p1 = valuesonstack.top();
					valuesonstack.pop();

					//Value* stackptr = builder.CreateLoad(pstackptr, false);
					//Value* p2 = builder.CreateLoad(stackptr, false);
					//Value* p1ptr = builder.CreateGEP(stackptr, offset);
					//Value* p1 = builder.CreateLoad(p1ptr, false);
					Value* result = builder.CreateAdd(p1, p2);
					//builder.CreateStore(result, p1ptr, false);
					//builder.CreateStore(p1ptr, pstackptr, false);
					valuesonstack.push(result);
				}
				else if(target == 17)
				{
					//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 1);

					Value* p2 = valuesonstack.top();
					valuesonstack.pop();
					Value* p1 = valuesonstack.top();
					valuesonstack.pop();

					//Value* stackptr = builder.CreateLoad(pstackptr, false);
					//Value* p2 = builder.CreateLoad(stackptr, false);
					//Value* p1ptr = builder.CreateGEP(stackptr, offset);
					//Value* p1 = builder.CreateLoad(p1ptr, false);
					Value* result = builder.CreateMul(p1, p2);
					//builder.CreateStore(result, p1ptr, false);
					//builder.CreateStore(p1ptr, pstackptr, false);
					valuesonstack.push(result);
				}
				else if(target == 4)
				{
					Value* p2 = valuesonstack.top();
					valuesonstack.pop();
					Value* c = valuesonstack.top();
					valuesonstack.pop();

					ConstantInt* cint = dyn_cast<ConstantInt>(c);
					StringHandle vartarget = static_cast<StringHandle>(cint->getValue().getLimitedValue());

					//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), 2);
					//Value* stackptr = builder.CreateLoad(pstackptr, false);
					//Value* p2 = builder.CreateLoad(stackptr, false);
					//Value* poppedptr = builder.CreateGEP(stackptr, offset);
					//builder.CreateStore(poppedptr, pstackptr, false);

					builder.CreateStore(p2, variablemap[vartarget], false);
				}
				else
				{
					throw std::runtime_error("Bollocks.");
				}
			}
			break;

		case Bytecode::Instructions::Push:
			{
				VM::EpochTypeID type = Fetch<VM::EpochTypeID>(bytecode, offset);
				ConstantInt* valueval;

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

				//Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(-1));

				//Value* stackptr = builder.CreateLoad(pstackptr, false);
				//Value* newstackptr = builder.CreateGEP(stackptr, offset);
				//builder.CreateStore(valueval, newstackptr, false);
				//builder.CreateStore(newstackptr, pstackptr, false);
				valuesonstack.push(valueval);
			}
			break;

		case Bytecode::Instructions::Pop:
			valuesonstack.pop();
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
  OurFPM.add(createPromoteMemoryToRegisterPass());
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

