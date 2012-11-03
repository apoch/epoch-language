//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"

#include "Utility/StringPool.h"

#include <sstream>


using namespace IRSemantics;


//
// Construct and initialize a program
//
Program::Program(StringPoolManager& strings, CompileSession& session)
	: CounterAnonParam(0),
	  CounterLexicalScope(0),
	  Strings(strings),
	  StructureTypeCounter(VM::EpochTypeFamily_Structure),
	  Session(session),
	  GlobalScope(new ScopeDescription()),
	  CounterUnitTypeIDs(0),
	  CounterSumTypeIDs(0)
{
}

//
// Destruct and clean up a program
//
Program::~Program()
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
		delete iter->second;

	for(std::map<StringHandle, Structure*>::iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
		delete iter->second;

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
		delete *iter;

	delete GlobalScope;
}


//
// Add a string to the program's string pool
//
StringHandle Program::AddString(const std::wstring& str)
{
	StringHandle ret = IdentifierCache.Find(str);
	if(!ret)
	{
		ret = Strings.Pool(str);
		IdentifierCache.Add(str, ret);
	}
	
	return ret;
}

StringHandle Program::FindString(const std::wstring& str) const
{
	StringHandle cached = IdentifierCache.Find(str);
	if(cached)
		return cached;

	return Strings.Find(str);
}

const std::wstring& Program::GetString(StringHandle handle) const
{
	return Strings.GetPooledString(handle);
}


//
// Add a structure definition to the program
//
void Program::AddStructure(StringHandle name, Structure* structure, CompileErrors& errors)
{
	if(Structures.find(name) != Structures.end())
	{
		delete structure;
		errors.SemanticError("Duplicate structure name");
		return;
	}

	Structures.insert(std::make_pair(name, structure));
	StructureTypes.insert(std::make_pair(name, ++StructureTypeCounter));

	const std::wstring& structurename = GetString(name);
	const std::vector<std::pair<StringHandle, StructureMember*> >& members = structure->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
	{
		std::wstring overloadname = GenerateStructureMemberAccessOverloadName(structurename, GetString(iter->first));
		StructureMemberAccessOverloadNameCache.Add(std::make_pair(name, iter->first), Strings.PoolFast(overloadname));
	}
}


//
// Allocate a function overload mangled name
//
StringHandle Program::CreateFunctionOverload(const std::wstring& name)
{
	StringHandle handle = AddString(name);
	unsigned counter = FunctionOverloadCounters[handle]++;
	StringHandle ret = AddString(GenerateFunctionOverloadName(handle, counter));
	FunctionOverloadNameCache.Add(std::make_pair(handle, counter), ret);
	return ret;
}

unsigned Program::GetNumFunctionOverloads(StringHandle name) const
{
	boost::unordered_map<StringHandle, unsigned>::const_iterator iter = FunctionOverloadCounters.find(name);
	if(iter == FunctionOverloadCounters.end())
		return 0;

	return iter->second;
}

StringHandle Program::GetFunctionOverloadName(StringHandle rawname, unsigned overloadindex) const
{
	return FunctionOverloadNameCache.Find(std::make_pair(rawname, overloadindex));
}

std::wstring Program::GenerateFunctionOverloadName(StringHandle name, size_t index) const
{
	if(index == 0)
		return GetString(name);

	std::wostringstream format;
	format << GetString(name) << L"@@overload@" << index;

	return format.str();
}


//
// Add a function definition to the program
//
void Program::AddFunction(StringHandle name, StringHandle rawname, Function* function, CompileErrors& errors)
{
	if(Functions.find(name) != Functions.end())
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

	for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(iter->second->GetRawName() != rawname)
			continue;

		if(iter->second->GetFunctionSignature(*this).Matches(function->GetFunctionSignature(*this)))
		{
			errors.SemanticError("Ambiguous function overload");
			break;
		}
	}

	function->SetRawName(rawname);
	Functions.insert(std::make_pair(name, function));
}

//
// Determine if the program contains a function by the given name
//
bool Program::HasFunction(StringHandle name) const
{
	return (Functions.find(name) != Functions.end());
}

//
// Add a global code block to a program
//
size_t Program::AddGlobalCodeBlock(CodeBlock* code)
{
	size_t index = GlobalCodeBlocks.size();
	AddString(GenerateAnonymousGlobalScopeName(index));
	GlobalCodeBlocks.push_back(code);
	AddScope(code->GetScope());
	return index;
}

