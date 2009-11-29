//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Metadata for tracking a lexical scope.
//
// Note that this data is more like a template than an actual lexical
// scope container; the container itself is ActivatedScope. See the
// class implementation of ActivatedScope for full details.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Types/Tuple.h"
#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Core Entities/Variables/ListVariable.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"
#include "Virtual Machine/Core Entities/Concurrency/Future.h"

#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/StackOps.h"

#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Virtual Machine/VMExceptions.h"

#include "Utility/Memory/Stack.h"
#include "Utility/Memory/Heap.h"
#include "Utility/Strings.h"


using namespace VM;


//-------------------------------------------------------------------------------
// Construction and destruction
//-------------------------------------------------------------------------------

//
// Construct and initialize a lexical scope template
//
ScopeDescription::ScopeDescription()
	: ParentScope(NULL)
{
}

//
// Construct a copy of a lexical scope template
//
ScopeDescription::ScopeDescription(const ScopeDescription&)
{
	throw Exception("Scope descriptions should not be copied!");
}

//
// Destruct and clean up a lexical scope template
//
ScopeDescription::~ScopeDescription()
{
	for(FunctionMap::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
		delete iter->second;

	for(ResponseMapList::iterator iter = ResponseMaps.begin(); iter != ResponseMaps.end(); ++iter)
		delete iter->second;

	for(FutureMap::iterator iter = Futures.begin(); iter != Futures.end(); ++iter)
		delete iter->second;
}


//-------------------------------------------------------------------------------
// Variable addition interface
//-------------------------------------------------------------------------------

//
// Add a variable to the scope
//
void ScopeDescription::AddVariable(const std::wstring& name, EpochVariableTypeID type)
{
	CheckForDuplicateIdentifier(name);

	MemberOrder.push_back(name);

	switch(type)
	{
	case EpochVariableType_Integer:
		Variables.insert(VariableMapEntry(name, IntegerVariable(NULL)));
		break;
	case EpochVariableType_Integer16:
		Variables.insert(VariableMapEntry(name, Integer16Variable(NULL)));
		break;
	case EpochVariableType_Boolean:
		Variables.insert(VariableMapEntry(name, BooleanVariable(NULL)));
		break;
	case EpochVariableType_Real:
		Variables.insert(VariableMapEntry(name, RealVariable(NULL)));
		break;
	case EpochVariableType_String:
		Variables.insert(VariableMapEntry(name, StringVariable(NULL)));
		break;
	case EpochVariableType_Function:
		Variables.insert(VariableMapEntry(name, FunctionBinding(NULL)));
		break;
	case EpochVariableType_Buffer:
		Variables.insert(VariableMapEntry(name, BufferVariable(NULL)));
		break;
	case EpochVariableType_List:
		Variables.insert(VariableMapEntry(name, ListVariable(NULL)));
		break;
	default:
		throw NotImplementedException("Cannot add variable to scope - type not recognized");
	}
}

//
// Add a variable of a custom tuple type to the scope
//
void ScopeDescription::AddTupleVariable(const std::wstring& tupletypename, const std::wstring& name)
{
	CheckForDuplicateIdentifier(name);

	MemberOrder.push_back(name);
	Variables.insert(VariableMapEntry(name, TupleVariable(NULL)));
	TupleTypeHints.insert(TupleTypeIDMapEntry(name, GetTupleTypeID(tupletypename)));
}

//
// Add a variable of a custom structure type to the scope
//
void ScopeDescription::AddStructureVariable(const std::wstring& structuretypename, const std::wstring& name)
{
	CheckForDuplicateIdentifier(name);

	MemberOrder.push_back(name);
	Variables.insert(VariableMapEntry(name, StructureVariable(NULL)));
	StructureTypeHints.insert(StructureTypeIDMapEntry(name, GetStructureTypeID(structuretypename)));
}

void ScopeDescription::AddStructureVariable(IDType structuretypeid, const std::wstring& name)
{
	CheckForDuplicateIdentifier(name);

	MemberOrder.push_back(name);
	Variables.insert(VariableMapEntry(name, StructureVariable(NULL)));
	StructureTypeHints.insert(StructureTypeIDMapEntry(name, structuretypeid));
}


//-------------------------------------------------------------------------------
// References
//-------------------------------------------------------------------------------

//
// Add a reference to the scope (references bind to variables in another scope)
//
void ScopeDescription::AddReference(EpochVariableTypeID type, const std::wstring& name)
{
	CheckForDuplicateIdentifier(name);

	MemberOrder.push_back(name);
	References.insert(VariableRefMapEntry(name, VariableRefDescriptor(type, NULL)));
}

//
// Determine if a given variable is a reference to another scope
//
bool ScopeDescription::IsReference(const std::wstring& name) const
{
	return (References.find(name) != References.end());
}


//-------------------------------------------------------------------------------
// Higher-order functions interface
//-------------------------------------------------------------------------------

//
// Register a function signature; used to validate parameters to higher-order functions
//
void ScopeDescription::AddFunctionSignature(const std::wstring& name, const FunctionSignature& signature, bool insertmember)
{
	if(insertmember)
	{
		CheckForDuplicateIdentifier(name);
		MemberOrder.push_back(name);
	}
	FunctionSignatures.insert(FunctionSignatureMapEntry(name, signature));
}

//
// Retrieve the requested function signature
//
const FunctionSignature& ScopeDescription::GetFunctionSignature(const std::wstring& name) const
{
	FunctionSignatureMap::const_iterator iter = FunctionSignatures.find(name);
	if(iter == FunctionSignatures.end())
	{
		if(!Ghosts.empty())
		{
			GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
			if(ghostiter != Ghosts.back().end())
				return ghostiter->second->GetFunctionSignature(name);
		}

		if(ParentScope)
			return ParentScope->GetFunctionSignature(name);

		throw ExecutionException("Failed to find the function signature \"" + narrow(name) + "\"");
	}

	return iter->second;
}

//
// Determine if the given identifier corresponds to a registered function signature
//
bool ScopeDescription::IsFunctionSignature(const std::wstring& name) const
{
	if(FunctionSignatures.find(name) == FunctionSignatures.end())
	{
		if((!Ghosts.empty()) && (Ghosts.back().find(name) != Ghosts.back().end()))
			return true;

		if(ParentScope)
			return ParentScope->IsFunctionSignature(name);

		return false;
	}
	
	return true;
}



//-------------------------------------------------------------------------------
// Futures
//-------------------------------------------------------------------------------

//
// Add a future to the scope so its value can be retrieved later
//
void ScopeDescription::AddFuture(const std::wstring& name, VM::OperationPtr boundop)
{
	CheckForDuplicateIdentifier(name);

	Futures.insert(std::make_pair(name, new Future(boundop)));
}


//-------------------------------------------------------------------------------
// Lists
//-------------------------------------------------------------------------------

//
// Track the element type of a list variable; this is primarily used for empty
// lists that cannot use type inference on their elements.
//
void ScopeDescription::SetListType(const std::wstring& listname, EpochVariableTypeID type)
{
	ListTypes[listname] = type;
}

//
// Retrieve the element type of a list variable
//
EpochVariableTypeID ScopeDescription::GetListType(const std::wstring& listname) const
{
	std::map<std::wstring, EpochVariableTypeID>::const_iterator iter = ListTypes.find(listname);
	if(iter == ListTypes.end())
		throw ExecutionException("Could not determine element type of this list");

	return iter->second;
}

//
// Track the expected size of a list
//
void ScopeDescription::SetListSize(const std::wstring& listname, size_t size)
{
	ListSizes[listname] = size;
}

//
// Retrieve the expected size of a list. Used for initialization of unbound list variables.
//
size_t ScopeDescription::GetListSize(const std::wstring& listname) const
{
	std::map<std::wstring, unsigned>::const_iterator iter = ListSizes.find(listname);
	if(iter == ListSizes.end())
		throw ExecutionException("Could not determine size of this list");

	return iter->second;
}




//-------------------------------------------------------------------------------
// Generic variable information retrieval
//-------------------------------------------------------------------------------

//
// Retrieve the type of the given variable
//
EpochVariableTypeID ScopeDescription::GetVariableType(const std::wstring& name) const
{
	VariableRefMap::const_iterator refiter = References.find(name);
	if(refiter != References.end())
		return refiter->second.first;

	FutureMap::const_iterator futiter = Futures.find(name);
	if(futiter != Futures.end())
		return futiter->second->GetType(*this);

	FunctionSignatureMap::const_iterator fsigiter = FunctionSignatures.find(name);
	if(fsigiter != FunctionSignatures.end())
		return EpochVariableType_Function;

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator iter = Ghosts.back().find(name);
		if(iter != Ghosts.back().end())
			return iter->second->GetVariableType(name);
	}

	return LookupVariable(name).GetType();
}


