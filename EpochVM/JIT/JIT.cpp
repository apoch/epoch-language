#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/TypeInfo.h"

#include "Utility/Strings.h"

#include <sstream>
#include <map>
#include <stack>

llvm::Module* module = NULL;
llvm::Function* vmgetstructure = NULL;
llvm::Function* vmhaltfunction = NULL;

std::map<std::string, llvm::Function*> FunctionCacheByName;
std::map<StringHandle, llvm::Function*> FunctionCache;
std::map<Metadata::EpochTypeID, llvm::StructType*> TaggedTypeCache;

template <typename T>
T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
{
	const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
	InstructionOffset += sizeof(T);
	return static_cast<T>(*data);
}

llvm::Type* GetJITType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context);

llvm::StructType* GetJITTaggedType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context)
{
	llvm::StructType* taggedtype = TaggedTypeCache[type];
	if(!taggedtype)
	{
		const VariantDefinition& def = ownervm.VariantDefinitions.find(type)->second;
		const std::set<Metadata::EpochTypeID>& types = def.GetBaseTypes();

		llvm::Type* rettype = NULL;
		size_t maxsize = 0;

		for(std::set<Metadata::EpochTypeID>::const_iterator iter = types.begin(); iter != types.end(); ++iter)
		{
			size_t size = Metadata::GetStorageSize(*iter);
			if(size > maxsize)
			{
				maxsize = size;
				rettype = GetJITType(ownervm, *iter, context);
			}
		}

		std::vector<llvm::Type*> elemtypes;
		elemtypes.push_back(llvm::Type::getInt32Ty(context));
		elemtypes.push_back(rettype);
		taggedtype = llvm::StructType::create(elemtypes);
		TaggedTypeCache[type] = taggedtype;
	}
	return taggedtype;
}


llvm::Type* GetJITType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context)
{
	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);
	switch(type)
	{
	case Metadata::EpochType_Integer:
		return llvm::Type::getInt32Ty(context);

	case Metadata::EpochType_Real:
		return llvm::Type::getFloatTy(context);

	default:
		if(family == Metadata::EpochTypeFamily_SumType)
			return GetJITTaggedType(ownervm, type, context);

		if(family == Metadata::EpochTypeFamily_Structure || family == Metadata::EpochTypeFamily_TemplateInstance)
			return llvm::Type::getInt32Ty(context);

		throw NotImplementedException("Unsupported type for native code generation");
	}
}

llvm::FunctionType* GetJITFunctionType(const VM::VirtualMachine& ownervm, StringHandle target, llvm::LLVMContext& context)
{
	using namespace llvm;
	
	Type* rettype = Type::getVoidTy(context);

	std::vector<Type*> args;
	args.push_back(Type::getInt1PtrTy(context));

	const ScopeDescription& scope = ownervm.GetScopeDescription(target);
	for(size_t i = 0; i < scope.GetVariableCount(); ++i)
	{
		if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
		{
			Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);
			Metadata::EpochTypeFamily varfamily = Metadata::GetTypeFamily(vartype);

			if(vartype == Metadata::EpochType_Nothing)
				continue;

			Type* type = GetJITType(ownervm, vartype, context);
			if(scope.IsReference(i) && !(varfamily == Metadata::EpochTypeFamily_Structure || varfamily == Metadata::EpochTypeFamily_TemplateInstance))
				args.push_back(PointerType::get(type, 0));
			else
				args.push_back(type);
		}
		else if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
			rettype = GetJITType(ownervm, scope.GetVariableTypeByIndex(i), context);
	}
	
	return FunctionType::get(rettype, args, false);
}


