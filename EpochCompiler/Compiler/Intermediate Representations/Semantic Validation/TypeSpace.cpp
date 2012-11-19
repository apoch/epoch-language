//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR management objects for type metadata and other tracking
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/TypeSpace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"

#include "Utility/StringPool.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"
#include "Compiler/Session.h"


using namespace IRSemantics;


//
// Construct and initialize a global/program-wide ID tracker
//
GlobalIDSpace::GlobalIDSpace()
	: CounterStructureTypeIDs(0),
	  CounterUnitTypeIDs(0),
	  CounterSumTypeIDs(0),
	  CounterTemplateInstantiations(0)
{
}

//
// Allocate a type ID for a template instantiation
//
Metadata::EpochTypeID GlobalIDSpace::NewTemplateInstantiation()
{
	return (CounterTemplateInstantiations++) | Metadata::EpochTypeFamily_TemplateInstance;
}

//
// Allocate a type ID for a structure
//
Metadata::EpochTypeID GlobalIDSpace::NewStructureTypeID()
{
	return (CounterStructureTypeIDs++) | Metadata::EpochTypeFamily_Structure;
}

//
// Allocate a type ID for a strong type alias
//
Metadata::EpochTypeID GlobalIDSpace::NewUnitTypeID()
{
	return (CounterUnitTypeIDs++) | Metadata::EpochTypeFamily_Unit;
}

//
// Allocate a type ID for an algebraic sum type
//
Metadata::EpochTypeID GlobalIDSpace::NewSumTypeID()
{
	return (CounterSumTypeIDs++) | Metadata::EpochTypeFamily_SumType;
}


//
// Construct and initialize a table of structure types
//
StructureTable::StructureTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

//
// Destruct and clean up a table of structure types
//
StructureTable::~StructureTable()
{
	for(std::map<StringHandle, Structure*>::iterator iter = NameToDefinitionMap.begin(); iter != NameToDefinitionMap.end(); ++iter)
		delete iter->second;
}

//
// Retrieve the IR definition of a given structure (by name)
//
const Structure* StructureTable::GetDefinition(StringHandle structurename) const
{
	std::map<StringHandle, Structure*>::const_iterator iter = NameToDefinitionMap.find(structurename);
	if(iter != NameToDefinitionMap.end())
		return iter->second;

	if(MyTypeSpace.MyNamespace.Parent)
		return MyTypeSpace.MyNamespace.Parent->Types.Structures.GetDefinition(structurename);

	throw InternalException("Invalid structure name");
}

Structure* StructureTable::GetDefinition(StringHandle structurename)
{
	std::map<StringHandle, Structure*>::iterator iter = NameToDefinitionMap.find(structurename);
	if(iter != NameToDefinitionMap.end())
		return iter->second;

	if(MyTypeSpace.MyNamespace.Parent)
		return MyTypeSpace.MyNamespace.Parent->Types.Structures.GetDefinition(structurename);

	throw InternalException("Invalid structure name");
}

//
// Retrieve the type of a given structure member (by name)
//
Metadata::EpochTypeID StructureTable::GetMemberType(StringHandle structurename, StringHandle membername) const
{
	if(MyTypeSpace.Templates.NameToTypeMap.find(structurename) != MyTypeSpace.Templates.NameToTypeMap.end())
		structurename = MyTypeSpace.Templates.GetTemplateForInstance(structurename);

	std::map<StringHandle, Structure*>::const_iterator defiter = NameToDefinitionMap.find(structurename);
	if(defiter == NameToDefinitionMap.end())
	{
		if(!MyTypeSpace.MyNamespace.Parent)
			throw InternalException("Invalid structure name");

		return MyTypeSpace.MyNamespace.Parent->Types.Structures.GetMemberType(structurename, membername);
	}

	const std::vector<std::pair<StringHandle, StructureMember*> >& members = defiter->second->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
	{
		if(iter->first == membername)
			return iter->second->GetEpochType(MyTypeSpace.MyNamespace);
	}

	throw InternalException("Invalid structure member");
}

//
// Retrieve the name of a structure type's constructor (by name)
//
StringHandle StructureTable::GetConstructorName(StringHandle structurename) const
{
	return NameToDefinitionMap.find(structurename)->second->GetConstructorName();
}