//
// Flag a variable as a constant
//
void ScopeDescription::SetConstant(const std::wstring& name)
{
	std::remove(Constants.begin(), Constants.end(), name);
	Constants.push_back(name);
}

//
// Determine if a given variable is a constant
//
bool ScopeDescription::IsConstant(const std::wstring& name) const
{
	if(std::find(Constants.begin(), Constants.end(), name) != Constants.end())
		return true;

	if(ParentScope)
		return ParentScope->IsConstant(name);

	return false;
}

//
// Locate which scope, if any, holds the given variable.
//
const ScopeDescription* ScopeDescription::GetScopeOwningVariable(const std::wstring& name) const
{
	if(Variables.find(name) != Variables.end())
		return this;

	if(ParentScope)
		return ParentScope->GetScopeOwningVariable(name);

	return NULL;
}


//-------------------------------------------------------------------------------
// Ghost references
//-------------------------------------------------------------------------------

//
// Set up a scope ghost set. See ActivatedScope::GhostIntoScope for details on ghosting.
//
void ScopeDescription::GhostIntoScope(ScopeDescription& other) const
{
	if(other.Ghosts.empty())
		throw InternalFailureException("Must PushNewGhostSet() before calling GhostIntoScope()");

	for(VariableMap::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		other.CheckForDuplicateIdentifier(iter->first);
		other.Ghosts.back()[iter->first] = const_cast<ScopeDescription*>(this);
	}

	for(VariableRefMap::const_iterator iter = References.begin(); iter != References.end(); ++iter)
	{
		other.CheckForDuplicateIdentifier(iter->first);
		other.Ghosts.back()[iter->first] = const_cast<ScopeDescription*>(this);
	}

	for(FunctionMap::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		other.CheckForDuplicateIdentifier(iter->first);
		other.Ghosts.back()[iter->first] = const_cast<ScopeDescription*>(this);
	}

	for(FunctionSignatureMap::const_iterator iter = FunctionSignatures.begin(); iter != FunctionSignatures.end(); ++iter)
	{
		other.CheckForDuplicateIdentifier(iter->first);
		other.Ghosts.back()[iter->first] = const_cast<ScopeDescription*>(this);
	}
}

