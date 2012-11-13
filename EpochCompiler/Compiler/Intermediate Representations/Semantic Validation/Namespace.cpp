//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR management objects for namespaces
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"

#include "Metadata/ScopeDescription.h"

#include "Utility/StringPool.h"


using namespace IRSemantics;


FunctionTable::FunctionTable(Namespace& mynamespace, CompileSession& session)
	: MyNamespace(mynamespace),
	  Session(session)
{
}


FunctionTable::~FunctionTable()
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
		delete iter->second;
}

void FunctionTable::Add(StringHandle name, StringHandle rawname, Function* function)
{
	if(FunctionIR.find(name) != FunctionIR.end())
	{
		delete function;

		//
		// This is a grevious error in the semantic IR generator.
		//
		// We have been given an overloaded function, but not generated
		// an overload mangled name for it; overloads should have names
		// automatically generated for them to aid in identification in
		// the semantic checking layer.
		//
		throw InternalException("Duplicate function name; overload creation failed");
	}

	function->SetRawName(rawname);
	FunctionIR.insert(std::make_pair(name, function));
}

void FunctionTable::AddOverload(StringHandle functionname, StringHandle overloadname)
{
	Session.FunctionOverloadNames[functionname].insert(overloadname);
}


//
// Allocate a function overload mangled name
//
StringHandle FunctionTable::CreateOverload(const std::wstring& name)
{
	StringHandle handle = MyNamespace.Strings.Pool(name);
	unsigned counter = FunctionOverloadCounters[handle]++;
	StringHandle ret = MyNamespace.Strings.Pool(GenerateFunctionOverloadName(handle, counter));
	FunctionOverloadNameCache.Add(std::make_pair(handle, counter), ret);
	Session.FunctionOverloadNames[handle].insert(ret);
	return ret;
}

std::wstring FunctionTable::GenerateFunctionOverloadName(StringHandle name, size_t index) const
{
	if(index == 0)
		return MyNamespace.Strings.GetPooledString(name);

	std::wostringstream format;
	format << MyNamespace.Strings.GetPooledString(name) << L"@@overload@" << index;

	return format.str();
}



StringHandle FunctionTable::AllocateTypeMatcher(StringHandle overloadname, const std::map<StringHandle, FunctionSignature>& matchingoverloads)
{
	if(MyNamespace.Parent)
		return MyNamespace.Parent->Functions.AllocateTypeMatcher(overloadname, matchingoverloads);

	if(OverloadTypeMatchers.find(overloadname) != OverloadTypeMatchers.end())
		return OverloadTypeMatchers.find(overloadname)->second;

	StringHandle matchername = MyNamespace.Strings.Pool(MyNamespace.Strings.GetPooledString(overloadname) + L"@@typematcher");
	OverloadTypeMatchers[overloadname] = matchername;

	RequiredTypeMatchers.insert(std::make_pair(matchername, matchingoverloads));

	return matchername;
}

bool FunctionTable::SignatureExists(StringHandle functionname) const
{
	return (Session.FunctionSignatures.find(functionname) != Session.FunctionSignatures.end());
}

const FunctionSignature& FunctionTable::GetSignature(StringHandle functionname) const
{
	FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(functionname);
	if(iter == Session.FunctionSignatures.end())
		throw InternalException("Function signature not cached");

	return iter->second;
}


bool FunctionTable::HasOverloads(StringHandle functionname) const
{
	OverloadMap::const_iterator iter = Session.FunctionOverloadNames.find(functionname);
	return iter != Session.FunctionOverloadNames.end();
}

StringHandle FunctionTable::GetOverloadName(StringHandle rawname, size_t overloadindex) const
{
	StringHandle ret = FunctionOverloadNameCache.Find(std::make_pair(rawname, overloadindex));
	if(!ret && MyNamespace.Parent)
		return MyNamespace.Parent->Functions.GetOverloadName(rawname, overloadindex);

	return ret;
}

const StringHandleSet& FunctionTable::GetOverloadNames(StringHandle functionname) const
{
	OverloadMap::const_iterator iter = Session.FunctionOverloadNames.find(functionname);
	if(iter == Session.FunctionOverloadNames.end())
		throw InternalException("Function not overloaded");

	return iter->second;
}

