#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"

#include "Utility/Strings.h"

#include <sstream>
#include <map>
#include <stack>

llvm::Module* module = NULL;
llvm::Function* vmgetstructure = NULL;

std::map<std::string, llvm::Function*> FunctionCacheByName;
std::map<StringHandle, llvm::Function*> FunctionCache;

template <typename T>
T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
{
	const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
	InstructionOffset += sizeof(T);
	return static_cast<T>(*data);
}


void JITByteCode(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset, StringHandle alias)
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& context = getGlobalContext();

	if(!module)
		module = new Module("EpochJIT", context);

	IRBuilder<> builder(context);

	PointerType* stackptrtype = PointerType::get(Type::getInt32Ty(context), 0);
	PointerType* pstackptrtype = PointerType::get(stackptrtype, 0);


	std::vector<Type*> args;
	args.push_back(pstackptrtype);
	args.push_back(Type::getInt1PtrTy(context));
	FunctionType* dostufffunctype = FunctionType::get(Type::getVoidTy(context), args, false);

	std::ostringstream name;
	name << "JITFunc_" << beginoffset;

	Function* dostufffunc = Function::Create(dostufffunctype, Function::ExternalLinkage, name.str().c_str(), module);


	std::map<Value*, Value*> structurelookupcache;

	std::vector<Type*> vmargs;
	vmargs.push_back(Type::getInt1PtrTy(context));
	vmargs.push_back(IntegerType::get(context, 32));
	FunctionType* vmgetstructuretype = FunctionType::get(Type::getInt1PtrTy(context), vmargs, false);

	if(!vmgetstructure)
		vmgetstructure = Function::Create(vmgetstructuretype, Function::ExternalLinkage, "VMGetStructure", module);

	BasicBlock* block = BasicBlock::Create(context, "entry", dostufffunc);
	builder.SetInsertPoint(block);

	Value* pstackptr = dostufffunc->arg_begin();
	Value* vmcontextptr = ++(dostufffunc->arg_begin());

	size_t localoffset = 0;
	
	BasicBlock* outerfunctionexit = BasicBlock::Create(context, "exit", dostufffunc);

	JIT::JITContext jitcontext;
	jitcontext.Builder = &builder;
	jitcontext.PStackPtr = pstackptr;
	jitcontext.Context = &context;
	jitcontext.MyModule = module;
	jitcontext.InnerFunction = NULL;
	BasicBlock* innerfunctionexit = NULL;
	
	Value* retval = NULL;
	Value* innerretval = NULL;
	unsigned numparameters = 0;
	unsigned numparamslots = 0;
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
					std::vector<Type*> innerargs;
					innerargs.push_back(vmcontextptr->getType());

					Type* rettype = Type::getVoidTy(context);

					Value* stackptr = builder.CreateLoad(pstackptr, true);
					Type* type = NULL;

					std::set<size_t> locals;

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
							rettype = type;
							jitcontext.VariableMap[i] = builder.CreateAlloca(type);
							// Deliberate fallthrough

						case VARIABLE_ORIGIN_LOCAL:
							locals.insert(i);
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparamslots);
								++numparameters;
								++numparamslots;
								++localoffset;

								if(scope.IsReference(i))
								{
									// TODO - support references for non-structure types
									if(varfamily == Metadata::EpochTypeFamily_Structure || varfamily == Metadata::EpochTypeFamily_TemplateInstance)
									{
										type = Type::getInt32Ty(context);
										Type* ptype = PointerType::get(PointerType::get(type, 0), 0);
										Value* local = builder.CreateAlloca(type);
										jitcontext.VariableMap[i] = local;
										Value* newstackptr = builder.CreateGEP(stackptr, offset);
										Value* stackval = builder.CreateLoad(builder.CreatePointerCast(newstackptr, ptype), false);
										Value* deref = builder.CreateLoad(stackval, false);
										builder.CreateStore(deref, local, false);
										++numparamslots;
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

								innerargs.push_back(type);
							}
							break;
						}

						jitcontext.NameToIndexMap[scope.GetVariableNameHandle(i)] = i;
					}

					std::ostringstream name;
					name << "JITFuncInner_" << beginoffset;

					FunctionType* innerfunctype = FunctionType::get(rettype, innerargs, false);
					if(!FunctionCacheByName[name.str()])
					{
						jitcontext.InnerFunction = Function::Create(innerfunctype, Function::InternalLinkage, name.str().c_str(), module);
						FunctionCacheByName[name.str()] = jitcontext.InnerFunction;
					}
					else
						jitcontext.InnerFunction = FunctionCacheByName[name.str()];

					builder.SetInsertPoint(block);

					if(numparamslots == 4)
					{
						Value* p1 = builder.CreateLoad(jitcontext.VariableMap[0]);
						Value* p2 = builder.CreateLoad(jitcontext.VariableMap[1]);
						Value* r = builder.CreateCall3(jitcontext.InnerFunction, vmcontextptr, p1, p2);
						builder.CreateStore(r, jitcontext.VariableMap[2]);
					}
					else if(numparamslots == 2)
					{						
						Value* p1 = builder.CreateLoad(jitcontext.VariableMap[0]);
						if(numreturns)
						{
							Value* r = builder.CreateCall2(jitcontext.InnerFunction, vmcontextptr, p1);
							builder.CreateStore(r, jitcontext.VariableMap[1]);
						}
						else
							builder.CreateCall2(jitcontext.InnerFunction, vmcontextptr, p1);
					}

					BasicBlock* innerentryblock = BasicBlock::Create(context, "innerentry", jitcontext.InnerFunction);
					builder.SetInsertPoint(innerentryblock);

					innerfunctionexit = BasicBlock::Create(context, "innerexit", jitcontext.InnerFunction);

					innerretval = builder.CreateAlloca(Type::getFloatTy(context));

					if(numparamslots == 4)
					{
						retval = jitcontext.VariableMap[2];
						jitcontext.VariableMap[0] = ++(jitcontext.InnerFunction->arg_begin());
						jitcontext.VariableMap[1] = ++(++(jitcontext.InnerFunction->arg_begin()));
						jitcontext.VariableMap[2] = innerretval;
					}
					else if(numparamslots == 2)
					{
						retval = jitcontext.VariableMap[1];
						jitcontext.VariableMap[0] = ++(jitcontext.InnerFunction->arg_begin());
						jitcontext.VariableMap[1] = innerretval;
					}

					for(std::set<size_t>::const_iterator localiter = locals.begin(); localiter != locals.end(); ++localiter)
					{
						// TODO - improve
						Type* type = Type::getFloatTy(context);
						jitcontext.VariableMap[*localiter] = builder.CreateAlloca(type, NULL, narrow(ownervm.GetPooledString(scope.GetVariableNameHandle(*localiter))));
					}
				}
				else
				{
					ownervm.JITHelpers.EntityHelpers.find(entitytype)->second(jitcontext, true);
				}
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				Bytecode::EntityTag tag = jitcontext.EntityTypes.top();
				jitcontext.EntityTypes.pop();
				if(tag != Bytecode::EntityTags::Function)
				{
					ownervm.JITHelpers.EntityHelpers.find(tag)->second(jitcontext, false);
				}
			}
			break;

		case Bytecode::Instructions::BeginChain:
			jitcontext.EntityChecks.push(BasicBlock::Create(context, "", jitcontext.InnerFunction));
			jitcontext.EntityChains.push(BasicBlock::Create(context, "", jitcontext.InnerFunction));
			jitcontext.EntityChainExits.push(BasicBlock::Create(context, "", jitcontext.InnerFunction));
			builder.CreateBr(jitcontext.EntityChecks.top());
			builder.SetInsertPoint(jitcontext.EntityChecks.top());
			break;

		case Bytecode::Instructions::EndChain:
			if(jitcontext.EntityChains.top()->empty())
			{
				builder.SetInsertPoint(jitcontext.EntityChains.top());
				builder.CreateBr(jitcontext.EntityChainExits.top());
			}

			jitcontext.EntityChecks.pop();
			jitcontext.EntityChains.pop();
			builder.SetInsertPoint(jitcontext.EntityChainExits.top());
			jitcontext.EntityChainExits.pop();
			break;

		case Bytecode::Instructions::Assign:
			{
				Value* reftarget = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				builder.CreateStore(jitcontext.ValuesOnStack.top(), reftarget, true);
			}
			break;

		case Bytecode::Instructions::ReadStack:
			{
				size_t frames = Fetch<size_t>(bytecode, offset);
				size_t stackoffset = Fetch<size_t>(bytecode, offset);
				size_t stacksize = Fetch<size_t>(bytecode, offset);

				if(frames != 0)
					throw NotImplementedException("More lazy.");

				if(stacksize != 4)
					throw NotImplementedException("Ridiculously lazy.");

				// TODO - this is really egregious

				Value* val = builder.CreateLoad(jitcontext.VariableMap[localoffset + stackoffset / 4]);
				jitcontext.ValuesOnStack.push(val);
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
				//size_t varindex = jitcontext.ReferencesOnStack.top();
				
				Metadata::EpochTypeID membertype = Fetch<Metadata::EpochTypeID>(bytecode, offset);
				size_t memberoffset = Fetch<size_t>(bytecode, offset);

				Value* voidstructptr = structurelookupcache[jitcontext.ValuesOnStack.top()];

				if(!voidstructptr)
				{
					Value* structurehandle = jitcontext.ValuesOnStack.top();
					voidstructptr = builder.CreateCall2(vmgetstructure, jitcontext.InnerFunction->arg_begin(), structurehandle);
					structurelookupcache[jitcontext.ValuesOnStack.top()] = voidstructptr;
				}


				Value* bytestructptr = builder.CreatePointerCast(voidstructptr, Type::getInt1PtrTy(context));
				Value* voidmemberptr = builder.CreateGEP(bytestructptr, ConstantInt::get(Type::getInt32Ty(context), memberoffset));

				Value* memberptr = NULL;

				switch(membertype)
				{
				case Metadata::EpochType_Real:
					{
						Type* pfinaltype = Type::getFloatPtrTy(context);
						memberptr = builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					break;

				default:
					if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_TemplateInstance)
					{
						Type* pfinaltype = Type::getInt32PtrTy(context);
						memberptr = builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					else
						throw NotImplementedException("I am lazy.");
				}

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
				innerretval = jitcontext.VariableMap[index];
			}
			break;

		case Bytecode::Instructions::Return:
			builder.CreateBr(innerfunctionexit);
			break;

		case Bytecode::Instructions::Invoke:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = ownervm.JITHelpers.InvokeHelpers.find(target);
				if(iter == ownervm.JITHelpers.InvokeHelpers.end())
					throw FatalException("Cannot invoke this function, no native code support!");

				iter->second(jitcontext, true);
			}
			break;

		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				Fetch<size_t>(bytecode, offset);		// skip dummy offset
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = ownervm.JITHelpers.InvokeHelpers.find(target);
				if(iter != ownervm.JITHelpers.InvokeHelpers.end())
					iter->second(jitcontext, true);
				else
				{
					// TODO - implement properly

					std::vector<Type*> args;
					args.push_back(Type::getInt1PtrTy(context));
					args.push_back(Type::getInt32Ty(context));
					args.push_back(Type::getInt32Ty(context));
					FunctionType* faketype = FunctionType::get(Type::getFloatTy(context), args, false);

					std::ostringstream name;
					name << "JITFuncInner_" << ownervm.GetFunctionInstructionOffsetNoThrow(target);
					Function* targetfunc;
					if(FunctionCacheByName[name.str()])
						targetfunc = FunctionCacheByName[name.str()];
					else
					{
						targetfunc = Function::Create(faketype, Function::InternalLinkage, name.str().c_str(), module);
						FunctionCacheByName[name.str()] = targetfunc;
					}

					Value* p1 = builder.CreateLoad(jitcontext.ValuesOnStack.top());
					jitcontext.ValuesOnStack.pop();
					Value* p2 = builder.CreateLoad(jitcontext.ValuesOnStack.top());
					jitcontext.ValuesOnStack.pop();
					jitcontext.ValuesOnStack.push(builder.CreateCall3(targetfunc, jitcontext.InnerFunction->arg_begin(), p1, p2));
				}
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

				case Metadata::EpochType_Real:
					{
						Real32 value = Fetch<Real32>(bytecode, offset);
						valueval = ConstantFP::get(Type::getFloatTy(context), value);
					}
					break;

				case Metadata::EpochType_Integer16:
				case Metadata::EpochType_Boolean:
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
	
	builder.SetInsertPoint(innerfunctionexit);
	if(innerretval)
	{
		Value* ret = builder.CreateLoad(innerretval);
		builder.CreateRet(ret);
	}
	else
		builder.CreateRetVoid();

	builder.SetInsertPoint(block);
	builder.CreateBr(outerfunctionexit);

	builder.SetInsertPoint(outerfunctionexit);
	Value* stackptr = builder.CreateLoad(pstackptr, true);
	Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparamslots - numreturns));
	Value* stackptr2 = builder.CreateGEP(stackptr, offset);
	if(retval)
	{
		Value* ret = builder.CreateLoad(retval);
		builder.CreateStore(ret, builder.CreatePointerCast(stackptr2, PointerType::get(ret->getType(), 0)), true);
	}
	builder.CreateStore(stackptr2, pstackptr, true);


	builder.CreateRetVoid();

	//verifyFunction(*dostufffunc);

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return;

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

	FunctionCache[alias] = dostufffunc;
}

void PopulateJITExecs(VM::VirtualMachine& ownervm)
{
	using namespace llvm;

	//module->dump();

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return;

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

	for(std::map<StringHandle, Function*>::const_iterator iter = FunctionCache.begin(); iter != FunctionCache.end(); ++iter)
	{
		void* fptr = ee->getPointerToFunction(iter->second);
		ownervm.JITExecs[iter->first] = (JITExecPtr)fptr;
	}
}