//
// Retrieve a given global code block from the program
//
const CodeBlock& Program::GetGlobalCodeBlock(size_t index) const
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
size_t Program::GetNumGlobalCodeBlocks() const
{
	return GlobalCodeBlocks.size();
}

//
// Retrieve the name of a given global code block
//
StringHandle Program::GetGlobalCodeBlockName(size_t index) const
{
	return Strings.Find(GenerateAnonymousGlobalScopeName(index));
}

//
// Validate a program
//
bool Program::Validate(CompileErrors& errors) const
{
	bool valid = true;

	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypeBaseTypeNames.begin(); iter != SumTypeBaseTypeNames.end(); ++iter)
	{
		for(std::set<StringHandle>::const_iterator setiter = iter->second.begin(); setiter != iter->second.end(); ++setiter)
		{
			if(LookupType(*setiter) == VM::EpochType_Error)
			{
				// TODO - set context
				errors.SemanticError("Base type not defined");
				valid = false;
			}
		}
	}

	for(std::map<StringHandle, Structure*>::const_iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
	{
		if(!iter->second->Validate(*this, errors))
			valid = false;
	}

	for(std::vector<CodeBlock*>::const_iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->Validate(*this))
			valid = false;
	}

	for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->Validate(*this))
			valid = false;
	}

	try
	{
		StringHandle entrypointhandle = Strings.Find(L"entrypoint");
		if(Functions.find(entrypointhandle) == Functions.end())
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

bool Program::TypeInference(CompileErrors& errors)
{
	InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->TypeInference(*this, context, errors))
			return false;
	}

	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->TypeInference(*this, context, errors))
			return false;
	}

	return true;
}

bool Program::CompileTimeCodeExecution(CompileErrors& errors)
{
	for(std::map<StringHandle, Structure*>::iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(iter->first, *this, errors))
			return false;
	}

	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypeBaseTypeNames.begin(); iter != SumTypeBaseTypeNames.end(); ++iter)
	{
		VM::EpochTypeID sumtypeid = iter->first;

		// TODO - refactor a bit
		//////////////////////////////////////
		std::wostringstream overloadnamebuilder;
		overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << sumtypeid;
		StringHandle sumtypeconstructorname = 0;
		for(std::map<StringHandle, VM::EpochTypeID>::const_iterator niter = SumTypeNames.begin(); niter != SumTypeNames.end(); ++niter)
		{
			if(niter->second == sumtypeid)
			{
				sumtypeconstructorname = niter->first;
				break;
			}
		}

		if(!sumtypeconstructorname)
			throw InternalException("Missing sum type name mapping");

		StringHandle overloadname = AddString(overloadnamebuilder.str());
		Session.FunctionOverloadNames[sumtypeconstructorname].insert(overloadname);

		FunctionSignature signature;
		signature.AddParameter(L"@id", VM::EpochType_Identifier, false);
		signature.AddParameter(L"@value", sumtypeid, false);

		// TODO - evil hackery here
		Session.CompileTimeHelpers.insert(std::make_pair(sumtypeconstructorname, Session.CompileTimeHelpers.find(FindString(L"integer"))->second));
		Session.FunctionSignatures.insert(std::make_pair(overloadname, signature));
		SumTypeConstructorNames[overloadname] = sumtypeconstructorname;
		//////////////////////////////////////

		for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
		{
			StringHandle basetypename = *btiter;

			std::wostringstream overloadnamebuilder;
			overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << GetString(basetypename);
			StringHandle sumtypeconstructorname = 0;
			for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = SumTypeNames.begin(); iter != SumTypeNames.end(); ++iter)
			{
				if(iter->second == sumtypeid)
				{
					sumtypeconstructorname = iter->first;
					break;
				}
			}

			if(!sumtypeconstructorname)
				throw InternalException("Missing sum type name mapping");

			StringHandle overloadname = AddString(overloadnamebuilder.str());
			Session.FunctionOverloadNames[sumtypeconstructorname].insert(overloadname);

			Session.CompileTimeHelpers.insert(std::make_pair(overloadname, Session.CompileTimeHelpers.find(basetypename)->second));
			Session.FunctionSignatures.insert(std::make_pair(overloadname, Session.FunctionSignatures.find(basetypename)->second));
			SumTypeConstructorNames[overloadname] = basetypename;
		}
	}

	// TODO - re-enable global stuffs
	/*for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(*this, errors))
			return false;
	}*/

	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(*this, errors))
			return false;
	}

	for(std::multimap<StringHandle, StringHandle>::const_iterator iter = FunctionsWithStaticPatternMatching.begin(); iter != FunctionsWithStaticPatternMatching.end(); ++iter)
	{
		StringHandle rawname = iter->first;
		StringHandle overloadname = iter->second;

		const Function* function = Functions.find(overloadname)->second;
		const FunctionSignature functionsignature = function->GetFunctionSignature(*this);
		for(size_t i = 0; i < FunctionOverloadCounters[rawname]; ++i)
		{
			StringHandle thisoverloadname = GetFunctionOverloadName(rawname, i);
			if(thisoverloadname == overloadname)
				continue;

			const Function* overload = Functions.find(thisoverloadname)->second;
			if(functionsignature.MatchesDynamicPattern(overload->GetFunctionSignature(*this)))
				FunctionsNeedingDynamicPatternMatching.insert(thisoverloadname);
		}
	}

	for(std::set<StringHandle>::const_iterator iter = FunctionsNeedingDynamicPatternMatching.begin(); iter != FunctionsNeedingDynamicPatternMatching.end(); ++iter)
	{
		StringHandle funcname = *iter;
		const std::wstring& funcnamestr = GetString(funcname);
		std::wstring patternmatchername = GeneratePatternMatcherName(funcnamestr);
		StringHandle pmnhandle = AddString(patternmatchername);
		PatternMatcherNameCache.Add(funcname, pmnhandle);
	}

	return true;
}