unsigned FunctionTable::GetNumOverloads(StringHandle name) const
{
	boost::unordered_map<StringHandle, unsigned>::const_iterator iter = FunctionOverloadCounters.find(name);
	if(iter == FunctionOverloadCounters.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.GetNumOverloads(name);

		return 0;
	}

	return iter->second;
}


bool FunctionTable::Exists(StringHandle functionname) const
{
	if(FunctionIR.find(functionname) != FunctionIR.end())
		return true;

	if(MyNamespace.Parent)
		return MyNamespace.Parent->Functions.Exists(functionname);

	return false;
}

const Function* FunctionTable::GetIR(StringHandle functionname) const
{
	boost::unordered_map<StringHandle, Function*>::const_iterator iter = FunctionIR.find(functionname);
	if(iter == FunctionIR.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.GetIR(functionname);

		throw InternalException("Not a user defined function");
	}

	return iter->second;
}


Function* FunctionTable::GetIR(StringHandle functionname)
{
	boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.find(functionname);
	if(iter == FunctionIR.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.GetIR(functionname);

		throw InternalException("Not a user defined function");
	}

	return iter->second;
}


bool FunctionTable::HasCompileHelper(StringHandle functionname) const
{
	return (Session.CompileTimeHelpers.find(functionname) != Session.CompileTimeHelpers.end());
}

CompilerHelperPtr FunctionTable::GetCompileHelper(StringHandle functionname) const
{
	return Session.CompileTimeHelpers.find(functionname)->second;
}

void FunctionTable::SetCompileHelper(StringHandle functionname, CompilerHelperPtr helper)
{
	Session.CompileTimeHelpers.insert(std::make_pair(functionname, helper));
}

void FunctionTable::SetSignature(StringHandle functionname, const FunctionSignature& signature)
{
	Session.FunctionSignatures[functionname] = signature;
}

void FunctionTable::MarkFunctionWithStaticPatternMatching(StringHandle rawname, StringHandle overloadname)
{
	FunctionsWithStaticPatternMatching.insert(std::make_pair(rawname, overloadname));
}

void FunctionTable::GenerateStructureFunctions(StringHandle name, Structure* structure)
{
	const std::wstring& structurename = MyNamespace.Strings.GetPooledString(name);
	const std::vector<std::pair<StringHandle, StructureMember*> >& members = structure->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
	{
		std::wstring overloadname = GenerateStructureMemberAccessOverloadName(structurename, MyNamespace.Strings.GetPooledString(iter->first));
		StructureMemberAccessOverloadNameCache.Add(std::make_pair(name, iter->first), MyNamespace.Strings.PoolFast(overloadname));
	}
}

std::wstring FunctionTable::GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername)
{
	return L".@@" + structurename + L"@@" + membername;
}

bool FunctionTable::FunctionNeedsDynamicPatternMatching(StringHandle overloadname) const
{
	return (FunctionsNeedingDynamicPatternMatching.count(overloadname) > 0);
}

StringHandle FunctionTable::GetDynamicPatternMatcherForFunction(StringHandle overloadname) const
{
	return PatternMatcherNameCache.Find(overloadname);
}

bool FunctionTable::Validate(CompileErrors& errors) const
{
	bool valid = true;

	for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
	{
		if(!iter->second->Validate(MyNamespace))
			valid = false;
	}

	try
	{
		StringHandle entrypointhandle = MyNamespace.Strings.Find(L"entrypoint");
		if(FunctionIR.find(entrypointhandle) == FunctionIR.end())
		{
			errors.SetLocation(L"", 0, 0, L"");
			errors.SemanticError("Program has no entrypoint function");
			valid = false;
		}
	}
	catch(const RecoverableException&)
	{
		valid = false;
	}

	return valid;
}

bool FunctionTable::TypeInference(InferenceContext& context, CompileErrors& errors)
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
	{
		if(!iter->second->TypeInference(MyNamespace, context, errors))
			return false;
	}

	return true;
}