//
// Track a new set of ghosts for the current scope. We need to allow
// multiple layers of "current" ghosts in order to properly handle
// recursion and other re-entrant behaviours.
//
void ScopeDescription::PushNewGhostSet()
{
	Ghosts.push_back(GhostVariableMap());
}

//
// Note that a set of ghost links is no longer in use, and restore
// the ghost set (if any) that was active previously.
//
void ScopeDescription::PopGhostSet()
{
	Ghosts.pop_back();
}


std::set<const ScopeDescription*> ScopeDescription::GetAllGhostScopes() const
{
	std::set<const ScopeDescription*> ret;
	if(!Ghosts.empty())
	{
		for(GhostVariableMap::const_iterator iter = Ghosts.back().begin(); iter != Ghosts.back().end(); ++iter)
			ret.insert(iter->second);
	}

	return ret;
}



//-------------------------------------------------------------------------------
// Interface for working with functions
//-------------------------------------------------------------------------------

//
// Add a function to the lexical scope
//
void ScopeDescription::AddFunction(const std::wstring& name, std::auto_ptr<FunctionBase> func)
{
	CheckForDuplicateIdentifier(name);
	Functions.insert(FunctionMapEntry(name, func.release()));
}

//
// Retrieve the function wrapper object with the given function name
//
FunctionBase* ScopeDescription::GetFunction(const std::wstring& name) const
{
	FunctionMap::const_iterator iter = Functions.find(name);
	if(iter != Functions.end())
		return iter->second;

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->GetFunction(name);
	}

	if(ParentScope)
		return ParentScope->GetFunction(name);

	throw ExecutionException("Function not found");
}

