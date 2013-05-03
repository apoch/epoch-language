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


//
// Construct and initialize a table of functions
//
FunctionTable::FunctionTable(Namespace& mynamespace, CompileSession& session)
	: MyNamespace(mynamespace),
	  Session(session)
{
}

//
// Destruct and clean up a table of functions
//
FunctionTable::~FunctionTable()
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
		delete iter->second;
}

//
// Add a new entry to a table of functions
//
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

//
// Add an overload mapping to a function table that links to
// an automatically-generated function (i.e. no IR)
//
void FunctionTable::AddInternalOverload(StringHandle functionname, StringHandle overloadname)
{
	Session.FunctionOverloadNames[functionname].insert(overloadname);
}


//
// Allocate a function overload mangled name
//
// Registers the newly created name as an overload of
// the base function name provided.
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

//
// Internal helper for overload name mangling
//
std::wstring FunctionTable::GenerateFunctionOverloadName(StringHandle name, size_t index) const
{
	if(index == 0)
		return MyNamespace.Strings.GetPooledString(name);

	std::wostringstream format;
	format << MyNamespace.Strings.GetPooledString(name) << L"@@overload@" << index;

	return format.str();
}


//
// Create a type matching dispatcher function
//
// These are magic entities which accept a number of parameters
// and type annotations in the VM. Based on the types of the
// provided arguments, the dispatcher invokes the appropriate
// function overload. Similar to virtual dispatch but supports
// multiple dispatch as well by virtue of examining all of the
// provided types.
//
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

//
// Determine if a function signature has been registered for the given function
//
bool FunctionTable::SignatureExists(StringHandle functionname) const
{
	return (Session.FunctionSignatures.find(functionname) != Session.FunctionSignatures.end());
}

//
// Retrieve a cached function signature for the given function
//
// Can also be used for accessing signatures of intrinsic functions
// or other non-IR-bearing functions.
//
const FunctionSignature& FunctionTable::GetSignature(StringHandle functionname) const
{
	FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(functionname);
	if(iter == Session.FunctionSignatures.end())
		throw InternalException("Function signature not cached");

	return iter->second;
}

//
// Determine if any overloads have been registered for the given base name
//
bool FunctionTable::HasOverloads(StringHandle functionname) const
{
	OverloadMap::const_iterator iter = Session.FunctionOverloadNames.find(functionname);
	return iter != Session.FunctionOverloadNames.end();
}

//
// Retrieve the given overload mangled name of a function
//
// Since overload index is a stable way to reference overloads,
// we prefer it to complicated solutions involving mangling out
// the names/types of parameters and so on.
//
StringHandle FunctionTable::GetOverloadName(StringHandle rawname, size_t overloadindex) const
{
	StringHandle ret = FunctionOverloadNameCache.Find(std::make_pair(rawname, overloadindex));
	if(!ret && MyNamespace.Parent)
		return MyNamespace.Parent->Functions.GetOverloadName(rawname, overloadindex);

	return ret;
}

//
// Retrieve a list of all known names corresponding
// to overloads of a given function.
//
const StringHandleSet& FunctionTable::GetOverloadNames(StringHandle functionname) const
{
	OverloadMap::const_iterator iter = Session.FunctionOverloadNames.find(functionname);
	if(iter == Session.FunctionOverloadNames.end())
		throw InternalException("Function not overloaded");

	return iter->second;
}

//
// Determine how many overloads of a given function exist.
//
// This should return 1 in the case of a function which is not
// overloaded but is defined.
//
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

//
// Determine if IR exists for a given function
//
bool FunctionTable::IRExists(StringHandle functionname) const
{
	if(FunctionIR.find(functionname) != FunctionIR.end())
		return true;

	if(MyNamespace.Parent)
		return MyNamespace.Parent->Functions.IRExists(functionname);

	return false;
}