bool FunctionTable::CompileTimeCodeExecution(CompileErrors& errors)
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(MyNamespace, errors))
			return false;
	}

	for(std::multimap<StringHandle, StringHandle>::const_iterator iter = FunctionsWithStaticPatternMatching.begin(); iter != FunctionsWithStaticPatternMatching.end(); ++iter)
	{
		StringHandle rawname = iter->first;
		StringHandle overloadname = iter->second;

		Function* function = FunctionIR.find(overloadname)->second;
		function->TypeInferenceParamsOnly(MyNamespace, errors);

		const FunctionSignature functionsignature = function->GetFunctionSignature(MyNamespace);
		for(size_t i = 0; i < FunctionOverloadCounters[rawname]; ++i)
		{
			StringHandle thisoverloadname = GetOverloadName(rawname, i);
			if(thisoverloadname == overloadname)
				continue;

			Function* overload = FunctionIR.find(thisoverloadname)->second;
			overload->TypeInferenceParamsOnly(MyNamespace, errors);
			if(functionsignature.MatchesDynamicPattern(overload->GetFunctionSignature(MyNamespace)))
				FunctionsNeedingDynamicPatternMatching.insert(thisoverloadname);
		}
	}

	for(std::set<StringHandle>::const_iterator iter = FunctionsNeedingDynamicPatternMatching.begin(); iter != FunctionsNeedingDynamicPatternMatching.end(); ++iter)
	{
		StringHandle funcname = *iter;
		const std::wstring& funcnamestr = MyNamespace.Strings.GetPooledString(funcname);
		std::wstring patternmatchername = GeneratePatternMatcherName(funcnamestr);
		StringHandle pmnhandle = MyNamespace.Strings.Pool(patternmatchername);
		PatternMatcherNameCache.Add(funcname, pmnhandle);
	}

	return true;
}

StringHandle FunctionTable::InstantiateAllOverloads(StringHandle templatename, const CompileTimeParameterVector& args, CompileErrors& errors)
{
	StringHandle ret = 0;

	if(GetNumOverloads(templatename) > 1)
	{
		const StringHandleSet& overloadnames = GetOverloadNames(templatename);
		for(StringHandleSet::const_iterator iter = overloadnames.begin(); iter != overloadnames.end(); ++iter)
		{
			if(!IsFunctionTemplate(*iter))
				continue;

			StringHandle instname = InstantiateTemplate(*iter, args, errors);
			if(!ret)
				ret = instname;
		}
	}
	else
		ret = InstantiateTemplate(templatename, args, errors);

	return ret;
}



StringHandle FunctionTable::InstantiateTemplate(StringHandle templatename, const CompileTimeParameterVector& originalargs, CompileErrors& errors)
{
	CompileTimeParameterVector args(originalargs);
	for(CompileTimeParameterVector::iterator iter = args.begin(); iter != args.end(); ++iter)
	{
		// TODO - filter this to arguments that are typenames?
		if(MyNamespace.Types.Aliases.HasWeakAliasNamed(iter->Payload.LiteralStringHandleValue))
			iter->Payload.LiteralStringHandleValue = MyNamespace.Types.Aliases.GetWeakTypeBaseName(iter->Payload.LiteralStringHandleValue);
	}

	Namespace* ns = &MyNamespace;
	while(ns)
	{
		InstantiationMap::iterator iter = ns->Functions.Instantiations.find(templatename);
		if(iter != ns->Functions.Instantiations.end())
		{
			InstancesAndArguments& instances = iter->second;

			for(InstancesAndArguments::const_iterator instanceiter = instances.begin(); instanceiter != instances.end(); ++instanceiter)
			{
				if(args.size() != instanceiter->second.size())
					continue;

				bool match = true;
				for(unsigned i = 0; i < args.size(); ++i)
				{
					if(!args[i].PatternMatch(instanceiter->second[i]))
					{
						match = false;
						break;
					}
				}

				if(match)
					return instanceiter->first;
			}
		}

		if(!ns->Parent)
			break;

		ns = ns->Parent;
	}

	StringHandle mangledname = ns->Functions.CreateOverload(MyNamespace.Strings.GetPooledString(templatename));
	ns->Functions.FunctionOverloadCounters[mangledname]++;
	ns->Functions.FunctionOverloadNameCache.Add(std::make_pair(mangledname, 0), mangledname);
	ns->Functions.Instantiations[templatename].insert(std::make_pair(mangledname, args));
	ns->Functions.FunctionIR[mangledname] = new Function(GetIR(templatename), MyNamespace, args);
	ns->Functions.FunctionIR[mangledname]->SetName(mangledname);
	ns->Functions.FunctionIR[mangledname]->SetRawName(mangledname);
	ns->Functions.FunctionIR[mangledname]->PopulateScope(MyNamespace, errors);
	ns->Functions.FunctionIR[mangledname]->FixupScope();

	return mangledname;
}

