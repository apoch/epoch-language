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
	args.push_back(Type::getInt1PtrTy(context));
	FunctionType* dostufffunctype = FunctionType::get(Type::getVoidTy(context), args, false);

	Function* dostufffunc = Function::Create(dostufffunctype, Function::ExternalLinkage, "dostuff", module);


	std::map<Value*, Value*> structurelookupcache;

	std::vector<Type*> vmargs;
	vmargs.push_back(Type::getInt1PtrTy(context));
	vmargs.push_back(IntegerType::get(context, 32));
	FunctionType* vmgetstructuretype = FunctionType::get(Type::getInt1PtrTy(context), vmargs, false);

	Function* vmgetstructure = Function::Create(vmgetstructuretype, Function::ExternalLinkage, "VMGetStructure", module);


	BasicBlock* block = BasicBlock::Create(context, "entry", dostufffunc);
	builder.SetInsertPoint(block);

	Value* pstackptr = dostufffunc->arg_begin();
	Value* vmcontextptr = ++(dostufffunc->arg_begin());
	
	JIT::JITContext jitcontext;
	jitcontext.Builder = &builder;
	jitcontext.FunctionExit = BasicBlock::Create(context, "exit", dostufffunc);
	jitcontext.PStackPtr = pstackptr;
	jitcontext.Context = &context;
	
	Value* retval = NULL;
	unsigned numparams = 0;
	unsigned numreturns = 0;

	const ScopeDescription* curscope = NULL;

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = bytecode[offset++];
		switch(instruction)
		{
		case Bytecode::Instructions::BeginEntity:
			{
				Bytecode::EntityTag entitytype = Fetch<Integer32>(bytecode, offset);
				jitcontext.EntityTypes.push(entitytype);

				StringHandle entityname = Fetch<StringHandle>(bytecode, offset);

				if(entitytype == Bytecode::EntityTags::Function)
				{				
					Value* stackptr = builder.CreateLoad(pstackptr, false);
					Type* type = NULL;

					const ScopeDescription& scope = ownervm.GetScopeDescription(entityname);
					curscope = &scope;
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);
						Metadata::EpochTypeFamily varfamily = Metadata::GetTypeFamily(vartype);
						switch(vartype)
						{
						case Metadata::EpochType_Integer:
							type = Type::getInt32Ty(context);
							break;

						case Metadata::EpochType_Real:
							type = Type::getFloatTy(context);
							break;

						default:
							if(varfamily != Metadata::EpochTypeFamily_Structure && varfamily != Metadata::EpochTypeFamily_TemplateInstance)
								throw NotImplementedException("Unsupported type for native code generation");
						}

						switch(scope.GetVariableOrigin(i))
						{
						case VARIABLE_ORIGIN_RETURN:
							++numreturns;
							// Deliberate fallthrough

						case VARIABLE_ORIGIN_LOCAL:
							jitcontext.VariableMap[i] = builder.CreateAlloca(type);
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparams);
								++numparams;

								if(scope.IsReference(i))
								{
									// TODO - support references for non-structure types
									if(varfamily == Metadata::EpochTypeFamily_Structure || varfamily == Metadata::EpochTypeFamily_TemplateInstance)
									{
										type = Type::getInt32Ty(context);
										Value* local = builder.CreateAlloca(type);
										jitcontext.VariableMap[i] = local;
										Value* newstackptr = builder.CreateGEP(stackptr, offset);
										Value* stackval = builder.CreateLoad(newstackptr, false);
										builder.CreateStore(stackval, local, false);
									}
									else
										throw NotImplementedException("Can't take reference to this type");
								}
								else
								{
									Value* local = builder.CreateAlloca(type);
									jitcontext.VariableMap[i] = local;
									Value* newstackptr = builder.CreateGEP(stackptr, offset);
									Value* stackval = builder.CreateLoad(newstackptr, false);
									builder.CreateStore(stackval, local, false);
								}
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

		case Bytecode::Instructions::Assign:
			{
				jitcontext.ValuesOnStack.pop();
				builder.CreateStore(jitcontext.ValuesOnStack.top(), jitcontext.VariableMap[jitcontext.ReferencesOnStack.top()], false);
				jitcontext.ReferencesOnStack.pop();
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				size_t frames = Fetch<size_t>(bytecode, offset);
				size_t index = Fetch<size_t>(bytecode, offset);

				if(frames > 0)
					throw NotImplementedException("Scope is not flat!");

				Value* ptr = jitcontext.VariableMap[index];
				jitcontext.ValuesOnStack.push(ptr);
				jitcontext.ReferencesOnStack.push(index);
			}
			break;

		case Bytecode::Instructions::BindMemberRef:
			{
				// TODO - support nested structures!
				StringHandle membername = Fetch<StringHandle>(bytecode, offset);
				size_t varindex = jitcontext.ReferencesOnStack.top();

				Metadata::EpochTypeID structuretype = curscope->GetVariableTypeByIndex(varindex);
				const StructureDefinition& def = ownervm.GetStructureDefinition(structuretype);
				size_t memberindex = def.FindMember(membername);

				Metadata::EpochTypeID membertype = def.GetMemberType(memberindex);
				size_t memberoffset = def.GetMemberOffset(memberindex);

				Value* voidstructptr = structurelookupcache[jitcontext.ValuesOnStack.top()];

				if(!voidstructptr)
				{
					Value* structurehandle = builder.CreateLoad(jitcontext.ValuesOnStack.top());
					voidstructptr = builder.CreateCall2(vmgetstructure, vmcontextptr, structurehandle);
					structurelookupcache[jitcontext.ValuesOnStack.top()] = voidstructptr;
				}


				Value* bytestructptr = builder.CreatePointerCast(voidstructptr, Type::getInt1PtrTy(context));
				Value* voidmemberptr = builder.CreateGEP(bytestructptr, ConstantInt::get(Type::getInt32Ty(context), memberoffset));

				if(membertype != Metadata::EpochType_Real)
					throw NotImplementedException("I am lazy.");

				Type* pfinaltype = Type::getFloatPtrTy(context);
				Value* memberptr = builder.CreatePointerCast(voidmemberptr, pfinaltype);

				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(memberptr);

				jitcontext.ReferencesOnStack.pop();
			}
			break;

		case Bytecode::Instructions::ReadRef:
			{
				Value* derefvalue = builder.CreateLoad(jitcontext.ValuesOnStack.top(), false);
				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(derefvalue);
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				size_t index = Fetch<size_t>(bytecode, offset);
				retval = jitcontext.VariableMap[index];
			}
			break;

		case Bytecode::Instructions::Return:
			builder.CreateBr(jitcontext.FunctionExit);
			break;

		case Bytecode::Instructions::Invoke:
		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = ownervm.JITHelpers.InvokeHelpers.find(target);
				if(iter == ownervm.JITHelpers.InvokeHelpers.end())
					throw FatalException("Cannot invoke this function, no native code support!");

				iter->second(jitcontext);
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

	Value* stackptr = builder.CreateLoad(pstackptr, true);
	Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparams - numreturns));
	Value* stackptr2 = builder.CreateGEP(stackptr, offset);
	if(retval)
		builder.CreateStore(retval, builder.CreatePointerCast(stackptr2, PointerType::get(retval->getType(), 0)), true);
	builder.CreateStore(stackptr2, pstackptr, true);

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

	//module->dump();

	void* fptr = ee->getPointerToFunction(dostufffunc);
	return (JITExecPtr)fptr;
}