//
// Add a new structure type definition to a structure table
//
void StructureTable::Add(StringHandle name, Structure* structure, CompileErrors& errors)
{
	if(NameToDefinitionMap.find(name) != NameToDefinitionMap.end())
	{
		delete structure;
		errors.SemanticError("Duplicate structure name");
		return;
	}

	NameToDefinitionMap.insert(std::make_pair(name, structure));
	NameToTypeMap.insert(std::make_pair(name, MyTypeSpace.IDSpace.NewStructureTypeID()));
	
	MyTypeSpace.MyNamespace.Functions.GenerateStructureFunctions(name, structure);
}


//
// Construct and initialize a table tracking algebraic sum types
//
SumTypeTable::SumTypeTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

//
// Add a new algebraic sum type to a table
//
Metadata::EpochTypeID SumTypeTable::Add(const std::wstring& name, CompileErrors& errors)
{
	StringHandle namehandle = MyTypeSpace.MyNamespace.Strings.Pool(name);

	if(MyTypeSpace.GetTypeByName(namehandle) != Metadata::EpochType_Error)
		errors.SemanticError("Type name already in use");

	Metadata::EpochTypeID type = MyTypeSpace.IDSpace.NewSumTypeID();
	NameToTypeMap[namehandle] = type;

	return type;
}

//
// Determine if the given sum type has a particular type as a base
//
bool SumTypeTable::IsBaseType(Metadata::EpochTypeID sumtypeid, Metadata::EpochTypeID basetype) const
{
	std::map<Metadata::EpochTypeID, std::set<StringHandle> >::const_iterator iter = BaseTypeNames.find(sumtypeid);
	if(iter == BaseTypeNames.end())
	{
		if(MyTypeSpace.MyNamespace.Parent)
			return MyTypeSpace.MyNamespace.Parent->Types.SumTypes.IsBaseType(sumtypeid, basetype);

		throw InternalException("Undefined sum type");
	}

	for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
	{
		if(MyTypeSpace.GetTypeByName(*btiter) == basetype)
			return true;
	}

	return false;
}

//
// Add a base type to an existing sum type definition (base type provided by name)
//
void SumTypeTable::AddBaseTypeToSumType(Metadata::EpochTypeID sumtypeid, StringHandle basetypename)
{
	BaseTypeNames[sumtypeid].insert(basetypename);
}

//
// Instantiate a sum type template
//
StringHandle SumTypeTable::InstantiateTemplate(StringHandle templatename, const CompileTimeParameterVector& originalargs)
{
	CompileTimeParameterVector args(originalargs);
	for(CompileTimeParameterVector::iterator iter = args.begin(); iter != args.end(); ++iter)
	{
		if(MyTypeSpace.Aliases.HasWeakAliasNamed(iter->Payload.LiteralStringHandleValue))
			iter->Payload.LiteralStringHandleValue = MyTypeSpace.Aliases.GetWeakTypeBaseName(iter->Payload.LiteralStringHandleValue);
	}

	TypeSpace* typespace = &MyTypeSpace;
	while(typespace)
	{
		InstantiationMap::iterator iter = typespace->SumTypes.Instantiations.find(templatename);
		if(iter != typespace->SumTypes.Instantiations.end())
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

		if(!typespace->MyNamespace.Parent)
			break;

		typespace = &typespace->MyNamespace.Parent->Types;
	}

	Metadata::EpochTypeID type = MyTypeSpace.IDSpace.NewSumTypeID();
	std::wstring mangled = GenerateTemplateMangledName(type);
	StringHandle mangledname = MyTypeSpace.MyNamespace.Strings.Pool(mangled);
	Instantiations[templatename].insert(std::make_pair(mangledname, args));
	NameToTypeMap[mangledname] = type;
	
	for(std::set<StringHandle>::const_iterator iter = BaseTypeNames[NameToTypeMap[templatename]].begin(); iter != BaseTypeNames[NameToTypeMap[templatename]].end(); ++iter)
	{
		if(MyTypeSpace.Structures.IsStructureTemplate(*iter))
		{
			// TODO - remap passed-in arguments to the arguments expected by this structure template
			StringHandle structurename = MyTypeSpace.Templates.InstantiateStructure(*iter, args);
			BaseTypeNames[type].insert(structurename);
		}
		else
			BaseTypeNames[type].insert(*iter);
	}

	return mangledname;
}

