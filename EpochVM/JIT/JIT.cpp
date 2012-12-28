#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/TypeInfo.h"

#include "Utility/Strings.h"

#include <llvm/Intrinsics.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Transforms/Vectorize.h>

#pragma warning(push)
#pragma warning(disable: 4146)
#include <llvm/ADT/Statistic.h>
#pragma warning(pop)

#include <sstream>
#include <map>
#include <stack>
#include <iostream>

llvm::Module* module = NULL;
llvm::Function* vmgetbuffer = NULL;
llvm::Function* vmallocstruct = NULL;
llvm::Function* vmcopystruct = NULL;
llvm::Function* vmhaltfunction = NULL;
llvm::Function* vmretfunction = NULL;
llvm::Function* vmbreak = NULL;

std::map<std::string, llvm::Function*> FunctionCacheByName;
std::map<std::string, llvm::Function*> NativeTypeMatcherCacheByName;
std::map<StringHandle, llvm::Function*> FunctionCache;
std::map<Metadata::EpochTypeID, llvm::StructType*> TaggedTypeCache;

template <typename T>
T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
{
	const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
	InstructionOffset += sizeof(T);
	return static_cast<T>(*data);
}

llvm::Type* GetJITType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context, bool flatten = false);

llvm::Type* GetJITTaggedType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context, bool flatten)
{
	llvm::StructType* taggedtype = TaggedTypeCache[type];
	if(!taggedtype && !flatten)
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
				rettype = GetJITType(ownervm, *iter, context, true);
			}
		}

		if(flatten)
			return rettype;

		std::ostringstream name;
		name << "SumTypeTag_" << type;

		std::vector<llvm::Type*> elemtypes;
		elemtypes.push_back(llvm::Type::getInt32Ty(context));
		elemtypes.push_back(rettype);
		taggedtype = llvm::StructType::create(elemtypes, name.str());
		TaggedTypeCache[type] = taggedtype;
	}
	return taggedtype;
}


llvm::Type* GetJITType(const VM::VirtualMachine& ownervm, Metadata::EpochTypeID type, llvm::LLVMContext& context, bool flatten)
{
	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);
	switch(type)
	{
	case Metadata::EpochType_Integer:
	case Metadata::EpochType_String:
	case Metadata::EpochType_Function:
	case Metadata::EpochType_Buffer:
		return llvm::Type::getInt32Ty(context);

	case Metadata::EpochType_Real:
		return llvm::Type::getFloatTy(context);

	case Metadata::EpochType_Boolean:
		return llvm::Type::getInt8Ty(context);

	case Metadata::EpochType_Integer16:
		return llvm::Type::getInt16Ty(context);

	case Metadata::EpochType_Identifier:
		// STUPID LAME HACK - we only use Identifier params for constructors, so assume this is an aggregate constructor param
		return llvm::Type::getInt8PtrTy(context);

	default:
		if(family == Metadata::EpochTypeFamily_SumType)
			return GetJITTaggedType(ownervm, type, context, flatten);

		if(family == Metadata::EpochTypeFamily_Structure || family == Metadata::EpochTypeFamily_TemplateInstance)
			return llvm::Type::getInt8PtrTy(context);

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

			if(vartype == Metadata::EpochType_Nothing)
			{
				args.push_back(Type::getInt32Ty(context));
				continue;
			}

			Type* type = GetJITType(ownervm, vartype, context);
			if(scope.IsReference(i))
				args.push_back(PointerType::get(type, 0));
			else
				args.push_back(type);
		}
		else if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
			rettype = GetJITType(ownervm, scope.GetVariableTypeByIndex(i), context);
	}
	
	return FunctionType::get(rettype, args, false);
}

void JITBreakpoint(llvm::IRBuilder<>& builder)
{
	using namespace llvm;

	if(!vmbreak)
	{
		FunctionType* fty = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);
		vmbreak = Function::Create(fty, Function::ExternalLinkage, "VMBreak", builder.GetInsertBlock()->getParent()->getParent());
	}

	builder.CreateCall(vmbreak);
}