//
// Retrieve the IR of a given function
//
const Function* FunctionTable::GetIR(StringHandle functionname) const
{
	boost::unordered_map<StringHandle, Function*>::const_iterator iter = FunctionIR.find(functionname);
	if(iter == FunctionIR.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.GetIR(functionname);

		throw RecoverableException("Not a user defined function");
	}

	return iter->second;
}

//
// Retrieve the IR of a given function (mutable version)
//
Function* FunctionTable::GetIR(StringHandle functionname)
{
	boost::unordered_map<StringHandle, Function*>::iterator iter = FunctionIR.find(functionname);
	if(iter == FunctionIR.end())
	{
		if(MyNamespace.Parent)
			return MyNamespace.Parent->Functions.GetIR(functionname);

		throw RecoverableException("Not a user defined function");
	}

	return iter->second;
}

//
// Determine if the given function has a compile-time helper
//
bool FunctionTable::HasCompileHelper(StringHandle functionname) const
{
	return (Session.CompileTimeHelpers.find(functionname) != Session.CompileTimeHelpers.end());
}

//
// Retrieve the compile-time helper for a given function
//
CompilerHelperPtr FunctionTable::GetCompileHelper(StringHandle functionname) const
{
	return Session.CompileTimeHelpers.find(functionname)->second;
}

//
// Assign a compile-time helper to a given function
//
void FunctionTable::SetCompileHelper(StringHandle functionname, CompilerHelperPtr helper)
{
	Session.CompileTimeHelpers.insert(std::make_pair(functionname, helper));
}

//
// Store/cache a signature for the given function
//
void FunctionTable::SetSignature(StringHandle functionname, const FunctionSignature& signature)
{
	FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(functionname);
	if(iter != Session.FunctionSignatures.end() && !iter->second.Matches(signature))
		throw FatalException("Attempting to replace function signature");

	Session.FunctionSignatures[functionname] = signature;
}

//
// Indicate that a function overload requires static pattern matching
//
// This is used as a hint that a function may also require dynamic pattern
// matching later on; setting up the dynamic matcher is done during the
// compile-time code execution phase.
//
void FunctionTable::MarkFunctionWithStaticPatternMatching(StringHandle rawname, StringHandle overloadname)
{
	FunctionsWithStaticPatternMatching.insert(std::make_pair(rawname, overloadname));
}

//
// Generate overloads of the . operator for accessing the
// members of a given structure; these are not mapped to
// any IR, because the implementations are automatically
// generated by the compiler.
//
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

//
// Internal helper for mangling . overload names
//
std::wstring FunctionTable::GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername)
{
	return L".@@" + structurename + L"@@" + membername;
}

//
// Determine if a given overload requires dynamic pattern matching
//
bool FunctionTable::FunctionNeedsDynamicPatternMatching(StringHandle overloadname) const
{
	return (FunctionsNeedingDynamicPatternMatching.count(overloadname) > 0);
}

//
// Retrieve the name of the dynamic pattern matcher for a given function
//
StringHandle FunctionTable::GetDynamicPatternMatcherForFunction(StringHandle overloadname) const
{
	return PatternMatcherNameCache.Find(overloadname);
}

//
// Perform validation checks for a function table
//
// Also relays validation request to all functions in the table
//
bool FunctionTable::Validate(CompileErrors& errors) const
{
	bool valid = true;

	for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = FunctionIR.begin(); iter != FunctionIR.end(); ++iter)
	{
		if(!iter->second->Validate(MyNamespace))
			valid = false;
	}

	if(!MyNamespace.Parent)
	{
		try
		{
			StringHandle entrypointhandle = MyNamespace.Strings.Find(L"entrypoint");
			GetIR(entrypointhandle);
		}
		catch(const RecoverableException&)
		{
			errors.SetLocation(L"", 0, 0, L"");
			errors.SemanticError("Program has no entrypoint function");

			valid = false;
		}
	}

	return valid;
}