bool FunctionTable::IsFunctionTemplate(StringHandle name) const
{
	boost::unordered_map<StringHandle, Function*>::const_iterator iter = FunctionIR.find(name);
	if(iter == FunctionIR.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.IsFunctionTemplate(name);

		return false;
	}

	return iter->second->IsTemplate();
}



bool OperatorTable::PrefixExists(StringHandle operatorname) const
{
	return Session.Identifiers.UnaryPrefixes.find(MyNamespace.Strings.GetPooledString(operatorname)) != Session.Identifiers.UnaryPrefixes.end();
}

int OperatorTable::GetPrecedence(StringHandle operatorname) const
{
	return Session.OperatorPrecedences.find(operatorname)->second;
}


FunctionTagTable::FunctionTagTable(const CompileSession& session)
	: Session(session)
{
}


bool FunctionTagTable::Exists(StringHandle tagname) const
{
	return (Session.FunctionTagHelpers.find(Session.StringPool.GetPooledString(tagname)) != Session.FunctionTagHelpers.end());
}

FunctionTagHelperPtr FunctionTagTable::GetHelper(StringHandle tagname) const
{
	return Session.FunctionTagHelpers.find(Session.StringPool.GetPooledString(tagname))->second;
}


OperatorTable::OperatorTable(Namespace& mynamespace, const CompileSession& session)
	: MyNamespace(mynamespace),
	  Session(session)
{
}


#pragma warning(push)
#pragma warning(disable: 4355)

Namespace::Namespace(GlobalIDSpace& idspace, StringPoolManager& strings, CompileSession& session)
	: Strings(strings),
	  CounterAnonParam(0),
	  CounterLexicalScope(0),
	  GlobalScope(new ScopeDescription()),
	  Types(*this, idspace),
	  Functions(*this, session),
	  FunctionTags(session),
	  Operators(*this, session),
	  Session(session),
	  Parent(NULL)
{
}

#pragma warning(pop)

Namespace::~Namespace()
{
	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
		delete *iter;

	delete GlobalScope;
}

void Namespace::AddScope(ScopeDescription* scope)
{
	AddScope(scope, LexicalScopeNameCache.Find(scope));
}

void Namespace::AddScope(ScopeDescription* scope, StringHandle name)
{
	if(Parent)
	{
		Parent->AddScope(scope, name);
		return;
	}

	LexicalScopes.insert(std::make_pair(name, scope));
}