//
// Retrieve the return type of the function with the given name
//
EpochVariableTypeID ScopeDescription::GetFunctionType(const std::wstring& name) const
{
	FunctionMap::const_iterator iter = Functions.find(name);
	if(iter != Functions.end())
		return iter->second->GetType(*this);

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->GetFunctionType(name);
	}

	FunctionSignatureMap::const_iterator sigiter = FunctionSignatures.find(name);
	if(sigiter != FunctionSignatures.end())
		return sigiter->second.GetReturnType();

	if(ParentScope)
		return ParentScope->GetFunctionType(name);

	throw ExecutionException("Function not found");
}

//
// Retrieve the name of the function with the given signature
//
const std::wstring& ScopeDescription::GetFunctionName(const FunctionBase* func) const
{
	for(FunctionMap::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(iter->second == func)
			return iter->first;
	}

	if(ParentScope)
		return ParentScope->GetFunctionName(func);

	throw ExecutionException("Function not found");
}



//-------------------------------------------------------------------------------
// Helpers for functions with multiple return values
//-------------------------------------------------------------------------------

//
// Retrieve the effective function return type, assuming that this
// scope represents the function's return value(s).
//
// For multiple return values, a tuple type matching the scope's
// defined variables is returned. For a single return value, the
// value's type is returned directly.
//
EpochVariableTypeID ScopeDescription::GetEffectiveType() const
{
	if(Variables.size() == 1)
		return Variables.begin()->second.GetType();

	return EpochVariableType_Tuple;
}

//
// When returning multiple values from a function, we need to
// register with the function's returns scope that there is a
// matching tuple type so we can retrieve the tuple via the
// GetEffectiveTuple() function.
//
void ScopeDescription::RegisterSelfAsTupleType(const std::wstring& nameoftype)
{
	if(MemberOrder.empty() || MemberOrder.size() == 1)
		return;

	TupleType ttype;

	for(std::vector<std::wstring>::const_iterator iter = MemberOrder.begin(); iter != MemberOrder.end(); ++iter)
	{
		VariableMap::iterator variter = Variables.find(*iter);
		if(variter == Variables.end())
			continue;

		Variable& var = variter->second;
		EpochVariableTypeID vartype = var.GetType();

		if(vartype == EpochVariableType_Tuple || vartype == EpochVariableType_Structure)
			throw NotImplementedException("Nested tuples/structures are not supported in function return tuples");
		else
			ttype.AddMember(*iter, vartype);
	}

	AddTupleType(L"@@reserved-internal:returns:" + nameoftype, ttype);
}



//-------------------------------------------------------------------------------
// Interface for working with tuples
//-------------------------------------------------------------------------------

//
// Add a new tuple type to the scope
//
void ScopeDescription::AddTupleType(const std::wstring& name, const TupleType& typeinfo)
{
	CheckForDuplicateIdentifier(name);

	IDType id = TupleTracker.RegisterTupleType(typeinfo);
	TupleTypes.insert(TupleTypeIDMapEntry(name, id));
}