//
// Perform type inference on all functions in the table
//
bool FunctionTable::TypeInference(InferenceContext& context, CompileErrors& errors)
{
	boost::unordered_map<StringHandle, Function*> ircopy(FunctionIR);
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = ircopy.begin(); iter != ircopy.end(); ++iter)
	{
		if(!iter->second->TypeInference(MyNamespace, context, errors))
			return false;
	}

	for(std::set<StringHandle>::const_iterator iter = PendingTypeInference.begin(); iter != PendingTypeInference.end(); ++iter)
	{
		if(!FunctionIR[*iter]->TypeInference(MyNamespace, context, errors))
			return false;
	}

	return true;
}

//
// Perform compile-time code execution on all functions in the table
//
// Also sets up dynamic pattern matching as necessary
//
bool FunctionTable::CompileTimeCodeExecution(CompileErrors& errors)
{
	boost::unordered_map<StringHandle, Function*> ircopy(FunctionIR);
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = ircopy.begin(); iter != ircopy.end(); ++iter)
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

//
// Template support - instantiate all overloads of a function
//
// Ensures that all required overloads of a templated function are
// instantiated with the appropriate arguments, so that further
// compilation efforts can rely on them existing.
//
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
			if(!ret && instname)
				ret = instname;
		}
	}
	else
		ret = InstantiateTemplate(templatename, args, errors);

	return ret;
}


//
// Internal helper for instantiating a function template
//
StringHandle FunctionTable::InstantiateTemplate(StringHandle templatename, const CompileTimeParameterVector& originalargs, CompileErrors& errors)
{
	const Function* templatefunc = GetIR(templatename);
	if(templatefunc->TemplateParams.size() != originalargs.size())
		return 0;

	// TODO - validate arguments against parameters

	CompileTimeParameterVector args(originalargs);
	for(size_t i = 0; i < args.size(); ++i)
	{
		if(templatefunc->TemplateParams[i].second != Metadata::EpochType_Wildcard)
			continue;

		if(MyNamespace.Types.Aliases.HasWeakAliasNamed(args[i].Payload.LiteralStringHandleValue))
			args[i].Payload.LiteralStringHandleValue = MyNamespace.Types.Aliases.GetWeakTypeBaseName(args[i].Payload.LiteralStringHandleValue);
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

	InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);

	StringHandle mangledname = ns->Functions.CreateOverload(MyNamespace.Strings.GetPooledString(templatename));
	ns->Functions.FunctionOverloadCounters[mangledname]++;
	ns->Functions.FunctionOverloadNameCache.Add(std::make_pair(mangledname, 0), mangledname);
	ns->Functions.Instantiations[templatename].insert(std::make_pair(mangledname, args));
	ns->Functions.FunctionIR[mangledname] = new Function(GetIR(templatename), MyNamespace, args, errors);
	ns->Functions.FunctionIR[mangledname]->SetName(mangledname);
	ns->Functions.FunctionIR[mangledname]->SetRawName(mangledname);
	ns->Functions.FunctionIR[mangledname]->PopulateScope(MyNamespace, errors);
	ns->Functions.FunctionIR[mangledname]->FixupScope();
	ns->Functions.FunctionIR[mangledname]->TypeInferenceParamsReturnOnly(MyNamespace, context, errors);
	ns->Functions.Session.FunctionSignatures[mangledname] = ns->Functions.FunctionIR[mangledname]->GetFunctionSignature(*ns);

	ns->Functions.PendingTypeInference.insert(mangledname);

	return mangledname;
}

//
// Determine if the given function name corresponds to a template
//
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