//
// Internal helper for generating template instance mangled names
//
std::wstring SumTypeTable::GenerateTemplateMangledName(Metadata::EpochTypeID type)
{
	std::wostringstream formatter;
	formatter << "@@sumtemplateinst@" << (type & 0xffffff);
	return formatter.str();
}

//
// Determine if the given name corresponds to a templated sum type
//
// Note the distinction between a templated sum type (which this function
// will detect) versus an instance of such a type (which it will not).
//
bool SumTypeTable::IsTemplate(StringHandle name) const
{
	if(NameToParamsMap.find(name) != NameToParamsMap.end())
		return true;

	if(MyTypeSpace.MyNamespace.Parent)
		return MyTypeSpace.MyNamespace.Parent->Types.SumTypes.IsTemplate(name);

	return false;
}

//
// Add a template parameter to the given sum type
//
// Sum type templates only accept type wildcards, so we don't
// need to provide anything besides the name of the parameter.
//
void SumTypeTable::AddTemplateParameter(Metadata::EpochTypeID sumtype, StringHandle name)
{
	NameToParamsMap[MyTypeSpace.GetNameOfType(sumtype)].push_back(name);
}

//
// Given the name of a sum type overload, return the name of the appropriate constructor
//
StringHandle SumTypeTable::MapConstructorName(StringHandle sumtypeoverloadname) const
{
	std::map<StringHandle, StringHandle>::const_iterator iter = NameToConstructorMap.find(sumtypeoverloadname);
	if(iter == NameToConstructorMap.end())
	{
		if(MyTypeSpace.MyNamespace.Parent)
			return MyTypeSpace.MyNamespace.Parent->Types.SumTypes.MapConstructorName(sumtypeoverloadname);

		return sumtypeoverloadname;
	}

	return iter->second;
}

//
// Retrieve the number of base types assigned to a given sum type
//
unsigned SumTypeTable::GetNumBaseTypes(Metadata::EpochTypeID type) const
{
	std::map<Metadata::EpochTypeID, std::set<StringHandle> >::const_iterator iter = BaseTypeNames.find(type);
	if(iter == BaseTypeNames.end())
	{
		if(MyTypeSpace.MyNamespace.Parent)
			return MyTypeSpace.MyNamespace.Parent->Types.SumTypes.GetNumBaseTypes(type);

		throw InternalException("Not a defined sum type");
	}

	return iter->second.size();
}

//
// Retrieve a table of all sum type definitions and their base types
//
// Provides type IDs only; primarily useful for code generation purposes
//
std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> > SumTypeTable::GetDefinitions() const
{
	std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> > ret;
	for(std::map<Metadata::EpochTypeID, std::set<StringHandle> >::const_iterator typeiter = BaseTypeNames.begin(); typeiter != BaseTypeNames.end(); ++typeiter)
	{
		for(std::set<StringHandle>::const_iterator nameiter = typeiter->second.begin(); nameiter != typeiter->second.end(); ++nameiter)
			ret[typeiter->first].insert(MyTypeSpace.GetTypeByName(*nameiter));
	}

	return ret;
}




//
// Construct and initialize a table that tracks strong and weak type aliases
//
TypeAliasTable::TypeAliasTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

//
// Retrieve the original type underlying a strong type alias
//
Metadata::EpochTypeID TypeAliasTable::GetStrongRepresentation(Metadata::EpochTypeID aliastypeid) const
{
	std::map<Metadata::EpochTypeID, Metadata::EpochTypeID>::const_iterator iter = StrongRepresentationTypes.find(aliastypeid);
	if(iter == StrongRepresentationTypes.end())
		throw InternalException("Invalid strong type alias");

	return iter->second;
}

//
// Retrieve the name of the type underlying a strong type alias
//
Metadata::EpochTypeID TypeAliasTable::GetStrongRepresentationName(Metadata::EpochTypeID aliastypeid) const
{
	std::map<Metadata::EpochTypeID, StringHandle>::const_iterator iter = StrongRepresentationNames.find(aliastypeid);
	if(iter == StrongRepresentationNames.end())
		throw InternalException("Invalid strong type alias");

	return iter->second;
}

//
// Add a weak alias to the alias table
//
void TypeAliasTable::AddWeakAlias(StringHandle aliasname, Metadata::EpochTypeID representationtype)
{
	WeakNameToTypeMap[aliasname] = representationtype;
}