void JITByteCode(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset, StringHandle alias)
{
	using namespace llvm;

	InitializeNativeTarget();

	LLVMContext& context = getGlobalContext();

	if(!module)
		module = new Module("EpochJIT", context);

	IRBuilder<> builder(context);

	PointerType* stackptrtype = PointerType::get(Type::getInt8Ty(context), 0);
	PointerType* pstackptrtype = PointerType::get(stackptrtype, 0);


	std::vector<Type*> args;
	args.push_back(pstackptrtype);
	args.push_back(Type::getInt1PtrTy(context));
	FunctionType* dostufffunctype = FunctionType::get(Type::getVoidTy(context), args, false);

	std::ostringstream name;
	name << "JITFunc_" << beginoffset;

	Function* dostufffunc = FunctionCacheByName[name.str()];
	if(!dostufffunc)
	{
		dostufffunc = Function::Create(dostufffunctype, Function::ExternalLinkage, name.str().c_str(), module);
		FunctionCacheByName[name.str()] = dostufffunc;
	}


	std::map<Value*, Value*> structurelookupcache;

	std::vector<Type*> vmargs;
	vmargs.push_back(Type::getInt1PtrTy(context));
	vmargs.push_back(IntegerType::get(context, 32));
	FunctionType* vmgetstructuretype = FunctionType::get(Type::getInt1PtrTy(context), vmargs, false);

	if(!vmgetstructure)
		vmgetstructure = Function::Create(vmgetstructuretype, Function::ExternalLinkage, "VMGetStructure", module);

	if(!vmhaltfunction)
		vmhaltfunction = Function::Create(FunctionType::get(Type::getVoidTy(context), false), Function::ExternalLinkage, "VMHalt", module);

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
	unsigned numparambytes = 0;
	unsigned numreturns = 0;
	unsigned typematchindex = 0;

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
					std::set<size_t> parameters;

					size_t retindex = static_cast<size_t>(-1);

					const ScopeDescription& scope = ownervm.GetScopeDescription(entityname);
					curscope = &scope;
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);
						Metadata::EpochTypeFamily varfamily = Metadata::GetTypeFamily(vartype);

						if(vartype == Metadata::EpochType_Nothing)
							continue;

						type = GetJITType(ownervm, vartype, context);

						switch(scope.GetVariableOrigin(i))
						{
						case VARIABLE_ORIGIN_RETURN:
							++numreturns;
							retindex = i;
							rettype = type;
							jitcontext.VariableMap[i] = builder.CreateAlloca(type);
							// Deliberate fallthrough

						case VARIABLE_ORIGIN_LOCAL:
							locals.insert(i);
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								parameters.insert(i);
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparambytes);
								++numparameters;
								++localoffset;

								if(scope.IsReference(i))
								{
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
									}
									else
									{
										Type* ptype = PointerType::get(PointerType::get(type, 0), 0);
										Value* newstackptr = builder.CreateGEP(stackptr, offset);
										Value* ref = builder.CreatePointerCast(newstackptr, ptype);
										jitcontext.VariableMap[i] = ref;
									}
									numparambytes += sizeof(void*) + sizeof(Metadata::EpochTypeID);
								}
								else
								{
									Value* local = builder.CreateAlloca(type);
									jitcontext.VariableMap[i] = local;
									Value* newstackptr = builder.CreateGEP(stackptr, offset);
									Value* stackval = builder.CreateLoad(newstackptr, false);
									builder.CreateStore(stackval, local, false);

									numparambytes += Metadata::GetStorageSize(vartype);
								}

								innerargs.push_back(type);
							}
							break;
						}

						jitcontext.NameToIndexMap[scope.GetVariableNameHandle(i)] = i;
					}

					std::ostringstream name;
					name << "JITFuncInner_" << beginoffset;

					FunctionType* innerfunctype = GetJITFunctionType(ownervm, entityname, context);
					if(!FunctionCacheByName[name.str()])
					{
						jitcontext.InnerFunction = Function::Create(innerfunctype, Function::InternalLinkage, name.str().c_str(), module);
						FunctionCacheByName[name.str()] = jitcontext.InnerFunction;
					}
					else
						jitcontext.InnerFunction = FunctionCacheByName[name.str()];

					builder.SetInsertPoint(block);

					std::vector<Value*> innerparams;
					innerparams.push_back(vmcontextptr);
					for(std::set<size_t>::const_iterator paramiter = parameters.begin(); paramiter != parameters.end(); ++paramiter)
					{
						innerparams.push_back(builder.CreateLoad(jitcontext.VariableMap[*paramiter]));
					}

					if(numreturns)
					{
						Value* r = builder.CreateCall(jitcontext.InnerFunction, innerparams);
						builder.CreateStore(r, jitcontext.VariableMap[retindex]);
					}
					else
						builder.CreateCall(jitcontext.InnerFunction, innerparams);

					BasicBlock* innerentryblock = BasicBlock::Create(context, "innerentry", jitcontext.InnerFunction);
					builder.SetInsertPoint(innerentryblock);

					innerfunctionexit = BasicBlock::Create(context, "innerexit", jitcontext.InnerFunction);

					if(numreturns)
						innerretval = builder.CreateAlloca(Type::getFloatTy(context));

					size_t i = 0;
					retval = numreturns ? jitcontext.VariableMap[retindex] : NULL;
					Function::ArgumentListType& args = jitcontext.InnerFunction->getArgumentList();
					Function::ArgumentListType::iterator argiter = args.begin();
					++argiter;
					for(; argiter != args.end(); ++argiter)
						jitcontext.VariableMap[i++] = ((Argument*)argiter);

					if(numreturns)
						jitcontext.VariableMap[retindex] = innerretval;

					for(std::set<size_t>::const_iterator localiter = locals.begin(); localiter != locals.end(); ++localiter)
					{
						Type* type = GetJITType(ownervm, scope.GetVariableTypeByIndex(*localiter), context);
						jitcontext.VariableMap[*localiter] = builder.CreateAlloca(type, NULL, narrow(ownervm.GetPooledString(scope.GetVariableNameHandle(*localiter))));
					}
				}
				else if(entitytype == Bytecode::EntityTags::TypeResolver)
				{

				}
				else
				{
					std::map<StringHandle, JIT::JITHelper>::const_iterator helperiter = ownervm.JITHelpers.EntityHelpers.find(entitytype);
					if(helperiter == ownervm.JITHelpers.EntityHelpers.end())
						throw FatalException("Cannot JIT this type of entity");
					
					helperiter->second(jitcontext, true);
				}
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				Bytecode::EntityTag tag = jitcontext.EntityTypes.top();
				jitcontext.EntityTypes.pop();
				if(tag != Bytecode::EntityTags::Function && tag != Bytecode::EntityTags::TypeResolver)
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
					else if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_SumType)
					{
						Type* pfinaltype = PointerType::get(GetJITTaggedType(ownervm, membertype, context), 0);
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

		case Bytecode::Instructions::InvokeOffset:
			throw FatalException("Cannot call from native code back into VM code!");

		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(bytecode, offset);
				Fetch<size_t>(bytecode, offset);		// skip dummy offset
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = ownervm.JITHelpers.InvokeHelpers.find(target);
				if(iter != ownervm.JITHelpers.InvokeHelpers.end())
					iter->second(jitcontext, true);
				else
				{
					FunctionType* targetfunctype = GetJITFunctionType(ownervm, target, context);

					std::ostringstream name;
					name << "JITFuncInner_" << ownervm.GetFunctionInstructionOffsetNoThrow(target);
					Function* targetfunc;
					if(FunctionCacheByName[name.str()])
						targetfunc = FunctionCacheByName[name.str()];
					else
					{
						targetfunc = Function::Create(targetfunctype, Function::InternalLinkage, name.str().c_str(), module);
						FunctionCacheByName[name.str()] = targetfunc;
					}

					std::vector<Value*> targetargs;
					targetargs.push_back(jitcontext.InnerFunction->arg_begin());
					for(size_t i = 1; i < targetfunctype->getNumParams(); ++i)
					{
						Value* p = builder.CreateLoad(jitcontext.ValuesOnStack.top());
						jitcontext.ValuesOnStack.pop();
						targetargs.push_back(p);
					}

					jitcontext.ValuesOnStack.push(builder.CreateCall(targetfunc, targetargs));
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

				case Metadata::EpochType_Identifier:
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

		case Bytecode::Instructions::TypeMatch:
			{
				const unsigned FLAG_PROVIDED_REF = 0x01;
				const unsigned FLAG_PROVIDED_NOTHING = 0x02;
				const unsigned FLAG_PROVIDED_NOTHING_REF = 0x04;

				std::ostringstream nextname;
				nextname << "nexttypematcher" << ++typematchindex;
				BasicBlock* nexttypematcher = BasicBlock::Create(context, nextname.str(), dostufffunc);

				Value* providedtype = builder.CreateAlloca(Type::getInt32Ty(context), NULL, "providedtype");
				Value* providedref = builder.CreateAlloca(Type::getInt1Ty(context), NULL, "providedref");
				Value* initialstackptr = builder.CreateLoad(pstackptr, true);
				Value* stackoffsettracker = builder.CreateAlloca(Type::getInt32Ty(context), NULL, "stackoffset");

				builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), 0), stackoffsettracker);

				StringHandle funcname = Fetch<StringHandle>(bytecode, offset);
				Fetch<size_t>(bytecode, offset);
				size_t paramcount = Fetch<size_t>(bytecode, offset);

				std::vector<Value*> allflags;

				for(size_t i = 0; i < paramcount; ++i)
				{
					bool expectref = Fetch<bool>(bytecode, offset);
					Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(bytecode, offset);

					builder.CreateStore(ConstantInt::get(Type::getInt1Ty(context), 0), providedref);

					BasicBlock* checkmatchblock = BasicBlock::Create(context, "checkmatch", dostufffunc);

					Value* flags = builder.CreateAlloca(Type::getInt32Ty(context), NULL, "paramflags");

					if(!expectref)
					{
						Value* lateststackptr = builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker));
						builder.CreateStore(providedtype, builder.CreatePointerCast(lateststackptr, Type::getInt32PtrTy(context)), true);

						// TODO - support undecomposed sum types
						/*
						while(GetTypeFamily(providedtype) == EpochTypeFamily_SumType)
						{
							stackptr += sizeof(EpochTypeID);
							providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);
						}
						*/

						Value* providedrefflag = builder.CreateICmpEQ(providedtype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(context, "providedref", dostufffunc);
						builder.CreateCondBr(providedrefflag, providedrefblock, nexttypematcher);

						builder.SetInsertPoint(providedrefblock);
						builder.CreateStore(ConstantInt::get(Type::getInt1Ty(context), 1), providedref);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* providednothingflag = builder.CreateICmpEQ(providedtype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						Value* expectnothing = (expecttype == Metadata::EpochType_Nothing ? ConstantInt::get(Type::getInt1Ty(context), 1) : ConstantInt::get(Type::getInt1Ty(context), 0));
						Value* provideandexpectnothing = builder.CreateAnd(providednothingflag, expectnothing);

						BasicBlock* handlesomethingblock = BasicBlock::Create(context, "handlesomething", dostufffunc);
						BasicBlock* handlenothingblock = BasicBlock::Create(context, "handlenothing", dostufffunc);

						builder.CreateCondBr(provideandexpectnothing, handlenothingblock, handlesomethingblock);
						builder.SetInsertPoint(handlenothingblock);
						builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing), providedtype);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(void*))), stackoffsettracker);
						builder.CreateBr(checkmatchblock);

						builder.SetInsertPoint(handlesomethingblock);
						BasicBlock* handlerefblock = BasicBlock::Create(context, "handleref", dostufffunc);
						builder.CreateCondBr(providednothingflag, nexttypematcher, handlerefblock);
						builder.SetInsertPoint(handlerefblock);

						Value* reftarget = builder.CreateLoad(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)));
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(void*))), stackoffsettracker);

						Value* reftype = builder.CreateLoad(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)));
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* refisnothingflag = builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						Value* refandexpectnothing = builder.CreateAnd(expectnothing, refisnothingflag);

						BasicBlock* handlenothingrefblock = BasicBlock::Create(context, "handlenothingref", dostufffunc);
						BasicBlock* checksumtypeblock = BasicBlock::Create(context, "checksumtype", dostufffunc);
						builder.CreateCondBr(refandexpectnothing, handlenothingrefblock, checksumtypeblock);

						builder.SetInsertPoint(handlenothingrefblock);
						builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing), providedtype);
						builder.CreateStore(builder.CreateOr(builder.CreateLoad(flags), ConstantInt::get(Type::getInt32Ty(context), FLAG_PROVIDED_NOTHING_REF)), flags);
						builder.CreateBr(checkmatchblock);

						BasicBlock* handlesumtypeblock = BasicBlock::Create(context, "handlesumtype", dostufffunc);

						builder.SetInsertPoint(checksumtypeblock);
						Value* providedtypefamily = builder.CreateAnd(providedtype, 0xff000000);
						Value* issumtype = builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochTypeFamily_SumType));
						builder.CreateCondBr(issumtype, handlesumtypeblock, nexttypematcher);

						builder.SetInsertPoint(handlesumtypeblock);
						Value* reftypeptr = builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
						reftype = builder.CreateLoad(builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(context)));

						BasicBlock* handlereftonothingblock = BasicBlock::Create(context, "handlereftonothing", dostufffunc);
						refisnothingflag = builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						refandexpectnothing = builder.CreateAnd(expectnothing, refisnothingflag);
						builder.CreateCondBr(refandexpectnothing, handlereftonothingblock, nexttypematcher);

						builder.SetInsertPoint(handlereftonothingblock);
						builder.CreateStore(providedtype, reftype);
						builder.CreateBr(checkmatchblock);
					}
					else
					{
						Value* magic = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context)), true);
						Value* providedrefflag = builder.CreateICmpEQ(magic, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(context, "providedexpectedref", dostufffunc);
						builder.CreateCondBr(providedrefflag, providedrefblock, nexttypematcher);
						builder.SetInsertPoint(providedrefblock);

						builder.CreateStore(ConstantInt::get(Type::getInt1Ty(context), 1), providedref);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* reftarget = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), PointerType::get(Type::getInt8PtrTy(context), 0)));
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(void*))), stackoffsettracker);

						Value* reftype = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context)));
						builder.CreateStore(reftype, providedtype);

						BasicBlock* setnothingrefflagblock = BasicBlock::Create(context, "setnothingrefflag", dostufffunc);
						BasicBlock* skipblock = BasicBlock::Create(context, "skip", dostufffunc);

						Value* providednothingflag = builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

						builder.SetInsertPoint(setnothingrefflagblock);
						builder.CreateStore(builder.CreateOr(builder.CreateLoad(flags), ConstantInt::get(Type::getInt32Ty(context), FLAG_PROVIDED_NOTHING_REF)), flags);
						builder.CreateBr(skipblock);

						builder.SetInsertPoint(skipblock);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);


						BasicBlock* handlesumtypeblock = BasicBlock::Create(context, "handlesumtype", dostufffunc);

						Value* providedtypefamily = builder.CreateAnd(builder.CreateLoad(providedtype), 0xff000000);
						Value* issumtype = builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochTypeFamily_SumType));
						builder.CreateCondBr(issumtype, handlesumtypeblock, checkmatchblock);

						builder.SetInsertPoint(handlesumtypeblock);
						Value* reftypeptr = builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
						reftype = builder.CreateLoad(builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(context)));
						builder.CreateStore(reftype, providedtype);
						builder.CreateBr(checkmatchblock);
					}

					builder.SetInsertPoint(checkmatchblock);

					Value* nomatch = builder.CreateICmpNE(builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(context), expecttype));
					Value* notexpectsumtype = ConstantInt::get(Type::getInt1Ty(context), Metadata::GetTypeFamily(expecttype) != Metadata::EpochTypeFamily_SumType);

					BasicBlock* setflagsblock = BasicBlock::Create(context, "setflags", dostufffunc);
					BasicBlock* nextparamblock = BasicBlock::Create(context, "nextparam", dostufffunc);

					Value* bailflag = builder.CreateAnd(nomatch, notexpectsumtype);
					builder.CreateCondBr(bailflag, nexttypematcher, setflagsblock);

					builder.SetInsertPoint(setflagsblock);
					BasicBlock* moreflagsblock = BasicBlock::Create(context, "moreflags", dostufffunc);
					BasicBlock* setrefflagblock = BasicBlock::Create(context, "setrefflag", dostufffunc);
					builder.CreateCondBr(builder.CreateLoad(providedref), setrefflagblock, moreflagsblock);
					builder.SetInsertPoint(setrefflagblock);
					builder.CreateStore(builder.CreateOr(builder.CreateLoad(flags), ConstantInt::get(Type::getInt32Ty(context), FLAG_PROVIDED_REF)), flags);
					builder.CreateBr(moreflagsblock);
					
					builder.SetInsertPoint(moreflagsblock);
					BasicBlock* setnothingflagblock = BasicBlock::Create(context, "setnothingflag", dostufffunc);
					Value* providednothingflag = builder.CreateICmpEQ(builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
					builder.CreateCondBr(providednothingflag, setnothingflagblock, nextparamblock);
					builder.SetInsertPoint(setnothingflagblock);
					builder.CreateStore(builder.CreateOr(builder.CreateLoad(flags), ConstantInt::get(Type::getInt32Ty(context), FLAG_PROVIDED_NOTHING)), flags);
					builder.CreateBr(nextparamblock);

					builder.SetInsertPoint(nextparamblock);

					allflags.push_back(flags);
				}

				BasicBlock* invokeblock = BasicBlock::Create(context, "invokesuccess", dostufffunc);
				builder.CreateBr(invokeblock);

				builder.SetInsertPoint(invokeblock);
				// TODO - call inner func with appropriate arguments
				// TODO - fixup the VM stack as necessary

				(void)(funcname);
				builder.CreateCall(vmhaltfunction);
				builder.CreateBr(outerfunctionexit);

				builder.SetInsertPoint(nexttypematcher);
			}
			break;

		case Bytecode::Instructions::Halt:
			builder.CreateCall(vmhaltfunction);
			builder.CreateBr(outerfunctionexit);
			break;

		default:
			throw FatalException("Unsupported instruction for JIT compilation");
		}
	}
	
	if(jitcontext.InnerFunction)
	{
		builder.SetInsertPoint(innerfunctionexit);
		if(innerretval)
		{
			Value* ret = builder.CreateLoad(innerretval);
			builder.CreateRet(ret);
		}
		else
			builder.CreateRetVoid();
	}

	builder.SetInsertPoint(block);
	if(!typematchindex)
		builder.CreateBr(outerfunctionexit);

	builder.SetInsertPoint(outerfunctionexit);
	Value* stackptr = builder.CreateLoad(pstackptr, true);
	Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparambytes - (numreturns * 4)));	// TODO - change to retbytes
	Value* stackptr2 = builder.CreateGEP(stackptr, offset);
	if(retval)
	{
		Value* ret = builder.CreateLoad(retval);
		builder.CreateStore(ret, builder.CreatePointerCast(stackptr2, PointerType::get(ret->getType(), 0)), true);
	}
	builder.CreateStore(stackptr2, pstackptr, true);


	builder.CreateRetVoid();

	verifyFunction(*dostufffunc);

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return;

	FunctionCache[alias] = dostufffunc;
}

void PopulateJITExecs(VM::VirtualMachine& ownervm)
{
	using namespace llvm;

	module->dump();

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

	for(std::map<StringHandle, Function*>::const_iterator iter = FunctionCache.begin(); iter != FunctionCache.end(); ++iter)
	{
		OurFPM.run(*iter->second);
	}

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
	OurMPM.add(createFunctionInliningPass());
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
	OurMPM.add(createFunctionInliningPass());

	OurMPM.run(*module);

	//module->dump();

	for(std::map<StringHandle, Function*>::const_iterator iter = FunctionCache.begin(); iter != FunctionCache.end(); ++iter)
	{
		void* fptr = ee->getPointerToFunction(iter->second);
		ownervm.JITExecs[iter->first] = (JITExecPtr)fptr;
	}
}