//
// Determine the possible types of parameters to a given function
//
// This is slightly complex because of the need to handle both functions
// which have IR and functions which do not (intrinsics, automatically
// generated code, etc.).
//
InferenceContext::PossibleParameterTypes FunctionTable::GetExpectedTypes(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors& errors) const
{
	if(scope.HasVariable(name) && Metadata::GetTypeFamily(scope.GetVariableTypeByID(name)) == Metadata::EpochTypeFamily_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = FunctionIR.find(contextname);
		if(funciter == FunctionIR.end())
		{
			if(!MyNamespace.Parent)
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
			else
				return MyNamespace.Parent->Functions.GetExpectedTypes(name, scope, contextname, errors);
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

//
// Similar to GetExpectedTypes, but retrieves higher-order function signatures
//
InferenceContext::PossibleSignatureSet FunctionTable::GetExpectedSignatures(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors& errors) const
{
	if(scope.HasVariable(name) && Metadata::GetTypeFamily(scope.GetVariableTypeByID(name)) == Metadata::EpochTypeFamily_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = FunctionIR.find(contextname);
		if(funciter == FunctionIR.end())
		{
			if(!MyNamespace.Parent)
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
			else
				return MyNamespace.Parent->Functions.GetExpectedSignatures(name, scope, contextname, errors);
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
			else
				throw FatalException("No signature available for function overload");

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

//
// Find the cached name of a . operator overload
//
StringHandle FunctionTable::FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const
{
	StringHandle ret = StructureMemberAccessOverloadNameCache.Find(std::make_pair(structurename, membername));
	if(!ret && MyNamespace.Parent)
		return MyNamespace.Parent->Functions.FindStructureMemberAccessOverload(structurename, membername);

	return ret;
}

//
// Locate functions which match the given signature
//
// Useful for combining overloading with higher order functions
//
unsigned FunctionTable::FindMatchingFunctions(StringHandle identifier, const FunctionSignature& expectedsignature, InferenceContext& context, CompileErrors& errors, StringHandle& resolvedidentifier)
{
	unsigned signaturematches = 0;

	FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(identifier);
	if(fsigiter != Session.FunctionSignatures.end())
	{
		if(expectedsignature.Matches(fsigiter->second))
			++signaturematches;
	}
	else if(IRExists(identifier))
	{
		unsigned overloadcount = GetNumOverloads(identifier);
		for(unsigned j = 0; j < overloadcount; ++j)
		{
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

//
// Internal helper for pattern matcher name generation
//
std::wstring FunctionTable::GeneratePatternMatcherName(const std::wstring& funcname)
{
	return funcname + L"@@patternmatch";
}


//
// Construct and initialize a function tag table
//
FunctionTagTable::FunctionTagTable(const CompileSession& session)
	: Session(session)
{
}

//
// Determine if the given function tag exists
//
bool FunctionTagTable::Exists(StringHandle tagname) const
{
	return (Session.FunctionTagHelpers.find(Session.StringPool.GetPooledString(tagname)) != Session.FunctionTagHelpers.end());
}

//
// Retrieve the compilation helper for a given function tag
//
FunctionTagHelperPtr FunctionTagTable::GetHelper(StringHandle tagname) const
{
	return Session.FunctionTagHelpers.find(Session.StringPool.GetPooledString(tagname))->second;
}


//
// Construct and initialize an operator table
//
OperatorTable::OperatorTable(Namespace& mynamespace, const CompileSession& session)
	: MyNamespace(mynamespace),
	  Session(session)
{
}

//
// Determine if the given operator is a unary prefix
//
bool OperatorTable::PrefixExists(StringHandle operatorname) const
{
	return Session.Identifiers.UnaryPrefixes.find(MyNamespace.Strings.GetPooledString(operatorname)) != Session.Identifiers.UnaryPrefixes.end();
}

//
// Retrieve the operator precedence of the given operator
//
int OperatorTable::GetPrecedence(StringHandle operatorname) const
{
	return Session.OperatorPrecedences.find(operatorname)->second;
}


// Evil: disable warning about "*this" in initializer lists
#pragma warning(push)
#pragma warning(disable: 4355)

//
// Construct an initialize a namespace wrapper
//
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

// End evil
#pragma warning(pop)


//
// Destruct and clean up a namespace wrapper
//
Namespace::~Namespace()
{
	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
		delete *iter;

	delete GlobalScope;
}

//
// Add an unnamed lexical scope to the namespace metadata
//
void Namespace::AddScope(ScopeDescription* scope)
{
	StringHandle name = LexicalScopeNameCache.Find(scope);
	if(!name)
		name = AllocateLexicalScopeName(scope);

	AddScope(scope, name);
}

//
// Add a named lexical scope to the namespace metadata
//
void Namespace::AddScope(ScopeDescription* scope, StringHandle name)
{
	if(Parent)
	{
		Parent->AddScope(scope, name);
		return;
	}

	if(LexicalScopes.find(name) != LexicalScopes.end())
		throw InternalException("Stomped on a lexical scope");

	LexicalScopes.insert(std::make_pair(name, scope));
	LexicalScopeNameCache.Add(scope, name);
}

//
// Allocate a name for an otherwise anonymous lexical scope
//
StringHandle Namespace::AllocateLexicalScopeName(const ScopeDescription* scopeptr)
{
	if(Parent)
		return Parent->AllocateLexicalScopeName(scopeptr);

	return Strings.PoolFast(GenerateLexicalScopeName(scopeptr));
}

//
// Retrieve the name of a lexical scope bound to a code block
//
// Predominantly useful for code generation
//
StringHandle Namespace::FindLexicalScopeName(const CodeBlock* blockptr) const
{
	return FindLexicalScopeName(blockptr->GetScope());
}

//
// Retrieve the name of a lexical scope (general version)
//
// Predominantly useful for code generation
//
StringHandle Namespace::FindLexicalScopeName(const ScopeDescription* scopeptr) const
{
	return LexicalScopeNameCache.Find(scopeptr);
}

//
// Internal helper for generating a lexical scope name
//
std::wstring Namespace::GenerateLexicalScopeName(const ScopeDescription* scopeptr)
{
	std::wostringstream name;
	name << L"@@scope@" << scopeptr;
	return name.str();
}

//
// Retrieve the "global" scope of this namespace
//
ScopeDescription* Namespace::GetGlobalScope() const
{
	return GlobalScope;
}

//
// Helper for locating entity tag IDs
//
// Entity tags are used to identify different sorts
// of entity in the runtime layer.
//
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

//
// Retrieve the entity tag of a postfix entity
//
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
// Add a global code block to a namespace
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
// Retrieve a given global code block from the namespace
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
// Retrieve the count of global code blocks in a given namespace
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

//
// Internal helper for generating names for otherwise anonymous "global" blocks
//
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

//
// Validate all content in a namespace
//
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

//
// Perform type inference on all content in a namespace
//
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

//
// Perform compile time code execution for all content in a namespace
//
bool Namespace::CompileTimeCodeExecution(CompileErrors& errors)
{
	if(!Types.CompileTimeCodeExecution(errors))
		return false;

	return Functions.CompileTimeCodeExecution(errors);
}

//
// Create a dummy namespace for aiding in template generation
//
// Template arguments create weak type aliases for their mapped types.
// In order to simplify lookup of templated types, we simply create a
// namespace holding these mappings and attach it to the function's IR.
//
Namespace* Namespace::CreateTemplateDummy(Namespace& parent, const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args)
{
	Namespace* ret = new Namespace(parent.Types.IDSpace, parent.Strings, parent.Session);
	ret->SetParent(&parent);

	if(params.size() != args.size())
		throw InternalException("Template parameter/argument mismatch");

	for(size_t i = 0; i < params.size(); ++i)
	{
		if(params[i].second == Metadata::EpochType_Wildcard)
		{
			ret->Types.Aliases.AddWeakAlias(params[i].first, parent.Types.GetTypeByName(static_cast<StringHandle>(args[i].Payload.IntegerValue)));
		}
	}

	return ret;
}

//
// Link a namespace to its owning "parent"
//
// Parents are often containing namespaces, but can also
// be used for tracking ownership of dummy namespaces.
//
void Namespace::SetParent(Namespace* parent)
{
	Parent = parent;
}


//
// Determine if the given identifier would cause shadowing in the current namespace
//
bool Namespace::ShadowingCheck(StringHandle identifier, CompileErrors& errors)
{
	if(Functions.GetNumOverloads(identifier) != 0)
	{
		errors.SemanticError("Identifier already used for a function in this namespace");
		return true;
	}

	if(Types.GetTypeByName(identifier) != Metadata::EpochType_Error)
	{
		errors.SemanticError("Identifier already used for a type in this namespace");
		return true;
	}
	
	if(Parent)
		return Parent->ShadowingCheck(identifier, errors);

	return false;
}


void FunctionTable::GenerateSumTypeFunctions(Metadata::EpochTypeID sumtypeid, const std::set<StringHandle>& basetypenames)
{
	StringHandle sumtypeconstructorname = 0;
	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator niter = MyNamespace.Types.SumTypes.NameToTypeMap.begin(); niter != MyNamespace.Types.SumTypes.NameToTypeMap.end(); ++niter)
	{
		if(niter->second == sumtypeid)
		{
			sumtypeconstructorname = niter->first;
			break;
		}
	}

	if(!sumtypeconstructorname)
		throw InternalException("Missing sum type name mapping");

	std::wostringstream overloadnamebuilder;
	overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << sumtypeid;
	StringHandle overloadname = MyNamespace.Strings.Pool(overloadnamebuilder.str());
	MyNamespace.Session.FunctionOverloadNames[sumtypeconstructorname].insert(overloadname);

	FunctionSignature signature;
	signature.AddParameter(L"@id", Metadata::EpochType_Identifier, false);
	signature.AddParameter(L"@value", sumtypeid, false);

	MyNamespace.Session.CompileTimeHelpers.insert(std::make_pair(sumtypeconstructorname, &CompileConstructorHelper));
	MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, signature));
	MyNamespace.Types.SumTypes.NameToConstructorMap[overloadname] = sumtypeconstructorname;

	for(std::set<StringHandle>::const_iterator btiter = basetypenames.begin(); btiter != basetypenames.end(); ++btiter)
	{
		StringHandle basetypename = *btiter;

		std::wostringstream overloadnamebuilder;
		overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << MyNamespace.Strings.GetPooledString(basetypename);
		StringHandle sumtypeconstructorname = 0;
		for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = MyNamespace.Types.SumTypes.NameToTypeMap.begin(); iter != MyNamespace.Types.SumTypes.NameToTypeMap.end(); ++iter)
		{
			if(iter->second == sumtypeid)
			{
				sumtypeconstructorname = iter->first;
				break;
			}
		}

		if(!sumtypeconstructorname)
			throw InternalException("Missing sum type name mapping");

		StringHandle overloadname = MyNamespace.Strings.Pool(overloadnamebuilder.str());
		MyNamespace.Session.FunctionOverloadNames[sumtypeconstructorname].insert(overloadname);

		Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(MyNamespace.Types.GetTypeByName(basetypename));
		switch(family)
		{
		case Metadata::EpochTypeFamily_Magic:
		case Metadata::EpochTypeFamily_Primitive:
		case Metadata::EpochTypeFamily_GC:
			// No adjustment needed
			break;

		case Metadata::EpochTypeFamily_Structure:
			basetypename = MyNamespace.Types.Structures.GetConstructorName(basetypename);
			break;

		case Metadata::EpochTypeFamily_TemplateInstance:
			basetypename = MyNamespace.Types.Templates.FindConstructorName(basetypename);
			break;

		default:
			throw NotImplementedException("Sum types with this base type not supported");
		}

		MyNamespace.Session.CompileTimeHelpers.insert(std::make_pair(overloadname, MyNamespace.Session.CompileTimeHelpers.find(basetypename)->second));

		if(MyNamespace.Types.GetTypeByName(basetypename) == Metadata::EpochType_Nothing)
		{
			FunctionSignature signature;
			signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
			signature.AddParameter(L"nothing", Metadata::EpochType_Nothing, false);

			MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, signature));
		}
		else
			MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, MyNamespace.Session.FunctionSignatures.find(basetypename)->second));

		MyNamespace.Types.SumTypes.NameToConstructorMap[overloadname] = basetypename;
	}
}