//
// Determine if the given name corresponds to a weak type alias
//
bool TypeAliasTable::HasWeakAliasNamed(StringHandle name) const
{
	return WeakNameToTypeMap.find(name) != WeakNameToTypeMap.end();
}

//
// Retrieve the name of the underlying type of a weak type alias
//
StringHandle TypeAliasTable::GetWeakTypeBaseName(StringHandle name) const
{
	return MyTypeSpace.GetNameOfType(WeakNameToTypeMap.find(name)->second);
}

//
// Add a strong type alias to the alias table
//
void TypeAliasTable::AddStrongAlias(StringHandle aliasname, Metadata::EpochTypeID representationtype, StringHandle representationname)
{
	Metadata::EpochTypeID newtypeid = MyTypeSpace.IDSpace.NewUnitTypeID();
	StrongNameToTypeMap[aliasname] = newtypeid;
	StrongRepresentationTypes[newtypeid] = representationtype;
	StrongRepresentationNames[newtypeid] = representationname;
}


//
// Construct and initialize a table of structure template instances
//
TemplateTable::TemplateTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

//
// Internal helper for generating a mangled name for a structure template instance
//
std::wstring TemplateTable::GenerateTemplateMangledName(Metadata::EpochTypeID type)
{
	std::wostringstream formatter;
	formatter << "@@templateinst@" << (type & 0xffffff);
	return formatter.str();
}

//
// Instantiate a structure template
//
StringHandle TemplateTable::InstantiateStructure(StringHandle templatename, const CompileTimeParameterVector& originalargs)
{
	{
		TypeSpace* typespace = &MyTypeSpace;
		while(typespace)
		{
			if(typespace->Templates.NameToTypeMap.find(templatename) != typespace->Templates.NameToTypeMap.end())
				return templatename;

			if(!typespace->MyNamespace.Parent)
				break;

			typespace = &typespace->MyNamespace.Parent->Types;
		}
	}

	const Structure* structdef = MyTypeSpace.Structures.GetDefinition(templatename);
	CompileTimeParameterVector args(originalargs);
	for(size_t i = 0; i < args.size(); ++i)
	{
		if(structdef->TemplateParams[i].second != Metadata::EpochType_Wildcard)
			continue;

		if(MyTypeSpace.Aliases.HasWeakAliasNamed(args[i].Payload.LiteralStringHandleValue))
			args[i].Payload.LiteralStringHandleValue = MyTypeSpace.Aliases.GetWeakTypeBaseName(args[i].Payload.LiteralStringHandleValue);
	}

	TypeSpace* typespace = &MyTypeSpace;
	while(typespace)
	{
		InstantiationMap::iterator iter = typespace->Templates.Instantiations.find(templatename);
		if(iter != typespace->Templates.Instantiations.end())
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

		if(!typespace->MyNamespace.Parent)
			break;

		typespace = &typespace->MyNamespace.Parent->Types;
	}

	Metadata::EpochTypeID type = typespace->IDSpace.NewTemplateInstantiation();
	std::wstring mangled = GenerateTemplateMangledName(type);
	StringHandle mangledname = typespace->MyNamespace.Strings.Pool(mangled);
	typespace->Templates.Instantiations[templatename].insert(std::make_pair(mangledname, args));
	typespace->Templates.NameToTypeMap[mangledname] = type;

	StringHandle constructorname = MyTypeSpace.MyNamespace.Strings.Pool(mangled + L"@@constructor");
	StringHandle anonconstructorname = MyTypeSpace.MyNamespace.Strings.Pool(mangled + L"@@anonconstructor");

	typespace->Templates.ConstructorNameCache.Add(mangledname, constructorname);
	typespace->Templates.AnonConstructorNameCache.Add(mangledname, anonconstructorname);

	return mangledname;
}

//
// Retrieve the name of the constructor for the given template instance (by name)
//
StringHandle TemplateTable::FindConstructorName(StringHandle instancename) const
{
	StringHandle ret = ConstructorNameCache.Find(instancename);
	if(!ret && MyTypeSpace.MyNamespace.Parent)
		return MyTypeSpace.MyNamespace.Parent->Types.Templates.FindConstructorName(instancename);

	return ret;
}