//
// Retrieve the given tuple type's descriptor data
//
const TupleType& ScopeDescription::GetTupleType(const std::wstring& name) const
{
	TupleTypeIDMap::const_iterator iter = TupleTypes.find(name);
	if(iter == TupleTypes.end())
	{
		if(ParentScope)
			return ParentScope->GetTupleType(name);

		throw ExecutionException("Unrecognized tuple type name");
	}

	return TupleTracker.GetTupleType(iter->second);
}

//
// Retrieve the given tuple type descriptor using its ID
//
const TupleType& ScopeDescription::GetTupleType(IDType tupletypeid) const
{
	return TupleTrackerClass::GetOwnerOfTupleType(tupletypeid)->GetTupleType(tupletypeid);
}

//
// Retrieve the ID of the tuple type's descriptor data
//
IDType ScopeDescription::GetTupleTypeID(const std::wstring& name) const
{
	TupleTypeIDMap::const_iterator iter = TupleTypes.find(name);
	if(iter == TupleTypes.end())
	{
		if(ParentScope)
			return ParentScope->GetTupleTypeID(name);

		throw ExecutionException("Unrecognized tuple type name");
	}

	return iter->second;
}

const std::wstring& ScopeDescription::GetTupleTypeID(IDType id) const
{
	for(TupleTypeIDMap::const_iterator iter = TupleTypes.begin(); iter != TupleTypes.end(); ++iter)
	{
		if(iter->second == id)
			return iter->first;
	}

	if(ParentScope)
		return ParentScope->GetTupleTypeID(id);
		
	throw ExecutionException("Unrecognized tuple type ID");
}

//
// Retrieve the tuple type ID of a given variable
//
IDType ScopeDescription::GetVariableTupleTypeID(const std::wstring& varname) const
{
	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(varname);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->GetVariableTupleTypeID(varname);
	}

	TupleTypeIDMap::const_iterator iter = TupleTypeHints.find(varname);
	if(iter == TupleTypeHints.end())
		throw ExecutionException("Variable is not a tuple or structure");

	return iter->second;
}

//
// Assign the tuple type ID hint of a given variable
//
void ScopeDescription::SetVariableTupleTypeID(const std::wstring& name, IDType id)
{
	TupleTypeHints.insert(TupleTypeIDMapEntry(name, id));
}


//-------------------------------------------------------------------------------
// Interface for working with structures
//-------------------------------------------------------------------------------

//
// Add a new structure type to the scope
//
void ScopeDescription::AddStructureType(const std::wstring& name, const StructureType& typeinfo)
{
	CheckForDuplicateIdentifier(name);

	IDType id = StructureTracker.RegisterStructureType(typeinfo);
	StructureTypes.insert(StructureTypeIDMapEntry(name, id));
}

//
// Retrieve the given structure type's descriptor data
//
const StructureType& ScopeDescription::GetStructureType(const std::wstring& name) const
{
	StructureTypeIDMap::const_iterator iter = StructureTypes.find(name);
	if(iter == StructureTypes.end())
	{
		if(ParentScope)
			return ParentScope->GetStructureType(name);

		throw ExecutionException("Unrecognized structure type name");
	}

	return StructureTracker.GetStructureType(iter->second);
}

//
// Retrieve the given structure type descriptor using its ID
//
const StructureType& ScopeDescription::GetStructureType(IDType structuretypeid) const
{
	return StructureTrackerClass::GetOwnerOfStructureType(structuretypeid)->GetStructureType(structuretypeid);
}

//
// Retrieve the ID of the structure type's descriptor data
//
IDType ScopeDescription::GetStructureTypeID(const std::wstring& name) const
{
	StructureTypeIDMap::const_iterator iter = StructureTypes.find(name);
	if(iter == StructureTypes.end())
	{
		if(ParentScope)
			return ParentScope->GetStructureTypeID(name);

		throw ExecutionException("Unrecognized structure type name");
	}

	return iter->second;
}