void JITNativeTypeMatcher(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset, llvm::IRBuilder<>& builder)
{
	using namespace llvm;

	LLVMContext& context = getGlobalContext();

	Function* matcherfunction = builder.GetInsertBlock()->getParent();

	Value* vmcontextptr = matcherfunction->arg_begin();

	std::vector<Value*> reftypes;
	std::vector<Value*> reftargets;

	StringHandle entityname = 0;

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = bytecode[offset++];
		switch(instruction)
		{
		case Bytecode::Instructions::BeginEntity:
			Fetch<Integer32>(bytecode, offset);
			entityname = Fetch<StringHandle>(bytecode, offset);
			break;

		case Bytecode::Instructions::EndEntity:
			break;

		case Bytecode::Instructions::Halt:
			builder.CreateCall(vmhaltfunction);
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				StringHandle funcname = Fetch<StringHandle>(bytecode, offset);
				size_t matchoffset = Fetch<size_t>(bytecode, offset);
				size_t paramcount = Fetch<size_t>(bytecode, offset);

				std::vector<Value*> actualparams;
				BasicBlock* nexttypematcher = BasicBlock::Create(context, "nexttypematcher", matcherfunction);

				Function::arg_iterator argiter = matcherfunction->arg_begin();

				if(reftypes.empty())
				{
					for(size_t i = 0; i < paramcount; ++i)
					{
						++argiter;
						Value* reftype = argiter;
						++argiter;
						Value* reftarget = argiter;

						reftypes.push_back(reftype);
						reftargets.push_back(reftarget);
					}
				}

				for(size_t i = 0; i < paramcount; ++i)
				{
					bool expectref = Fetch<bool>(bytecode, offset);
					Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(bytecode, offset);

					BasicBlock* checkmatchblock = BasicBlock::Create(context, "checkmatch", matcherfunction);
					Value* providedtypeholder = builder.CreateAlloca(Type::getInt32Ty(context));
					Value* parampayloadptr = builder.CreateAlloca(Type::getInt8PtrTy(context), NULL, "parampayloadptr");

					builder.CreateStore(reftypes[i], providedtypeholder);

					BasicBlock* setnothingrefflagblock = BasicBlock::Create(context, "setnothingrefflag", matcherfunction);
					BasicBlock* skipblock = BasicBlock::Create(context, "skip", matcherfunction);

					Value* providednothingflag = builder.CreateICmpEQ(reftypes[i], ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
					builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

					builder.SetInsertPoint(setnothingrefflagblock);
					builder.CreateBr(skipblock);

					builder.SetInsertPoint(skipblock);
					builder.CreateStore(reftargets[i], parampayloadptr);


					BasicBlock* handlesumtypeblock = BasicBlock::Create(context, "handlesumtype", matcherfunction);

					Value* providedtypefamily = builder.CreateAnd(builder.CreateLoad(providedtypeholder), 0xff000000);
					Value* issumtype = builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochTypeFamily_SumType));
					builder.CreateCondBr(issumtype, handlesumtypeblock, checkmatchblock);

					builder.SetInsertPoint(handlesumtypeblock);

					Value* rt = builder.CreateLoad(builder.CreatePointerCast(reftargets[i], Type::getInt32PtrTy(context)));
					builder.CreateStore(rt, providedtypeholder);
					Value* gep = builder.CreateGEP(reftargets[i], ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID)));
					Value* castgep = builder.CreatePointerCast(gep, Type::getInt8PtrTy(context));
					builder.CreateStore(castgep, parampayloadptr);
					builder.CreateBr(checkmatchblock);

					builder.SetInsertPoint(checkmatchblock);

					Value* nomatch = builder.CreateICmpNE(builder.CreateLoad(providedtypeholder), ConstantInt::get(Type::getInt32Ty(context), expecttype));
					Value* notexpectsumtype = ConstantInt::get(Type::getInt1Ty(context), Metadata::GetTypeFamily(expecttype) != Metadata::EpochTypeFamily_SumType);

					BasicBlock* nextparamblock = BasicBlock::Create(context, "nextparam", matcherfunction);

					Value* bailflag = builder.CreateAnd(nomatch, notexpectsumtype);
					builder.CreateCondBr(bailflag, nexttypematcher, nextparamblock);

					builder.SetInsertPoint(nextparamblock);

					if(expecttype != Metadata::EpochType_Nothing)
					{
						Value* v = builder.CreatePointerCast(parampayloadptr, GetJITType(ownervm, expecttype, context)->getPointerTo());

						if(Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_TemplateInstance)
						{
							v = builder.CreatePointerCast(parampayloadptr, GetJITType(ownervm, expecttype, context)->getPointerTo()->getPointerTo());
							v = builder.CreateLoad(v);
						}
						else if(!expectref)
						{
							v = builder.CreatePointerCast(parampayloadptr, GetJITType(ownervm, expecttype, context)->getPointerTo()->getPointerTo());
							v = builder.CreateLoad(v);
							v = builder.CreateLoad(v);
						}

						actualparams.push_back(v);
					}
					else
					{
						actualparams.push_back(ConstantInt::get(Type::getInt32Ty(context), 0xbaadf00d));
					}
				}

				std::ostringstream matchname;
				matchname << "JITFuncInner_" << matchoffset;

				Function* targetinnerfunc = NULL;
				FunctionType* targetinnerfunctype = GetJITFunctionType(ownervm, funcname, context);
				if(!FunctionCacheByName[matchname.str()])
				{
					targetinnerfunc = Function::Create(targetinnerfunctype, Function::InternalLinkage, matchname.str().c_str(), module);
					FunctionCacheByName[matchname.str()] = targetinnerfunc;
				}
				else
					targetinnerfunc = FunctionCacheByName[matchname.str()];

				std::vector<Value*> resolvedargs;
				for(std::vector<Value*>::const_iterator argiter = actualparams.begin(); argiter != actualparams.end(); ++argiter)
					resolvedargs.push_back(*argiter);
				resolvedargs.push_back(vmcontextptr);
				std::reverse(resolvedargs.begin(), resolvedargs.end());

				if(targetinnerfunc->getReturnType() != Type::getVoidTy(context))
				{
					CallInst* typematchret = builder.CreateCall(targetinnerfunc, resolvedargs);
					builder.CreateRet(typematchret);
				}
				else
				{
					builder.CreateCall(targetinnerfunc, resolvedargs);
					builder.CreateRetVoid();
				}

				builder.SetInsertPoint(nexttypematcher);
			}
			break;

		default:
			throw FatalException("Invalid instruction in type matcher");
		}
	}

	builder.CreateCall(vmhaltfunction);
	if(ownervm.TypeMatcherRetType.find(entityname)->second)
	{
		Value* typematchret = builder.CreateAlloca(GetJITType(ownervm, ownervm.TypeMatcherRetType.find(entityname)->second, context));
		builder.CreateRet(builder.CreateLoad(typematchret));
	}
	else
		builder.CreateRetVoid();
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

	if(!vmgetbuffer)
	{
		std::vector<Type*> vmargs;
		vmargs.push_back(Type::getInt1PtrTy(context));
		vmargs.push_back(IntegerType::get(context, 32));
		FunctionType* vmgetbuffertype = FunctionType::get(Type::getInt1PtrTy(context), vmargs, false);

		vmgetbuffer = Function::Create(vmgetbuffertype, Function::ExternalLinkage, "VMGetBuffer", module);
		vmgetbuffer->addAttribute(static_cast<unsigned>(~0), Attribute::ReadNone);
	}

	if(!vmhaltfunction)
		vmhaltfunction = Function::Create(FunctionType::get(Type::getVoidTy(context), false), Function::ExternalLinkage, "VMHalt", module);

	if(!vmretfunction)
	{
		std::vector<Type*> vmretargs;
		vmretargs.push_back(Type::getInt1PtrTy(context));
		vmretfunction = Function::Create(FunctionType::get(Type::getVoidTy(context), vmretargs, false), Function::ExternalLinkage, "VMRet", module);
	}

	if(!vmallocstruct)
	{
		std::vector<Type*> vmargs;
		vmargs.push_back(Type::getInt1PtrTy(context));
		vmargs.push_back(IntegerType::get(context, 32));
		FunctionType* vmallocstructtype = FunctionType::get(Type::getInt8PtrTy(context), vmargs, false);

		vmallocstruct = Function::Create(vmallocstructtype, Function::ExternalLinkage, "VMAllocStruct", module);
	}

	if(!vmcopystruct)
	{
		std::vector<Type*> vmargs;
		vmargs.push_back(Type::getInt1PtrTy(context));
		vmargs.push_back(Type::getInt8PtrTy(context));
		FunctionType* vmcopystructtype = FunctionType::get(Type::getInt8PtrTy(context), vmargs, false);

		vmcopystruct = Function::Create(vmcopystructtype, Function::ExternalLinkage, "VMCopyStruct", module);
	}

	Function* sqrtintrinsic = module->getFunction("llvm.sqrt.f32");
	if(!sqrtintrinsic)
	{
		std::vector<Type*> argtypes;
		argtypes.push_back(Type::getFloatTy(context));
		FunctionType* functype = FunctionType::get(Type::getFloatTy(context), argtypes, false);
		sqrtintrinsic = Function::Create(functype, Function::ExternalLinkage, "llvm.sqrt.f32", module);
	}


	BasicBlock* block = BasicBlock::Create(context, "entry", dostufffunc);
	builder.SetInsertPoint(block);

	Value* pstackptr = builder.CreateAlloca(pstackptrtype);
	builder.CreateStore(dostufffunc->arg_begin(), pstackptr);
	Value* vmcontextptr = ++(dostufffunc->arg_begin());

	size_t localoffset = 0;
	
	BasicBlock* outerfunctionexit = BasicBlock::Create(context, "exit", dostufffunc);

	JIT::JITContext jitcontext;
	jitcontext.Builder = &builder;
	jitcontext.VMContextPtr = NULL;
	jitcontext.Context = &context;
	jitcontext.MyModule = module;
	jitcontext.InnerFunction = NULL;
	jitcontext.VMGetBuffer = vmgetbuffer;
	jitcontext.SqrtIntrinsic = sqrtintrinsic;
	BasicBlock* innerfunctionexit = NULL;
	BasicBlock* nativematchblock = NULL;
	
	Value* retval = NULL;
	Value* innerretval = NULL;
	Value* typematchret = NULL;
	unsigned numparameters = 0;
	unsigned numparambytes = 0;
	unsigned numreturns = 0;
	unsigned typematchindex = 0;

	std::stack<Metadata::EpochTypeID> annotations;
	std::map<size_t, size_t> offsettoindexmap;
	std::map<size_t, size_t> paramoffsettoindexmap;
	std::map<std::pair<Value*, size_t>, Value*> structuremembercache;
	std::map<Value*, Value*> structurerefcache;

	const ScopeDescription* curscope = NULL;

	Metadata::EpochTypeID hackstructtype = 0;

	Value* typematchconsumedbytes = NULL;

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
					Type* rettype = Type::getVoidTy(context);

					Value* stackptr = builder.CreateLoad(builder.CreateLoad(pstackptr));
					Type* type = NULL;

					std::set<size_t> locals;
					std::set<size_t> parameters;

					size_t localoffsetbytes = 0;

					size_t retindex = static_cast<size_t>(-1);

					const ScopeDescription& scope = ownervm.GetScopeDescription(entityname);
					curscope = &scope;
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);
						//Metadata::EpochTypeFamily varfamily = Metadata::GetTypeFamily(vartype);

						if(vartype == Metadata::EpochType_Nothing)
						{
							if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
							{
								Value* zero = builder.CreateAlloca(Type::getInt32Ty(context));
								builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), 0xfeedface), zero);
								parameters.insert(i);
								++numparameters;
								jitcontext.VariableMap[i] = zero;
							}
							continue;
						}

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
								paramoffsettoindexmap[numparambytes] = i;
								parameters.insert(i);
								Constant* offset = ConstantInt::get(Type::getInt32Ty(context), numparambytes);
								++numparameters;
								++localoffset;

								if(scope.IsReference(i))
								{
									Type* ptype = PointerType::get(PointerType::get(type, 0), 0);
									Value* newstackptr = builder.CreateGEP(stackptr, offset);
									Value* ref = builder.CreatePointerCast(newstackptr, ptype);
									jitcontext.VariableMap[i] = ref;//builder.CreatePointerCast(builder.CreateLoad(ref), ptype);
									numparambytes += sizeof(void*) + sizeof(Metadata::EpochTypeID);
								}
								else
								{
									Value* local = builder.CreateAlloca(type);
									jitcontext.VariableMap[i] = local;
									Value* newstackptr = builder.CreateGEP(stackptr, offset);
									Value* stackval = builder.CreateLoad(builder.CreatePointerCast(newstackptr, type->getPointerTo()));
									builder.CreateStore(stackval, local);
									numparambytes += Metadata::GetStorageSize(vartype);
								}
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
						Value* p = builder.CreateLoad(jitcontext.VariableMap[*paramiter]);
						innerparams.push_back(p);
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
						innerretval = builder.CreateAlloca(rettype);

					size_t i = 0;
					retval = numreturns ? jitcontext.VariableMap[retindex] : NULL;
					Function::ArgumentListType& args = jitcontext.InnerFunction->getArgumentList();
					Function::ArgumentListType::iterator argiter = args.begin();
					++argiter;
					for(; argiter != args.end(); ++argiter)
						jitcontext.VariableMap[i++] = ((Argument*)argiter);

					if(numreturns)
						jitcontext.VariableMap[retindex] = innerretval;

					localoffsetbytes = 0;
					for(std::set<size_t>::const_iterator localiter = locals.begin(); localiter != locals.end(); ++localiter)
					{
						Metadata::EpochTypeID localtype = scope.GetVariableTypeByIndex(*localiter);
						Type* type = GetJITType(ownervm, localtype, context);

						if(*localiter != retindex)
							jitcontext.VariableMap[*localiter] = builder.CreateAlloca(type, NULL, narrow(ownervm.GetPooledString(scope.GetVariableNameHandle(*localiter))));

						offsettoindexmap[localoffsetbytes] = *localiter;

						if(Metadata::GetTypeFamily(localtype) == Metadata::EpochTypeFamily_SumType)
							localoffsetbytes += ownervm.VariantDefinitions.find(localtype)->second.GetMaxSize();
						else
							localoffsetbytes += Metadata::GetStorageSize(localtype);
					}

					jitcontext.VMContextPtr = jitcontext.InnerFunction->arg_begin();
				}
				else if(entitytype == Bytecode::EntityTags::TypeResolver)
				{
					std::ostringstream matchername;
					matchername << "JITMatcher_" << beginoffset;

					Function* nativetypematcher = NativeTypeMatcherCacheByName[matchername.str()];
					if(!nativetypematcher)
					{
						std::vector<Type*> matchargtypes;
						matchargtypes.push_back(vmcontextptr->getType());
						for(size_t i = 0; i < ownervm.TypeMatcherParamCount.find(entityname)->second; ++i)
						{
							matchargtypes.push_back(Type::getInt32Ty(context));		// type annotation
							matchargtypes.push_back(Type::getInt8PtrTy(context));	// pointer to payload
						}

						Type* retty = Type::getVoidTy(context);
						if(ownervm.TypeMatcherRetType.find(entityname)->second != Metadata::EpochType_Error)
							retty = GetJITType(ownervm, ownervm.TypeMatcherRetType.find(entityname)->second, context);
						FunctionType* matchfunctype = FunctionType::get(retty, matchargtypes, false);

						nativetypematcher = Function::Create(matchfunctype, Function::ExternalLinkage, matchername.str().c_str(), module);
						NativeTypeMatcherCacheByName[matchername.str()] = nativetypematcher;
					}

					nativematchblock = BasicBlock::Create(context, "nativematchentry", nativetypematcher);
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
				Value* v = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				builder.CreateStore(v, reftarget);
			}
			break;

		case Bytecode::Instructions::AssignSumType:
			{
				Value* reftarget = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Value* actualtype = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* typeholder = builder.CreatePointerCast(reftarget, Type::getInt32PtrTy(context));
				builder.CreateStore(actualtype, typeholder);

				Value* target = builder.CreateGEP(builder.CreatePointerCast(reftarget, Type::getInt8PtrTy(context)), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID)));
				Value* casttarget = builder.CreatePointerCast(target, jitcontext.ValuesOnStack.top()->getType()->getPointerTo());
				builder.CreateStore(jitcontext.ValuesOnStack.top(), casttarget);
			}
			break;

		case Bytecode::Instructions::Read:
			{
				StringHandle varname = Fetch<StringHandle>(bytecode, offset);
				size_t index = jitcontext.NameToIndexMap[varname];

				Value* v = NULL;

				if(index >= numparameters)
					v = builder.CreateLoad(jitcontext.VariableMap[index]);
				else
					v = jitcontext.VariableMap[index];

				if(curscope->IsReferenceByID(varname))
					v = builder.CreateLoad(v);

				jitcontext.ValuesOnStack.push(v);
			}
			break;

		case Bytecode::Instructions::ReadStack:
			{
				size_t frames = Fetch<size_t>(bytecode, offset);
				size_t stackoffset = Fetch<size_t>(bytecode, offset);
				Fetch<size_t>(bytecode, offset);		// stack size is irrelevant

				if(frames != 0)
					throw NotImplementedException("Scope is not flat!");

				size_t index = offsettoindexmap[stackoffset];
				Value* val = builder.CreateLoad(jitcontext.VariableMap[index]);
				jitcontext.ValuesOnStack.push(val);
			}
			break;

		case Bytecode::Instructions::ReadParam:
			{
				size_t frames = Fetch<size_t>(bytecode, offset);
				size_t stackoffset = Fetch<size_t>(bytecode, offset);
				Fetch<size_t>(bytecode, offset);		// stack size is irrelevant

				if(frames != 0)
					throw NotImplementedException("Scope is not flat!");
				
				size_t idx = paramoffsettoindexmap[stackoffset];
				Value* val = jitcontext.VariableMap[idx];
				if(curscope->IsReference(idx))
					val = builder.CreateLoad(val);
				jitcontext.ValuesOnStack.push(val);
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				size_t frames = Fetch<size_t>(bytecode, offset);
				size_t index = Fetch<size_t>(bytecode, offset);

				if(frames > 0)
					throw NotImplementedException("Scope is not flat!");

				Metadata::EpochTypeID vartype = curscope->GetVariableTypeByIndex(index);
				Value* ptr = jitcontext.VariableMap[index];
				jitcontext.ValuesOnStack.push(ptr);
				annotations.push(vartype);
			}
			break;

		case Bytecode::Instructions::BindMemberRef:
			{
				Metadata::EpochTypeID membertype = Fetch<Metadata::EpochTypeID>(bytecode, offset);
				size_t memberoffset = Fetch<size_t>(bytecode, offset);

				Value* voidstructptr = jitcontext.ValuesOnStack.top();
				if(voidstructptr->getType() == Type::getInt8PtrTy(context)->getPointerTo())
					voidstructptr = builder.CreateLoad(voidstructptr);
				Value* bytestructptr = builder.CreatePointerCast(voidstructptr, Type::getInt8PtrTy(context));
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
						Type* pfinaltype = Type::getInt8PtrTy(context)->getPointerTo();
						memberptr = builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					else if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_SumType)
					{
						Type* pfinaltype = PointerType::get(GetJITTaggedType(ownervm, membertype, context, false), 0);
						memberptr = builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					else
						throw NotImplementedException("I am lazy.");
				}

				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(memberptr);
				annotations.push(membertype);
			}
			break;

		case Bytecode::Instructions::ReadRef:
			{
				Value* derefvalue = builder.CreateLoad(jitcontext.ValuesOnStack.top());
				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(derefvalue);
				annotations.pop();
			}
			break;

		case Bytecode::Instructions::ReadRefAnnotated:
			{
				std::vector<Value*> gepindices;
				gepindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
				gepindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
				Value* annotationgep = builder.CreateGEP(jitcontext.ValuesOnStack.top(), gepindices);

				std::vector<Value*> derefindices;
				derefindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
				derefindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 1));
				Value* derefgep = builder.CreateLoad(builder.CreateGEP(jitcontext.ValuesOnStack.top(), derefindices));

				Value* annotationvalue = builder.CreateLoad(annotationgep);
				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(derefgep);
				jitcontext.ValuesOnStack.push(annotationvalue);
				annotations.pop();
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				size_t index = Fetch<size_t>(bytecode, offset);
				builder.CreateStore(builder.CreateLoad(jitcontext.VariableMap[index]), innerretval);
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
			{
				StringHandle functionname = Fetch<StringHandle>(bytecode, offset);
				size_t internaloffset = Fetch<size_t>(bytecode, offset);

				if(ownervm.TypeMatcherParamCount.find(functionname) == ownervm.TypeMatcherParamCount.end())
					throw FatalException("Cannot invoke VM code from native code");

				std::ostringstream matchername;
				matchername << "JITMatcher_" << internaloffset;

				std::vector<Type*> matchargtypes;
				matchargtypes.push_back(vmcontextptr->getType());
				for(size_t i = 0; i < ownervm.TypeMatcherParamCount.find(functionname)->second; ++i)
				{
					matchargtypes.push_back(Type::getInt32Ty(context));		// type annotation
					matchargtypes.push_back(Type::getInt8PtrTy(context));	// pointer to payload
				}


				Type* retty = Type::getVoidTy(context);
				if(ownervm.TypeMatcherRetType.find(functionname)->second != Metadata::EpochType_Error)
					retty = GetJITType(ownervm, ownervm.TypeMatcherRetType.find(functionname)->second, context);
				FunctionType* matchfunctype = FunctionType::get(retty, matchargtypes, false);

				Function* nativetypematcher = NativeTypeMatcherCacheByName[matchername.str()];
				if(!nativetypematcher)
				{
					nativetypematcher = Function::Create(matchfunctype, Function::ExternalLinkage, matchername.str().c_str(), module);
					NativeTypeMatcherCacheByName[matchername.str()] = nativetypematcher;
				}

				if(ownervm.TypeMatcherParamCount.find(functionname) == ownervm.TypeMatcherParamCount.end())
					throw FatalException("Unsure of how many parameters this type matcher accepts!");

				std::vector<Value*> matchervarargs;
				size_t numparams = ownervm.TypeMatcherParamCount.find(functionname)->second;
				matchervarargs.push_back(jitcontext.InnerFunction->arg_begin());
				for(size_t i = 0; i < numparams; ++i)
				{
					Value* v1 = jitcontext.ValuesOnStack.top();
					jitcontext.ValuesOnStack.pop();

					Value* v2 = jitcontext.ValuesOnStack.top();
					jitcontext.ValuesOnStack.pop();

					if(v2->getType()->isPointerTy())
					{
						Metadata::EpochTypeID paramepochtype = annotations.top();
						Value* annotation = ConstantInt::get(Type::getInt32Ty(context), paramepochtype);
						annotations.pop();

						matchervarargs.push_back(annotation);
						matchervarargs.push_back(builder.CreatePointerCast(v2, Type::getInt8PtrTy(context)));
					}
					else
					{
						Value* stacktemp = builder.CreateAlloca(v2->getType());
						builder.CreateStore(v2, stacktemp);

						matchervarargs.push_back(v1);
						matchervarargs.push_back(builder.CreatePointerCast(stacktemp, Type::getInt8PtrTy(context)));
					}
				}

				jitcontext.ValuesOnStack.push(builder.CreateCall(nativetypematcher, matchervarargs));
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
					for(size_t i = 1; i < targetfunctype->getNumParams(); ++i)
					{
						Value* p = jitcontext.ValuesOnStack.top();
						jitcontext.ValuesOnStack.pop();

						//if(p->getType() == targetfunctype->getContainedType(targetfunctype->getNumParams() - i + 1)->getPointerTo())
						//	p = builder.CreateLoad(p);

						targetargs.push_back(p);
					}
					targetargs.push_back(jitcontext.InnerFunction->arg_begin());
					std::reverse(targetargs.begin(), targetargs.end());

					if(targetfunc->getReturnType() != Type::getVoidTy(context))
					{
						Value* rv = builder.CreateAlloca(targetfunc->getReturnType());
						builder.CreateStore(builder.CreateCall(targetfunc, targetargs), rv);
						jitcontext.ValuesOnStack.push(builder.CreateLoad(rv));
					}
					else
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
				std::ostringstream nextname;
				nextname << "nexttypematcher" << ++typematchindex;
				BasicBlock* nexttypematcher = BasicBlock::Create(context, nextname.str(), dostufffunc);

				Value* providedtype = builder.CreateAlloca(Type::getInt32Ty(context), NULL, "providedtype");
				Value* initialstackptr = builder.CreateLoad(builder.CreateLoad(pstackptr));
				Value* stackoffsettracker = builder.CreateAlloca(Type::getInt32Ty(context), NULL, "stackoffset");

				typematchconsumedbytes = builder.CreateAlloca(Type::getInt32Ty(context)), NULL, "consumedbytes";
				builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), 0), stackoffsettracker);

				StringHandle funcname = Fetch<StringHandle>(bytecode, offset);
				size_t matchoffset = Fetch<size_t>(bytecode, offset);
				size_t paramcount = Fetch<size_t>(bytecode, offset);

				std::vector<Value*> actualparams;

				for(size_t i = 0; i < paramcount; ++i)
				{
					bool expectref = Fetch<bool>(bytecode, offset);
					Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(bytecode, offset);

					BasicBlock* checkmatchblock = BasicBlock::Create(context, "checkmatch", dostufffunc);

					Value* parampayloadptr = builder.CreateAlloca(Type::getInt8PtrTy(context), NULL, "parampayloadptr");

					if(!expectref)
					{
						Value* lateststackptr = builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker));
						Value* gettype = builder.CreateLoad(builder.CreatePointerCast(lateststackptr, Type::getInt32PtrTy(context)));
						builder.CreateStore(gettype, providedtype);

						// TODO - support undecomposed sum types
						/*
						while(GetTypeFamily(providedtype) == EpochTypeFamily_SumType)
						{
							stackptr += sizeof(EpochTypeID);
							providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);
						}
						*/

						Value* providedrefflag = builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(context, "providedref", dostufffunc);
						BasicBlock* providedvalueblock = BasicBlock::Create(context, "providedvalue", dostufffunc);
						builder.CreateCondBr(providedrefflag, providedrefblock, providedvalueblock);

						builder.SetInsertPoint(providedrefblock);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* providednothingflag = builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
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

						Value* reftarget = builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker));
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(void*))), stackoffsettracker);

						Value* reftype = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context)));
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* refisnothingflag = builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						Value* refandexpectnothing = builder.CreateAnd(expectnothing, refisnothingflag);

						BasicBlock* handlenothingrefblock = BasicBlock::Create(context, "handlenothingref", dostufffunc);
						BasicBlock* checksumtypeblock = BasicBlock::Create(context, "checksumtype", dostufffunc);
						builder.CreateCondBr(refandexpectnothing, handlenothingrefblock, checksumtypeblock);

						builder.SetInsertPoint(handlenothingrefblock);
						builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing), providedtype);
						builder.CreateBr(checkmatchblock);

						BasicBlock* handlesumtypeblock = BasicBlock::Create(context, "handlesumtype", dostufffunc);

						builder.SetInsertPoint(checksumtypeblock);
						Value* providedtypefamily = builder.CreateAnd(gettype, 0xff000000);
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
						builder.CreateStore(reftype, providedtype);
						builder.CreateBr(checkmatchblock);

						builder.SetInsertPoint(providedvalueblock);
						builder.CreateStore(builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context))), providedtype);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);
						builder.CreateStore(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), parampayloadptr);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), Metadata::GetStorageSize(expecttype))), stackoffsettracker);
						builder.CreateBr(checkmatchblock);
					}
					else
					{
						Value* magic = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context)), true);
						Value* providedrefflag = builder.CreateICmpEQ(magic, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(context, "providedexpectedref", dostufffunc);
						builder.CreateCondBr(providedrefflag, providedrefblock, nexttypematcher);
						builder.SetInsertPoint(providedrefblock);

						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* reftarget = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), PointerType::get(Type::getInt8PtrTy(context), 0)));
						builder.CreateStore(reftarget, parampayloadptr);
						builder.CreateStore(builder.CreateAdd(builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(context), sizeof(void*))), stackoffsettracker);

						Value* reftype = builder.CreateLoad(builder.CreatePointerCast(builder.CreateGEP(initialstackptr, builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(context)));
						builder.CreateStore(reftype, providedtype);

						BasicBlock* setnothingrefflagblock = BasicBlock::Create(context, "setnothingrefflag", dostufffunc);
						BasicBlock* skipblock = BasicBlock::Create(context, "skip", dostufffunc);

						Value* providednothingflag = builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
						builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

						builder.SetInsertPoint(setnothingrefflagblock);
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
						builder.CreateStore(reftarget, parampayloadptr);
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
					builder.CreateBr(moreflagsblock);
					
					builder.SetInsertPoint(moreflagsblock);
					BasicBlock* setnothingflagblock = BasicBlock::Create(context, "setnothingflag", dostufffunc);
					Value* providednothingflag = builder.CreateICmpEQ(builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(context), Metadata::EpochType_Nothing));
					builder.CreateCondBr(providednothingflag, setnothingflagblock, nextparamblock);
					builder.SetInsertPoint(setnothingflagblock);
					builder.CreateBr(nextparamblock);

					builder.SetInsertPoint(nextparamblock);

					if(expecttype != Metadata::EpochType_Nothing)
					{
						if(expectref)
						{
							Value* actualparam = builder.CreatePointerCast(builder.CreateLoad(parampayloadptr), GetJITType(ownervm, expecttype, context)->getPointerTo());
							actualparams.push_back(actualparam);
						}
						else
						{
							Value* actualparam = builder.CreatePointerCast(builder.CreateLoad(parampayloadptr), GetJITType(ownervm, expecttype, context)->getPointerTo());
							actualparams.push_back(builder.CreateLoad(actualparam));
						}
					}
					else
						actualparams.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
				}

				BasicBlock* invokeblock = BasicBlock::Create(context, "invokesuccess", dostufffunc);
				builder.CreateBr(invokeblock);

				builder.SetInsertPoint(invokeblock);

				std::ostringstream matchname;
				matchname << "JITFuncInner_" << matchoffset;

				Function* targetinnerfunc = NULL;
				FunctionType* targetinnerfunctype = GetJITFunctionType(ownervm, funcname, context);
				if(!FunctionCacheByName[matchname.str()])
				{
					targetinnerfunc = Function::Create(targetinnerfunctype, Function::InternalLinkage, matchname.str().c_str(), module);
					FunctionCacheByName[matchname.str()] = targetinnerfunc;
				}
				else
					targetinnerfunc = FunctionCacheByName[matchname.str()];

				std::vector<Value*> resolvedargs;
				resolvedargs.push_back(vmcontextptr);
				for(std::vector<Value*>::const_reverse_iterator argiter = actualparams.rbegin(); argiter != actualparams.rend(); ++argiter)
					resolvedargs.push_back(*argiter);

				typematchret = builder.CreateCall(targetinnerfunc, resolvedargs);

				builder.CreateStore(builder.CreateLoad(stackoffsettracker), typematchconsumedbytes);

				// Fixup VM stack with return value
				Type* matchrettype = typematchret->getType();
				if(matchrettype != Type::getVoidTy(context))
				{
					Value* offset = builder.CreateSub(builder.CreateLoad(typematchconsumedbytes), ConstantInt::get(Type::getInt32Ty(context), 4));
					Value* stackptr = builder.CreateLoad(builder.CreateLoad(pstackptr));
					Value* retstackptr = builder.CreateGEP(stackptr, offset);
					builder.CreateStore(typematchret, builder.CreatePointerCast(retstackptr, matchrettype->getPointerTo()));
					builder.CreateStore(retstackptr, builder.CreateLoad(pstackptr));
				}
				else
				{
					Value* offset = builder.CreateLoad(typematchconsumedbytes);
					Value* stackptr = builder.CreateLoad(builder.CreateLoad(pstackptr));
					Value* retstackptr = builder.CreateGEP(stackptr, offset);
					builder.CreateStore(retstackptr, builder.CreateLoad(pstackptr));
				}
				
				builder.CreateRetVoid();
				builder.SetInsertPoint(nexttypematcher);
			}
			break;

		case Bytecode::Instructions::Halt:
			builder.CreateCall(vmhaltfunction);
			builder.CreateBr(outerfunctionexit);
			break;

		case Bytecode::Instructions::AllocStructure:
			{
				Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(bytecode, offset);
				Value* typeconst = ConstantInt::get(Type::getInt32Ty(context), type);
				Value* handle = builder.CreateCall2(vmallocstruct, jitcontext.InnerFunction->arg_begin(), typeconst);
				jitcontext.ValuesOnStack.push(handle);

				hackstructtype = type;
			}
			break;

		case Bytecode::Instructions::ConstructSumType:
			{
				Value* vartype = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* value = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* targetid = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(targetid);
				size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

				Value* storagetarget = jitcontext.VariableMap[jitcontext.NameToIndexMap[vartarget]];

				// Set contents
				{
					std::vector<Value*> memberindices;
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 1));
					Value* valueholder = builder.CreateGEP(storagetarget, memberindices);
					builder.CreateStore(value, valueholder);
				}

				// Set type annotation
				{
					std::vector<Value*> memberindices;
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(context), 0));
					Value* typeholder = builder.CreateGEP(storagetarget, memberindices);
					builder.CreateStore(vartype, typeholder);
				}
			}
			break;

		case Bytecode::Instructions::CopyToStructure:
			{
				StringHandle variablename = Fetch<StringHandle>(bytecode, offset);
				StringHandle actualmember = Fetch<StringHandle>(bytecode, offset);

				Value* v = jitcontext.VariableMap[jitcontext.NameToIndexMap[variablename]];
				Value* cached = structurerefcache[v];
				if(!cached)
				{
					cached = builder.CreateLoad(v);
					structurerefcache[v] = cached;
				}
				Value* structptr = cached;

				const StructureDefinition& def = ownervm.GetStructureDefinition(hackstructtype);
				size_t memberindex = def.FindMember(actualmember);
				size_t memberoffset = def.GetMemberOffset(memberindex);
				Metadata::EpochTypeID membertype = def.GetMemberType(memberindex);

				Value* memberptr = builder.CreateGEP(structptr, ConstantInt::get(Type::getInt32Ty(context), memberoffset));
				Value* castmemberptr = builder.CreatePointerCast(memberptr, GetJITType(ownervm, membertype, context)->getPointerTo());
				
				builder.CreateStore(jitcontext.ValuesOnStack.top(), castmemberptr);

				jitcontext.ValuesOnStack.pop();
			}
			break;

			case Bytecode::Instructions::CopyStructure:
			{
				Value* structureptr = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Value* copyptr = builder.CreateCall2(vmcopystruct, jitcontext.InnerFunction->arg_begin(), structureptr);
				Value* castptr = builder.CreatePointerCast(copyptr, Type::getInt8PtrTy(context));
				jitcontext.ValuesOnStack.push(castptr);
			}
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

	LoadInst* stackptr = builder.CreateLoad(builder.CreateLoad(pstackptr));
	Constant* offset = ConstantInt::get(Type::getInt32Ty(context), static_cast<unsigned>(numparambytes - (numreturns * 4)));	// TODO - change to retbytes
	Value* stackptr2 = builder.CreateGEP(stackptr, offset);
	if(retval)
	{
		Value* ret = builder.CreateLoad(retval);
		builder.CreateStore(ret, builder.CreatePointerCast(stackptr2, PointerType::get(ret->getType(), 0)));
	}
	builder.CreateStore(stackptr2, builder.CreateLoad(pstackptr));

	builder.CreateRetVoid();


	if(nativematchblock)
	{
		builder.SetInsertPoint(nativematchblock);
		JITNativeTypeMatcher(ownervm, bytecode, beginoffset, endoffset, builder);
	}

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!ee)
		return;

	FunctionCache[alias] = dostufffunc;
}

void PopulateJITExecs(VM::VirtualMachine& ownervm)
{
	using namespace llvm;

	//module->dump();
	verifyModule(*module, llvm::PrintMessageAction);

#ifdef _DEBUG
	EnableStatistics();
#endif

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

	VectorizeConfig vcfg;
	vcfg.ReqChainDepth = 1;
	vcfg.MaxIter = 500;
	OurFPM.add(createBBVectorizePass(vcfg));

	OurFPM.doInitialization();

	for(std::map<StringHandle, Function*>::const_iterator iter = FunctionCache.begin(); iter != FunctionCache.end(); ++iter)
		OurFPM.run(*iter->second);

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

	PrintStatistics();

	CrashRecoveryContext::Disable();
}