//
// Retrieve the name of the anonymous (inline-temporary) constructor for the
// given template instance (by name)
//
// Anonymous constructors do not create variables in their containing lexical
// scopes. Instead, they simply create a temporary value on the stack, suitable
// for short-lived objects. Anonymous temporaries can generate garbage.
//
StringHandle TemplateTable::FindAnonConstructorName(StringHandle instancename) const
{
	StringHandle ret = AnonConstructorNameCache.Find(instancename);
	if(!ret && MyTypeSpace.MyNamespace.Parent)
		return MyTypeSpace.MyNamespace.Parent->Types.Templates.FindAnonConstructorName(instancename);

	return ret;
}

//
// Retrieve the name of the structure that defined the given template instance (by name)
//
StringHandle TemplateTable::GetTemplateForInstance(StringHandle instancename) const
{
	for(InstantiationMap::const_iterator iter = Instantiations.begin(); iter != Instantiations.end(); ++iter)
	{
		if(iter->second.find(instancename) != iter->second.end())
			return iter->first;
	}

	throw InternalException("Invalid template instance");
}

//
// Determine if the given name correponds to a structure template
//
// Note that this does not detect structure template instances!
//
bool StructureTable::IsStructureTemplate(StringHandle name) const
{
	std::map<StringHandle, Structure*>::const_iterator iter = NameToDefinitionMap.find(name);
	if(iter == NameToDefinitionMap.end())
	{
		if(MyTypeSpace.MyNamespace.Parent)
			return MyTypeSpace.MyNamespace.Parent->Types.Structures.IsStructureTemplate(name);

		return false;
	}

	return iter->second->IsTemplate();
}


// Evil: suppress warning about *this in initializer lists
#pragma warning(push)
#pragma warning(disable: 4355)

//
// Construct and initialize a typespace wrapper
//
TypeSpace::TypeSpace(Namespace& mynamespace, GlobalIDSpace& idspace)
	: MyNamespace(mynamespace),
	  IDSpace(idspace),
	  Aliases(*this),
	  Structures(*this),
	  SumTypes(*this),
	  Templates(*this)
{
}

#pragma warning(pop)
// End evil

//
// Retrieve the type ID corresponding to the given name
//
Metadata::EpochTypeID TypeSpace::GetTypeByName(StringHandle name) const
{
	{
		NameToTypeTable::const_iterator iter = MyNamespace.Session.IntrinsicTypes.find(name);
		if(iter != MyNamespace.Session.IntrinsicTypes.end())
			return iter->second;
	}

	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Structures.NameToTypeMap.find(name);
		if(iter != Structures.NameToTypeMap.end())
			return iter->second;
	}
	
	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Aliases.WeakNameToTypeMap.find(name);
		if(iter != Aliases.WeakNameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Aliases.StrongNameToTypeMap.find(name);
		if(iter != Aliases.StrongNameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = SumTypes.NameToTypeMap.find(name);
		if(iter != SumTypes.NameToTypeMap.end())
		{
			if(!SumTypes.IsTemplate(name))
				return iter->second;
		}
	}

	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Templates.NameToTypeMap.find(name);
		if(iter != Templates.NameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, StringHandle>::const_iterator iter = SumTypes.NameToConstructorMap.find(name);
		if(iter != SumTypes.NameToConstructorMap.end())
			return GetTypeByName(iter->second);
	}

	if(MyNamespace.Parent)
		return MyNamespace.Parent->Types.GetTypeByName(name);

	return Metadata::EpochType_Error;
}