InferenceContext::PossibleParameterTypes FunctionTable::GetExpectedTypes(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors& errors) const
{
	if(scope.HasVariable(name) && scope.GetVariableTypeByID(name) == VM::EpochType_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = FunctionIR.find(contextname);
		if(funciter == FunctionIR.end())
		{
			//
			// This is a critical internal failure. A function overload name
			// has been registered but the corresponding definition of the
			// function cannot be located.
			//
			// Examine the handling of overload registration, name resolution,
			// and definition storage. We should not reach the phase of type
			// inference until all function definitions have been visited by
			// AST traversal in prior semantic validation passes.
			//
			throw InternalException("Function overload registered but definition not found");
		}
		
		InferenceContext::PossibleParameterTypes ret(1, InferenceContext::TypePossibilities());

		FunctionSignature sig = funciter->second->GetParameterSignature(name, MyNamespace);
		for(size_t i = 0; i < sig.GetNumParameters(); ++i)
			ret.back().push_back(sig.GetParameter(i).Type);
	
		return ret;
	}

	unsigned numoverloads = GetNumOverloads(name);
	if(numoverloads)
	{
		InferenceContext::PossibleParameterTypes ret;
		ret.reserve(numoverloads);

		for(size_t i = 0; i < numoverloads; ++i)
		{
			StringHandle overloadname = GetOverloadName(name, i);
			FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(overloadname);
			if(fsigiter != Session.FunctionSignatures.end())
			{
				ret.push_back(InferenceContext::TypePossibilities());

				for(size_t j = 0; j < fsigiter->second.GetNumParameters(); ++j)
					ret.back().push_back(fsigiter->second.GetParameter(j).Type);
			}
			else
			{
				std::map<StringHandle, StringHandle>::const_iterator mangleiter;
				Function* ir = const_cast<Function*>(GetIR(overloadname));
				if(ir)
				{
					if(ir->IsTemplate())
						continue;

					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					ir->TypeInference(MyNamespace, context, errors);
					ret.push_back(InferenceContext::TypePossibilities());
					FunctionSignature signature = ir->GetFunctionSignature(MyNamespace);

					for(size_t j = 0; j < signature.GetNumParameters(); ++j)
						ret.back().push_back(signature.GetParameter(j).Type);
				}
				else
				{
					//
					// This is a critical internal failure. A function overload name
					// has been registered but the corresponding definition of the
					// function cannot be located.
					//
					// Examine the handling of overload registration, name resolution,
					// and definition storage. We should not reach the phase of type
					// inference until all function definitions have been visited by
					// AST traversal in prior semantic validation passes.
					//
					throw InternalException("Function overload registered but definition not found");
				}
			}
		}

		return ret;
	}

	OverloadMap::const_iterator ovmapiter = Session.FunctionOverloadNames.find(name);
	if(ovmapiter != Session.FunctionOverloadNames.end())
	{
		InferenceContext::PossibleParameterTypes ret(ovmapiter->second.size(), InferenceContext::TypePossibilities());
		size_t i = 0;
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(*oviter);
			if(fsigiter != Session.FunctionSignatures.end())
			{
				for(size_t j = 0; j < fsigiter->second.GetNumParameters(); ++j)
					ret[i].push_back(fsigiter->second.GetParameter(j).Type);
			}

			++i;
		}
		
		return ret;
	}

	FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(name);
	if(fsigiter != Session.FunctionSignatures.end())
	{
		InferenceContext::PossibleParameterTypes ret(1, InferenceContext::TypePossibilities());
		for(size_t i = 0; i < fsigiter->second.GetNumParameters(); ++i)
			ret.front().push_back(fsigiter->second.GetParameter(i).Type);

		return ret;
	}

	return InferenceContext::PossibleParameterTypes();
}


InferenceContext::PossibleSignatureSet FunctionTable::GetExpectedSignatures(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors&) const
{
	if(scope.HasVariable(name) && scope.GetVariableTypeByID(name) == VM::EpochType_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = FunctionIR.find(contextname);
		if(funciter == FunctionIR.end())
		{
			//
			// This is a critical internal failure. A function overload name
			// has been registered but the corresponding definition of the
			// function cannot be located.
			//
			// Examine the handling of overload registration, name resolution,
			// and definition storage. We should not reach the phase of type
			// inference until all function definitions have been visited by
			// AST traversal in prior semantic validation passes.
			//
			throw InternalException("Function overload registered but definition not found");
		}
		
		InferenceContext::PossibleSignatureSet ret(1, InferenceContext::SignaturePossibilities());
		ret.back().push_back(funciter->second->GetParameterSignature(name, MyNamespace));

		return ret;
	}

	boost::unordered_map<StringHandle, unsigned>::const_iterator iter = FunctionOverloadCounters.find(name);
	if(iter != FunctionOverloadCounters.end())
	{
		InferenceContext::PossibleSignatureSet ret;
		ret.reserve(iter->second);

		for(size_t i = 0; i < iter->second; ++i)
		{
			StringHandle overloadname = i ? FunctionOverloadNameCache.Find(std::make_pair(name, i)) : name;
			boost::unordered_map<StringHandle, Function*>::const_iterator funciter = FunctionIR.find(overloadname);
			if(funciter != FunctionIR.end())
			{
				ret.push_back(InferenceContext::SignaturePossibilities());

				std::vector<StringHandle> paramnames = funciter->second->GetParameterNames();
				for(std::vector<StringHandle>::const_iterator paramiter = paramnames.begin(); paramiter != paramnames.end(); ++paramiter)
					ret.back().push_back(funciter->second->GetParameterSignature(*paramiter, MyNamespace));
			}
			else
			{
				FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(overloadname);
				if(fsigiter != Session.FunctionSignatures.end())
				{
					ret.push_back(InferenceContext::SignaturePossibilities());

					for(size_t i = 0; i < fsigiter->second.GetNumParameters(); ++i)
						ret.back().push_back(fsigiter->second.GetFunctionSignature(i));
				}
				else
				{
					//
					// This is a critical internal failure. A function overload name
					// has been registered but the corresponding definition of the
					// function cannot be located.
					//
					// Examine the handling of overload registration, name resolution,
					// and definition storage. We should not reach the phase of type
					// inference until all function definitions have been visited by
					// AST traversal in prior semantic validation passes.
					//
					throw InternalException("Function overload registered but definition not found");
				}
			}
		}

		return ret;
	}

	OverloadMap::const_iterator ovmapiter = Session.FunctionOverloadNames.find(name);
	if(ovmapiter != Session.FunctionOverloadNames.end())
	{
		InferenceContext::PossibleSignatureSet ret(ovmapiter->second.size(), InferenceContext::SignaturePossibilities());
		size_t i = 0;
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(*oviter);
			if(fsigiter != Session.FunctionSignatures.end())
			{
				for(size_t j = 0; j < fsigiter->second.GetNumParameters(); ++j)
					ret[i].push_back(fsigiter->second.GetFunctionSignature(j));
			}

			++i;
		}
		
		return ret;
	}

	FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(name);
	if(fsigiter != Session.FunctionSignatures.end())
	{
		InferenceContext::PossibleSignatureSet ret(1, InferenceContext::SignaturePossibilities());
		for(size_t i = 0; i < fsigiter->second.GetNumParameters(); ++i)
			ret.front().push_back(fsigiter->second.GetFunctionSignature(i));

		return ret;
	}

	return InferenceContext::PossibleSignatureSet();
}