//
// Allocate a name for an anonymous function parameter (usually an expression)
//
StringHandle Program::AllocateAnonymousParamName()
{
	std::wostringstream name;
	name << L"@@anonparam@" << (++CounterAnonParam);
	return Strings.PoolFast(name.str());
}

std::wstring Program::GenerateAnonymousGlobalScopeName(size_t index)
{
	std::wostringstream name;
	name << L"@@globalscope@" << index;
	return name.str();
}


StringHandle Program::AllocateLexicalScopeName(const CodeBlock* blockptr)
{
	StringHandle ret = Strings.PoolFast(GenerateLexicalScopeName(blockptr->GetScope()));
	LexicalScopeNameCache.Add(blockptr->GetScope(), ret);
	return ret;
}

StringHandle Program::FindLexicalScopeName(const CodeBlock* blockptr) const
{
	return LexicalScopeNameCache.Find(blockptr->GetScope());
}

StringHandle Program::FindLexicalScopeName(const ScopeDescription* scopeptr) const
{
	return LexicalScopeNameCache.Find(scopeptr);
}

std::wstring Program::GenerateLexicalScopeName(const ScopeDescription* scopeptr)
{
	std::wostringstream name;
	name << L"@@scope@" << scopeptr;
	return name.str();
}


VM::EpochTypeID Program::LookupType(StringHandle name) const
{
	// TODO - replace this with a less hard-coded solution
	const std::wstring& type = Strings.GetPooledString(name);

	if(type == L"integer")
		return VM::EpochType_Integer;
	else if(type == L"integer16")
		return VM::EpochType_Integer16;
	else if(type == L"string")
		return VM::EpochType_String;
	else if(type == L"boolean")
		return VM::EpochType_Boolean;
	else if(type == L"real")
		return VM::EpochType_Real;
	else if(type == L"buffer")
		return VM::EpochType_Buffer;
	else if(type == L"identifier")
		return VM::EpochType_Identifier;
	else if(type == L"nothing")
		return VM::EpochType_Nothing;

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StructureTypes.find(name);
		if(iter != StructureTypes.end())
			return iter->second;
	}
	
	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = TypeAliases.find(name);
		if(iter != TypeAliases.end())
			return iter->second;
	}

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StrongTypeAliasTypes.find(name);
		if(iter != StrongTypeAliasTypes.end())
			return iter->second;
	}

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = SumTypeNames.find(name);
		if(iter != SumTypeNames.end())
			return iter->second;
	}

	{
		std::map<StringHandle, StringHandle>::const_iterator iter = SumTypeConstructorNames.find(name);
		if(iter != SumTypeConstructorNames.end())
			return LookupType(iter->second);
	}

	return VM::EpochType_Error;
}


