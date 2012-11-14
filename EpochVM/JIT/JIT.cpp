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

	PointerType* stackptrtype = PointerType::get(Type::getInt32Ty(context), 0);
	PointerType* pstackptrtype = PointerType::get(stackptrtype, 0);


	std::vector<Type*> args;
	args.push_back(pstackptrtype);
	args.push_back(IntegerType::get(context, 32));
	FunctionType* dostufffunctype = FunctionType::get(Type::getVoidTy(context), args, false);

	Function* dostufffunc = Function::Create(dostufffunctype, Function::ExternalLinkage, "dostuff", module);

	BasicBlock* block = BasicBlock::Create(context, "entry", dostufffunc);
	builder.SetInsertPoint(block);

	Value* pstackptr = dostufffunc->arg_begin();
	
	JIT::JITContext jitcontext;
	jitcontext.Builder = &builder;
	jitcontext.FunctionExit = BasicBlock::Create(context, "exit", dostufffunc);
	
	Value* retval = NULL;
	unsigned numparams = 0;
	unsigned numreturns = 0;

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		switch(bytecode[offset++])
		{
		case Bytecode::Instructions::BeginEntity:
			{
				Bytecode::EntityTag entitytype = Fetch<Integer32>(bytecode, offset);
				jitcontext.EntityTypes.push(entitytype);

				StringHandle entityname = Fetch<StringHandle>(bytecode, offset);

				if(entitytype == Bytecode::EntityTags::Function)
				{				
					Value* stackptr = builder.CreateLoad(pstackptr, false);

					const ScopeDescription& scope = ownervm.GetScopeDescription(entityname);
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						switch(scope.GetVariableOrigin(i))
						{
						case VARIABLE_ORIGIN_RETURN:
							++numreturns;
							// Deliberate fallthrough

						case VARIABLE_ORIGIN_LOCAL:
							jitcontext.VariableMap[scope.GetVariableNameHandle(i)] = builder.CreateAlloca(Type::getInt32Ty(context));
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparams);
								++numparams;

								Value* local = builder.CreateAlloca(Type::getInt32Ty(context));
								jitcontext.VariableMap[scope.GetVariableNameHandle(i)] = local;
								Value* newstackptr = builder.CreateGEP(stackptr, offset);
								Value* stackval = builder.CreateLoad(newstackptr, false);
								builder.CreateStore(stackval, local, false);
							}
							break;
						}
					}
				}
				else
					ownervm.JITHelpers.EntityHelpers.find(entitytype)->second(jitcontext);
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				Bytecode::EntityTag tag = jitcontext.EntityTypes.top();
				jitcontext.EntityTypes.pop();
				if(tag != Bytecode::EntityTags::Function)
				{
					builder.CreateBr(jitcontext.EntityCheck);
					builder.SetInsertPoint(jitcontext.EntityExit);
				}
			}
			break;

		case Bytecode::Instructions::BeginChain:
			jitcontext.EntityCheck = BasicBlock::Create(context, "", dostufffunc);
			jitcontext.EntityBody = BasicBlock::Create(context, "", dostufffunc);
			jitcontext.EntityExit = BasicBlock::Create(context, "", dostufffunc);
			builder.CreateBr(jitcontext.EntityCheck);
			builder.SetInsertPoint(jitcontext.EntityCheck);
			break;

		case Bytecode::Instructions::EndChain:
			// TODO - support for multiple entities in a chain
			break;

		case Bytecode::Instructions::Read:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				Value* varvalue = builder.CreateLoad(jitcontext.VariableMap[target], false);
				jitcontext.ValuesOnStack.push(varvalue);
			}
			break;

		case Bytecode::Instructions::Assign:
			{
				builder.CreateStore(jitcontext.ValuesOnStack.top(), jitcontext.VariableMap[jitcontext.ReferencesOnStack.top()], false);
				jitcontext.ValuesOnStack.pop();
				jitcontext.ReferencesOnStack.pop();
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				Value* c = jitcontext.ValuesOnStack.top();

				ConstantInt* cint = dyn_cast<ConstantInt>(c);
				StringHandle vartarget = static_cast<StringHandle>(cint->getValue().getLimitedValue());
				jitcontext.ReferencesOnStack.push(vartarget);
				jitcontext.ValuesOnStack.pop();
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				StringHandle value = Fetch<StringHandle>(bytecode, offset);
				retval = builder.CreateLoad(jitcontext.VariableMap[value], false);
			}
			break;

		case Bytecode::Instructions::Return:
			builder.CreateBr(jitcontext.FunctionExit);
			break;

		case Bytecode::Instructions::Invoke:
		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				ownervm.JITHelpers.InvokeHelpers.find(target)->second(jitcontext);
			}
			break;

		case Bytecode::Instructions::Push:
			{
				Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(bytecode, offset);
				Constant* valueval;

				switch(type)
				{
				case Metadata::EpochType_Integer:
					{
						Integer32 value = Fetch<Integer32>(bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(context), value);
					}
					break;

				case Metadata::EpochType_String:
					{
						StringHandle value = Fetch<StringHandle>(bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(context), value);
					}
					break;

				case Metadata::EpochType_Integer16:
				case Metadata::EpochType_Boolean:
				case Metadata::EpochType_Real:
				case Metadata::EpochType_Buffer:
				default:
					throw FatalException("Unsupported type for JIT compilation");
				}

				jitcontext.ValuesOnStack.push(valueval);
			}
			break;

		case Bytecode::Instructions::Pop:
			jitcontext.ValuesOnStack.pop();
			break;

		default:
			throw FatalException("Unsupported instruction for JIT compilation");
		}
	}

	builder.SetInsertPoint(jitcontext.FunctionExit);

	Value* stackptr = builder.CreateLoad(pstackptr, false);
	Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparams - numreturns));
	Value* stackptr2 = builder.CreateGEP(stackptr, offset);
	if(retval)
		builder.CreateStore(retval, stackptr2, false);
	builder.CreateStore(stackptr2, pstackptr, false);

	builder.CreateRetVoid();

	//module->dump();

	verifyFunction(*dostufffunc);

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return 0;

	FunctionPassManager OurFPM(module);

	OurFPM.add(new TargetData(*ee->getTargetData()));
	OurFPM.add(createTypeBasedAliasAnalysisPass());
	OurFPM.add(createBasicAliasAnalysisPass());
	OurFPM.add(createCFGSimplificationPass());
	OurFPM.add(createScalarReplAggregatesPass());
	OurFPM.add(createEarlyCSEPass());
	OurFPM.add(createLowerExpectIntrinsicPass());

	OurFPM.doInitialization();

	OurFPM.run(*dostufffunc);


	PassManager OurMPM;
	OurMPM.add(new TargetData(*ee->getTargetData()));
	OurMPM.add(createTypeBasedAliasAnalysisPass());
	OurMPM.add(createBasicAliasAnalysisPass());
	OurMPM.add(createGlobalOptimizerPass());
	OurMPM.add(createIPSCCPPass());
	OurMPM.add(createDeadArgEliminationPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createPruneEHPass());
	OurMPM.add(createFunctionAttrsPass());
	OurMPM.add(createArgumentPromotionPass());
	OurMPM.add(createScalarReplAggregatesPass(-1, false));
	OurMPM.add(createEarlyCSEPass());
	OurMPM.add(createSimplifyLibCallsPass());
	OurMPM.add(createJumpThreadingPass());
	OurMPM.add(createCorrelatedValuePropagationPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createTailCallEliminationPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createReassociatePass());
	OurMPM.add(createLoopRotatePass());
	OurMPM.add(createLICMPass());
	OurMPM.add(createLoopUnswitchPass(false));
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createIndVarSimplifyPass());
	OurMPM.add(createLoopIdiomPass());
	OurMPM.add(createLoopDeletionPass());
	OurMPM.add(createLoopUnrollPass());
	OurMPM.add(createGVNPass());
	OurMPM.add(createMemCpyOptPass());
	OurMPM.add(createSCCPPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createJumpThreadingPass());
	OurMPM.add(createCorrelatedValuePropagationPass());
	OurMPM.add(createDeadStoreEliminationPass());
	OurMPM.add(createAggressiveDCEPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createInstructionCombiningPass());

	OurMPM.run(*module);

	module->dump();

	void* fptr = ee->getPointerToFunction(dostufffunc);
	return (JITExecPtr)fptr;
}