StringHandle FunctionTable::FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const
{
	StringHandle ret = StructureMemberAccessOverloadNameCache.Find(std::make_pair(structurename, membername));
	if(!ret && MyNamespace.Parent)
		return MyNamespace.Parent->Functions.FindStructureMemberAccessOverload(structurename, membername);

	return ret;
}


unsigned FunctionTable::FindMatchingFunctions(StringHandle identifier, const FunctionSignature& expectedsignature, InferenceContext& context, CompileErrors& errors, StringHandle& resolvedidentifier)
{
	bool foundidentifier = false;
	unsigned signaturematches = 0;

	FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(identifier);
	if(fsigiter != Session.FunctionSignatures.end())
	{
		foundidentifier = true;
		if(expectedsignature.Matches(fsigiter->second))
			++signaturematches;
	}
	else if(Exists(identifier))
	{
		unsigned overloadcount = GetNumOverloads(identifier);
		for(unsigned j = 0; j < overloadcount; ++j)
		{
			foundidentifier = true;
			StringHandle overloadname = GetOverloadName(identifier, j);
			Function* func = FunctionIR.find(overloadname)->second;

			func->TypeInference(MyNamespace, context, errors);
			FunctionSignature sig = func->GetFunctionSignature(MyNamespace);
			if(expectedsignature.Matches(sig))
			{
				resolvedidentifier = overloadname;
				++signaturematches;
			}
		}
	}

	return signaturematches;
}

StringHandle Namespace::AllocateLexicalScopeName(const CodeBlock* blockptr)
{
	StringHandle ret = Strings.PoolFast(GenerateLexicalScopeName(blockptr->GetScope()));
	LexicalScopeNameCache.Add(blockptr->GetScope(), ret);
	return ret;
}

StringHandle Namespace::FindLexicalScopeName(const CodeBlock* blockptr) const
{
	return LexicalScopeNameCache.Find(blockptr->GetScope());
}

StringHandle Namespace::FindLexicalScopeName(const ScopeDescription* scopeptr) const
{
	return LexicalScopeNameCache.Find(scopeptr);
}

std::wstring Namespace::GenerateLexicalScopeName(const ScopeDescription* scopeptr)
{
	std::wostringstream name;
	name << L"@@scope@" << scopeptr;
	return name.str();
}

ScopeDescription* Namespace::GetGlobalScope() const
{
	return GlobalScope;
}

Bytecode::EntityTag Namespace::GetEntityTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = Session.InfoTable.Entities->begin(); iter != Session.InfoTable.Entities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.InfoTable.ChainedEntities->begin(); iter != Session.InfoTable.ChainedEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.InfoTable.PostfixEntities->begin(); iter != Session.InfoTable.PostfixEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	//
	// This represents a failure in the parser.
	//
	// The parser should reject any code which attempts to utilize an undefined
	// entity. By the time we reach semantic validation, it should no longer be
	// possible to refer to an entity identifier which doesn't have any backing
	// descriptor data (including the entity type-tag we're retrieving here).
	//
	throw InternalException("String handle does not correspond to any known type of entity");
}