VM::EpochTypeID Program::GetStructureMemberType(StringHandle structurename, StringHandle membername) const
{
	std::map<StringHandle, Structure*>::const_iterator iter = Structures.find(structurename);
	if(iter == Structures.end())
	{
		//
		// This should occur only when something has gone horribly
		// wrong in the parsing system, i.e. there is a logic bug
		// in the compiler implementation itself.
		//
		// Essentially this means someone thought they had a token
		// that was bound to a structure type definition, but either
		// it never was, or we forgot the type definition metadata
		// somehow during compilation.
		//
		throw InternalException("String handle does not correspond to any known structure type");
	}

	const std::vector<std::pair<StringHandle, StructureMember*> >& members = iter->second->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
	{
		if(memberiter->first == membername)
			return memberiter->second->GetEpochType(*this);
	}
	
	//
	// This is also limited to logic bugs in the compiler.
	//
	// We should not attempt to access a structure member's type
	// if the structure has no members by that name. Semantic
	// checking should have first determined that the member does
	// not exist, prior to ever asking for the type information.
	//
	// Further, semantic checking should have failed once the
	// missing member was detected, preventing any further type
	// validation surrounding the faulty expression.
	//
	throw InternalException("Structure type does not provide any members by this name");
}

StringHandle Program::GetNameOfStructureType(VM::EpochTypeID structuretype) const
{
	for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StructureTypes.begin(); iter != StructureTypes.end(); ++iter)
	{
		if(iter->second == structuretype)
			return iter->first;
	}

	//
	// This catches a potential logic bug in the compiler implementation.
	//
	// If a structure has been allocated a type ID, that type ID should never
	// be able to exist without being also bound to an identifier, even if that
	// identifier is just an automatically generated magic anonymous token.
	//
	// Callers cannot be faulted directly for this exception; the problem most
	// likely lies elsewhere in the structure type handling code.
	//
	throw InternalException("Type ID is not bound to any known identifier");
}

StringHandle Program::FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const
{
	return StructureMemberAccessOverloadNameCache.Find(std::make_pair(structurename, membername));
}

std::wstring Program::GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername)
{
	return L".@@" + structurename + L"@@" + membername;
}


Bytecode::EntityTag Program::GetEntityTag(StringHandle identifier) const
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

Bytecode::EntityTag Program::GetEntityCloserTag(StringHandle identifier) const
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


ScopeDescription* Program::GetGlobalScope() const
{
	return GlobalScope;
}