const std::wstring& ScopeDescription::GetStructureTypeID(IDType id) const
{
	for(StructureTypeIDMap::const_iterator iter = StructureTypes.begin(); iter != StructureTypes.end(); ++iter)
	{
		if(iter->second == id)
			return iter->first;
	}

	if(ParentScope)
		return ParentScope->GetStructureTypeID(id);
		
	throw InternalFailureException("Unrecognized structure type ID");
}

//
// Retrieve the structure type ID of a given variable
//
IDType ScopeDescription::GetVariableStructureTypeID(const std::wstring& varname) const
{
	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(varname);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->GetVariableStructureTypeID(varname);
	}

	StructureTypeIDMap::const_iterator iter = StructureTypeHints.find(varname);
	if(iter == StructureTypeHints.end())
	{
		if(ParentScope)
			return ParentScope->GetVariableStructureTypeID(varname);
		else
			throw ExecutionException("Variable is not a tuple or structure");
	}

	return iter->second;
}

//
// Assign the structure type ID hint of a given variable
//
void ScopeDescription::SetVariableStructureTypeID(const std::wstring& name, IDType id)
{
	StructureTypeHints.insert(StructureTypeIDMapEntry(name, id));
}



//-------------------------------------------------------------------------------
// Message responses
//-------------------------------------------------------------------------------

//
// Add a message response map to the current scope.
//
// Message reception operations can now use this named map to
// filter and respond to incoming messages.
//
void ScopeDescription::AddResponseMap(const std::wstring& name, VM::ResponseMap* responses)
{
	CheckForDuplicateIdentifier(name);
	ResponseMaps.insert(ResponseMapListEntry(name, responses));
}


//
// Return the requested response map structure
//
const VM::ResponseMap& ScopeDescription::GetResponseMap(const std::wstring& name) const
{
	ResponseMapList::const_iterator iter = ResponseMaps.find(name);
	if(iter == ResponseMaps.end())
	{
		if(ParentScope)
			return ParentScope->GetResponseMap(name);

		throw ExecutionException("Response map is not defined");
	}

	return *(iter->second);
}


//-------------------------------------------------------------------------------
// Internal helpers
//-------------------------------------------------------------------------------

//
// Look up and retrieve a reference to the variable with the given name
//
Variable& ScopeDescription::LookupVariable(const std::wstring& name) const
{
	VariableMap::const_iterator iter = Variables.find(name);
	if(iter != Variables.end())
		return const_cast<Variable&>(iter->second);

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->LookupVariable(name);
	}

	VariableRefMap::const_iterator refiter = References.find(name);
	if(refiter != References.end())
	{
		if(!refiter->second.second)
			throw ExecutionException("Cannot access unbound reference");
		return *refiter->second.second;
	}

	if(ParentScope)
		return ParentScope->LookupVariable(name);

	throw MissingVariableException("Failed to find the given variable");
}

//
// Ensure that we cannot duplicate identifiers within scopes, or introduce shadowing identifiers
//
void ScopeDescription::CheckForDuplicateIdentifier(const std::wstring& name) const
{
	if(Variables.find(name) != Variables.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a variable identifier");

	if(References.find(name) != References.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a variable reference identifier");

	if(Futures.find(name) != Futures.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a future identifier");

	if(Functions.find(name) != Functions.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a function identifier");

	if(FunctionSignatures.find(name) != FunctionSignatures.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a function signature identifier");

	if(TupleTypes.find(name) != TupleTypes.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a tuple type identifier");

	if(StructureTypes.find(name) != StructureTypes.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a structure type identifier");

	if(ResponseMaps.find(name) != ResponseMaps.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a response map identifier");

	for(std::deque<GhostVariableMap>::const_iterator iter = Ghosts.begin(); iter != Ghosts.end(); ++iter)
	{
		GhostVariableMap::const_iterator ghostiter = iter->find(name);
		if(ghostiter != iter->end())
			ghostiter->second->CheckForDuplicateIdentifier(name);
	}

	if(ParentScope)
		ParentScope->CheckForDuplicateIdentifier(name);
}