Bytecode::EntityTag Namespace::GetEntityCloserTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = Session.InfoTable.PostfixClosers->begin(); iter != Session.InfoTable.PostfixClosers->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	//
	// This represents a failure in the parser.
	//
	// The parser should reject any code which attempts to utilize an undefined
	// entity. By the time we reach semantic validation, it should no longer be
	// possible to refer to an entity identifier which doesn't have any backing
	// descriptor data (including the entity type-tag we're retrieving here).
	//
	throw InternalException("String handle does not correspond to any known type of entity closer");
}


//
// Add a global code block to a program
//
size_t Namespace::AddGlobalCodeBlock(CodeBlock* code)
{
	size_t index = GlobalCodeBlocks.size();
	Strings.Pool(GenerateAnonymousGlobalScopeName(index));
	GlobalCodeBlocks.push_back(code);
	AddScope(code->GetScope());
	return index;
}

//
// Retrieve a given global code block from the program
//
const CodeBlock& Namespace::GetGlobalCodeBlock(size_t index) const
{
	if(index >= GlobalCodeBlocks.size())
	{
		//
		// This is a failure on the part of the caller.
		//
		// Callers should pay attention to how many global code blocks
		// have been defined and make sure they don't ask for one that
		// doesn't exist. See GetNumGlobalCodeBlocks().
		//
		throw InternalException("Requested a global code block index that has not been defined");
	}

	return *GlobalCodeBlocks[index];
}

//
// Retrieve the count of global code blocks in a given program
//
size_t Namespace::GetNumGlobalCodeBlocks() const
{
	return GlobalCodeBlocks.size();
}

//
// Retrieve the name of a given global code block
//
StringHandle Namespace::GetGlobalCodeBlockName(size_t index) const
{
	return Strings.Find(GenerateAnonymousGlobalScopeName(index));
}

std::wstring Namespace::GenerateAnonymousGlobalScopeName(size_t index)
{
	std::wostringstream name;
	name << L"@@globalscope@" << index;
	return name.str();
}

//
// Allocate a name for an anonymous function parameter (usually an expression)
//
StringHandle Namespace::AllocateAnonymousParamName()
{
	std::wostringstream name;
	name << L"@@anonparam@" << (++CounterAnonParam);
	return Strings.PoolFast(name.str());
}


std::wstring FunctionTable::GeneratePatternMatcherName(const std::wstring& funcname)
{
	return funcname + L"@@patternmatch";
}


bool Namespace::Validate(CompileErrors& errors) const
{
	bool valid = true;

	if(!Types.Validate(errors))
		valid = false;

	for(std::vector<CodeBlock*>::const_iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->Validate(*this))
			valid = false;
	}

	if(!Functions.Validate(errors))
		valid = false;

	return valid;
}

bool Namespace::TypeInference(CompileErrors& errors)
{
	InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->TypeInference(*this, context, errors))
			return false;
	}

	return Functions.TypeInference(context, errors);
}

bool Namespace::CompileTimeCodeExecution(CompileErrors& errors)
{
	if(!Types.CompileTimeCodeExecution(errors))
		return false;

	// TODO - reenable global stuffs
	/*
	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(*this, errors))
			return false;
	}
	*/

	return Functions.CompileTimeCodeExecution(errors);
}

Namespace* Namespace::CreateTemplateDummy(Namespace& parent, const std::vector<std::pair<StringHandle, VM::EpochTypeID> >& params, const CompileTimeParameterVector& args)
{
	Namespace* ret = new Namespace(parent.Types.IDSpace, parent.Strings, parent.Session);
	ret->SetParent(&parent);

	if(params.size() != args.size())
		throw InternalException("Template parameter/argument mismatch");

	for(size_t i = 0; i < params.size(); ++i)
	{
		if(params[i].second == VM::EpochType_Wildcard)
		{
			ret->Types.Aliases.AddWeakAlias(params[i].first, parent.Types.GetTypeByName(static_cast<StringHandle>(args[i].Payload.IntegerValue)));
		}
	}

	return ret;
}

void Namespace::SetParent(Namespace* parent)
{
	Parent = parent;
}