InferenceContext::PossibleParameterTypes Program::GetExpectedTypesForStatement(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors&) const
{
	if(scope.HasVariable(name) && scope.GetVariableTypeByID(name) == VM::EpochType_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = Functions.find(contextname);
		if(funciter == Functions.end())
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

		FunctionSignature sig = funciter->second->GetParameterSignature(name, *this);
		for(size_t i = 0; i < sig.GetNumParameters(); ++i)
			ret.back().push_back(sig.GetParameter(i).Type);
	
		return ret;
	}

	boost::unordered_map<StringHandle, unsigned>::const_iterator iter = FunctionOverloadCounters.find(name);
	if(iter != FunctionOverloadCounters.end())
	{
		InferenceContext::PossibleParameterTypes ret;
		ret.reserve(iter->second);

		for(size_t i = 0; i < iter->second; ++i)
		{
			StringHandle overloadname = i ? FunctionOverloadNameCache.Find(std::make_pair(name, i)) : name;
			FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(overloadname);
			if(fsigiter != Session.FunctionSignatures.end())
			{
				ret.push_back(InferenceContext::TypePossibilities());

				for(size_t j = 0; j < fsigiter->second.GetNumParameters(); ++j)
					ret.back().push_back(fsigiter->second.GetParameter(j).Type);
			}
			else
			{
				boost::unordered_map<StringHandle, Function*>::const_iterator funciter = Functions.find(overloadname);
				if(funciter != Functions.end())
				{
					ret.push_back(InferenceContext::TypePossibilities());
					FunctionSignature signature = funciter->second->GetFunctionSignature(*this);

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

	//errors.SemanticError("Undefined function");
	return InferenceContext::PossibleParameterTypes();
}

InferenceContext::PossibleSignatureSet Program::GetExpectedSignaturesForStatement(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors&) const
{
	if(scope.HasVariable(name) && scope.GetVariableTypeByID(name) == VM::EpochType_Function)
	{
		boost::unordered_map<StringHandle, Function*>::const_iterator funciter = Functions.find(contextname);
		if(funciter == Functions.end())
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
		ret.back().push_back(funciter->second->GetParameterSignature(name, *this));

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
			boost::unordered_map<StringHandle, Function*>::const_iterator funciter = Functions.find(overloadname);
			if(funciter != Functions.end())
			{
				ret.push_back(InferenceContext::SignaturePossibilities());

				std::vector<StringHandle> paramnames = funciter->second->GetParameterNames();
				for(std::vector<StringHandle>::const_iterator paramiter = paramnames.begin(); paramiter != paramnames.end(); ++paramiter)
					ret.back().push_back(funciter->second->GetParameterSignature(*paramiter, *this));
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

	//errors.SemanticError("Undefined function");
	return InferenceContext::PossibleSignatureSet();
}


void Program::AddScope(ScopeDescription* scope)
{
	AddScope(scope, LexicalScopeNameCache.Find(scope));
}

void Program::AddScope(ScopeDescription* scope, StringHandle name)
{
	LexicalScopes.insert(std::make_pair(name, scope));
}

void Program::MarkFunctionWithStaticPatternMatching(StringHandle rawname, StringHandle overloadname)
{
	FunctionsWithStaticPatternMatching.insert(std::make_pair(rawname, overloadname));
}

bool Program::FunctionNeedsDynamicPatternMatching(StringHandle overloadname) const
{
	return (FunctionsNeedingDynamicPatternMatching.count(overloadname) > 0);
}

StringHandle Program::GetDynamicPatternMatcherForFunction(StringHandle overloadname) const
{
	return PatternMatcherNameCache.Find(overloadname);
}

std::wstring Program::GeneratePatternMatcherName(const std::wstring& funcname)
{
	return funcname + L"@@patternmatch";
}


VM::EpochTypeID Program::AddSumType(const std::wstring& name, CompileErrors& errors)
{
	StringHandle namehandle = AddString(name);

	if(LookupType(namehandle) != VM::EpochType_Error)
		errors.SemanticError("Type name already in use");

	VM::EpochTypeID type = (++CounterSumTypeIDs) | VM::EpochTypeFamily_SumType;
	SumTypeNames[namehandle] = type;

	return type;
}

void Program::AddSumTypeBase(VM::EpochTypeID sumtypeid, StringHandle basetypename)
{
	SumTypeBaseTypeNames[sumtypeid].insert(basetypename);
}

StringHandle Program::MapConstructorNameForSumType(StringHandle sumtypeoverloadname)
{
	std::map<StringHandle, StringHandle>::const_iterator iter = SumTypeConstructorNames.find(sumtypeoverloadname);
	if(iter == SumTypeConstructorNames.end())
		return sumtypeoverloadname;

	return iter->second;
}


bool Program::SumTypeHasTypeAsBase(VM::EpochTypeID sumtypeid, VM::EpochTypeID basetype) const
{
	std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypeBaseTypeNames.find(sumtypeid);
	for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
	{
		if(LookupType(*btiter) == basetype)
			return true;
	}

	return false;
}


StringHandle Program::AllocateTypeMatcher(StringHandle overloadname, const std::map<StringHandle, FunctionSignature>& matchingoverloads)
{
	if(OverloadTypeMatchers.find(overloadname) != OverloadTypeMatchers.end())
		return OverloadTypeMatchers.find(overloadname)->second;

	StringHandle matchername = AddString(GetString(overloadname) + L"@@typematcher");
	OverloadTypeMatchers[overloadname] = matchername;

	RequiredTypeMatchers.insert(std::make_pair(matchername, matchingoverloads));

	return matchername;
}

std::map<VM::EpochTypeID, std::set<VM::EpochTypeID> > Program::GetSumTypes() const
{
	std::map<VM::EpochTypeID, std::set<VM::EpochTypeID> > ret;

	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypeBaseTypeNames.begin(); iter != SumTypeBaseTypeNames.end(); ++iter)
	{
		for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
			ret[iter->first].insert(LookupType(*btiter));
	}

	return ret;
}


unsigned Program::FindMatchingFunctions(StringHandle identifier, const FunctionSignature& expectedsignature, InferenceContext& context, CompileErrors& errors, StringHandle& resolvedidentifier)
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
	else if(HasFunction(identifier))
	{
		unsigned overloadcount = GetNumFunctionOverloads(identifier);
		for(unsigned j = 0; j < overloadcount; ++j)
		{
			foundidentifier = true;
			StringHandle overloadname = GetFunctionOverloadName(identifier, j);
			Function* func = GetFunctions().find(overloadname)->second;

			func->TypeInference(*this, context, errors);
			FunctionSignature sig = func->GetFunctionSignature(*this);
			if(expectedsignature.Matches(sig))
			{
				resolvedidentifier = overloadname;
				++signaturematches;
			}
		}
	}

	return signaturematches;
}