//
// Retrieve the name of the given type
//
StringHandle TypeSpace::GetNameOfType(Metadata::EpochTypeID type) const
{
	for(NameToTypeTable::const_iterator iter = MyNamespace.Session.IntrinsicTypes.begin(); iter != MyNamespace.Session.IntrinsicTypes.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Structures.NameToTypeMap.begin(); iter != Structures.NameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Aliases.WeakNameToTypeMap.begin(); iter != Aliases.WeakNameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Aliases.StrongNameToTypeMap.begin(); iter != Aliases.StrongNameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = SumTypes.NameToTypeMap.begin(); iter != SumTypes.NameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = Templates.NameToTypeMap.begin(); iter != Templates.NameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	if(MyNamespace.Parent)
		return MyNamespace.Parent->Types.GetNameOfType(type);

	//
	// This catches a potential logic bug in the compiler implementation.
	//
	// If a data type has been allocated a type ID, that type ID should never
	// be able to exist without being also bound to an identifier, even if that
	// identifier is just an automatically generated magic anonymous token.
	//
	// Callers cannot be faulted directly for this exception; the problem most
	// likely lies elsewhere in the type handling code.
	//
	throw InternalException("Type ID is not bound to any known identifier");

}

//
// Validate the contents of a typespace
//
bool TypeSpace::Validate(CompileErrors& errors) const
{
	bool valid = true;

	for(std::map<Metadata::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypes.BaseTypeNames.begin(); iter != SumTypes.BaseTypeNames.end(); ++iter)
	{
		for(std::set<StringHandle>::const_iterator setiter = iter->second.begin(); setiter != iter->second.end(); ++setiter)
		{
			if(GetTypeByName(*setiter) == Metadata::EpochType_Error)
			{
				// TODO - set context
				errors.SemanticError("Base type not defined");
				valid = false;
			}
		}
	}

	for(std::map<StringHandle, Structure*>::const_iterator iter = Structures.NameToDefinitionMap.begin(); iter != Structures.NameToDefinitionMap.end(); ++iter)
	{
		if(!iter->second->Validate(MyNamespace, errors))
			valid = false;
	}

	return valid;
}

//
// Perform compile-time code execution on the contents of a typespace
//
// Major responsibilities include generation of type instances from templates.
// Also handles setting up constructors and other support infrastructure.
//
bool TypeSpace::CompileTimeCodeExecution(CompileErrors& errors)
{
	for(std::map<StringHandle, Structure*>::iterator iter = Structures.NameToDefinitionMap.begin(); iter != Structures.NameToDefinitionMap.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(iter->first, MyNamespace, errors))
			return false;
	}

	for(std::map<StringHandle, Structure*>::iterator iter = Structures.NameToDefinitionMap.begin(); iter != Structures.NameToDefinitionMap.end(); ++iter)
	{
		if(iter->second->IsTemplate())
		{
			const InstancesAndArguments& instances = Templates.Instantiations[iter->first];
			if(instances.empty())
			{
				errors.SemanticError("Template never instantiated");
				return false;
			}

			for(InstancesAndArguments::const_iterator instanceiter = instances.begin(); instanceiter != instances.end(); ++instanceiter)
			{
				if(!iter->second->InstantiateTemplate(instanceiter->first, instanceiter->second, MyNamespace, errors))
					return false;
			}
		}
	}


	for(std::map<Metadata::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypes.BaseTypeNames.begin(); iter != SumTypes.BaseTypeNames.end(); ++iter)
	{
		Metadata::EpochTypeID sumtypeid = iter->first;
		if(SumTypes.IsTemplate(GetNameOfType(sumtypeid)))
			continue;

		StringHandle sumtypeconstructorname = 0;
		for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator niter = SumTypes.NameToTypeMap.begin(); niter != SumTypes.NameToTypeMap.end(); ++niter)
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
		SumTypes.NameToConstructorMap[overloadname] = sumtypeconstructorname;

		for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
		{
			StringHandle basetypename = *btiter;

			std::wostringstream overloadnamebuilder;
			overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << MyNamespace.Strings.GetPooledString(basetypename);
			StringHandle sumtypeconstructorname = 0;
			for(std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = SumTypes.NameToTypeMap.begin(); iter != SumTypes.NameToTypeMap.end(); ++iter)
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

			Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(GetTypeByName(basetypename));
			switch(family)
			{
			case Metadata::EpochTypeFamily_Magic:
			case Metadata::EpochTypeFamily_Primitive:
			case Metadata::EpochTypeFamily_GC:
				// No adjustment needed
				break;

			case Metadata::EpochTypeFamily_Structure:
				basetypename = Structures.GetConstructorName(basetypename);
				break;

			case Metadata::EpochTypeFamily_TemplateInstance:
				basetypename = Templates.FindConstructorName(basetypename);
				break;

			default:
				throw NotImplementedException("Sum types with this base type not supported");
			}

			MyNamespace.Session.CompileTimeHelpers.insert(std::make_pair(overloadname, MyNamespace.Session.CompileTimeHelpers.find(basetypename)->second));
			MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, MyNamespace.Session.FunctionSignatures.find(basetypename)->second));
			SumTypes.NameToConstructorMap[overloadname] = basetypename;
		}
	}

	return true;
}

