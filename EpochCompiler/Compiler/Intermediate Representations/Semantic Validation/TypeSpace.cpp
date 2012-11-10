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


GlobalIDSpace::GlobalIDSpace()
	: CounterStructureTypeIDs(0),
	  CounterUnitTypeIDs(0),
	  CounterSumTypeIDs(0),
	  CounterTemplateInstantiations(0)
{
}

VM::EpochTypeID GlobalIDSpace::NewTemplateInstantiation()
{
	return (CounterTemplateInstantiations++) | VM::EpochTypeFamily_TemplateInstance;
}

VM::EpochTypeID GlobalIDSpace::NewStructureTypeID()
{
	return (CounterStructureTypeIDs++) | VM::EpochTypeFamily_Structure;
}

VM::EpochTypeID GlobalIDSpace::NewUnitTypeID()
{
	return (CounterUnitTypeIDs++) | VM::EpochTypeFamily_Unit;
}

VM::EpochTypeID GlobalIDSpace::NewSumTypeID()
{
	return (CounterSumTypeIDs++) | VM::EpochTypeFamily_SumType;
}


StructureTable::StructureTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

StructureTable::~StructureTable()
{
	for(std::map<StringHandle, Structure*>::iterator iter = NameToDefinitionMap.begin(); iter != NameToDefinitionMap.end(); ++iter)
		delete iter->second;
}


VM::EpochTypeID StructureTable::GetMemberType(StringHandle structurename, StringHandle membername) const
{
	const std::vector<std::pair<StringHandle, StructureMember*> >& members = NameToDefinitionMap.find(structurename)->second->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
	{
		if(iter->first == membername)
			return iter->second->GetEpochType(MyTypeSpace.MyNamespace);
	}

	throw InternalException("Invalid structure member");
}

StringHandle StructureTable::GetConstructorName(StringHandle structurename) const
{
	return NameToDefinitionMap.find(structurename)->second->GetConstructorName();
}

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


SumTypeTable::SumTypeTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}


VM::EpochTypeID SumTypeTable::Add(const std::wstring& name, CompileErrors& errors)
{
	StringHandle namehandle = MyTypeSpace.MyNamespace.Strings.Pool(name);

	if(MyTypeSpace.GetTypeByName(namehandle) != VM::EpochType_Error)
		errors.SemanticError("Type name already in use");

	VM::EpochTypeID type = MyTypeSpace.IDSpace.NewSumTypeID();
	NameToTypeMap[namehandle] = type;

	return type;
}



bool SumTypeTable::IsBaseType(VM::EpochTypeID sumtypeid, VM::EpochTypeID basetype) const
{
	std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = BaseTypeNames.find(sumtypeid);
	for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
	{
		if(MyTypeSpace.GetTypeByName(*btiter) == basetype)
			return true;
	}

	return false;
}

void SumTypeTable::AddBaseTypeToSumType(VM::EpochTypeID sumtypeid, StringHandle basetypename)
{
	BaseTypeNames[sumtypeid].insert(basetypename);
}



TypeAliasTable::TypeAliasTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

VM::EpochTypeID TypeAliasTable::GetStrongRepresentation(VM::EpochTypeID aliastypeid) const
{
	std::map<VM::EpochTypeID, VM::EpochTypeID>::const_iterator iter = StrongRepresentationTypes.find(aliastypeid);
	if(iter == StrongRepresentationTypes.end())
		throw InternalException("Invalid strong type alias");

	return iter->second;
}

VM::EpochTypeID TypeAliasTable::GetStrongRepresentationName(VM::EpochTypeID aliastypeid) const
{
	std::map<VM::EpochTypeID, StringHandle>::const_iterator iter = StrongRepresentationNames.find(aliastypeid);
	if(iter == StrongRepresentationNames.end())
		throw InternalException("Invalid strong type alias");

	return iter->second;
}

void TypeAliasTable::AddWeakAlias(StringHandle aliasname, VM::EpochTypeID representationtype)
{
	WeakNameToTypeMap[aliasname] = representationtype;
}

void TypeAliasTable::AddStrongAlias(StringHandle aliasname, VM::EpochTypeID representationtype, StringHandle representationname)
{
	VM::EpochTypeID newtypeid = MyTypeSpace.IDSpace.NewUnitTypeID();
	StrongNameToTypeMap[aliasname] = newtypeid;
	StrongRepresentationTypes[newtypeid] = representationtype;
	StrongRepresentationNames[newtypeid] = representationname;
}

StringHandle SumTypeTable::MapConstructorName(StringHandle sumtypeoverloadname) const
{
	std::map<StringHandle, StringHandle>::const_iterator iter = NameToConstructorMap.find(sumtypeoverloadname);
	if(iter == NameToConstructorMap.end())
		return sumtypeoverloadname;

	return iter->second;
}

unsigned SumTypeTable::GetNumBaseTypes(VM::EpochTypeID type) const
{
	return BaseTypeNames.find(type)->second.size();
}

std::map<VM::EpochTypeID, std::set<VM::EpochTypeID> > SumTypeTable::GetDefinitions() const
{
	std::map<VM::EpochTypeID, std::set<VM::EpochTypeID> > ret;
	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator typeiter = BaseTypeNames.begin(); typeiter != BaseTypeNames.end(); ++typeiter)
	{
		for(std::set<StringHandle>::const_iterator nameiter = typeiter->second.begin(); nameiter != typeiter->second.end(); ++nameiter)
			ret[typeiter->first].insert(MyTypeSpace.GetTypeByName(*nameiter));
	}

	return ret;
}


TemplateTable::TemplateTable(TypeSpace& typespace)
	: MyTypeSpace(typespace)
{
}

std::wstring TemplateTable::GenerateTemplateMangledName(VM::EpochTypeID type)
{
	std::wostringstream formatter;
	formatter << "@@templateinst@" << (type & 0xffffff);
	return formatter.str();
}


StringHandle TemplateTable::InstantiateStructure(StringHandle templatename, const CompileTimeParameterVector& args)
{
	InstantiationMap::iterator iter = Instantiations.find(templatename);
	if(iter != Instantiations.end())
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

	VM::EpochTypeID type = MyTypeSpace.IDSpace.NewTemplateInstantiation();
	std::wstring mangled = GenerateTemplateMangledName(type);
	StringHandle mangledname = MyTypeSpace.MyNamespace.Strings.Pool(mangled);
	Instantiations[templatename].insert(std::make_pair(mangledname, args));
	NameToTypeMap[mangledname] = type;

	StringHandle constructorname = MyTypeSpace.MyNamespace.Strings.Pool(mangled + L"@@constructor");
	StringHandle anonconstructorname = MyTypeSpace.MyNamespace.Strings.Pool(mangled + L"@@anonconstructor");

	ConstructorNameCache.Add(mangledname, constructorname);
	AnonConstructorNameCache.Add(mangledname, anonconstructorname);

	return mangledname;
}

StringHandle TemplateTable::FindConstructorName(StringHandle instancename) const
{
	return ConstructorNameCache.Find(instancename);
}

StringHandle TemplateTable::FindAnonConstructorName(StringHandle instancename) const
{
	return AnonConstructorNameCache.Find(instancename);
}


#pragma warning(push)
#pragma warning(disable: 4355)

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

VM::EpochTypeID TypeSpace::GetTypeByName(StringHandle name) const
{
	// TODO - replace this with a less hard-coded solution
	const std::wstring& type = MyNamespace.Strings.GetPooledString(name);

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
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Structures.NameToTypeMap.find(name);
		if(iter != Structures.NameToTypeMap.end())
			return iter->second;
	}
	
	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Aliases.WeakNameToTypeMap.find(name);
		if(iter != Aliases.WeakNameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Aliases.StrongNameToTypeMap.find(name);
		if(iter != Aliases.StrongNameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = SumTypes.NameToTypeMap.find(name);
		if(iter != SumTypes.NameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Templates.NameToTypeMap.find(name);
		if(iter != Templates.NameToTypeMap.end())
			return iter->second;
	}

	{
		std::map<StringHandle, StringHandle>::const_iterator iter = SumTypes.NameToConstructorMap.find(name);
		if(iter != SumTypes.NameToConstructorMap.end())
			return GetTypeByName(iter->second);
	}

	return VM::EpochType_Error;
}

StringHandle TypeSpace::GetNameOfType(VM::EpochTypeID type) const
{
	// TODO - extend to other names

	for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Structures.NameToTypeMap.begin(); iter != Structures.NameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
			return iter->first;
	}

	for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = Templates.NameToTypeMap.begin(); iter != Templates.NameToTypeMap.end(); ++iter)
	{
		if(iter->second == type)
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

bool TypeSpace::Validate(CompileErrors& errors) const
{
	bool valid = true;

	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypes.BaseTypeNames.begin(); iter != SumTypes.BaseTypeNames.end(); ++iter)
	{
		for(std::set<StringHandle>::const_iterator setiter = iter->second.begin(); setiter != iter->second.end(); ++setiter)
		{
			if(GetTypeByName(*setiter) == VM::EpochType_Error)
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
			const TemplateTable::InstancesAndArguments& instances = Templates.Instantiations[iter->first];
			if(instances.empty())
			{
				errors.SemanticError("Template never instantiated");
				return false;
			}

			for(TemplateTable::InstancesAndArguments::const_iterator instanceiter = instances.begin(); instanceiter != instances.end(); ++instanceiter)
			{
				if(!iter->second->InstantiateTemplate(instanceiter->first, instanceiter->second, MyNamespace, errors))
					return false;
			}
		}
	}


	for(std::map<VM::EpochTypeID, std::set<StringHandle> >::const_iterator iter = SumTypes.BaseTypeNames.begin(); iter != SumTypes.BaseTypeNames.end(); ++iter)
	{
		VM::EpochTypeID sumtypeid = iter->first;

		// TODO - refactor a bit
		//////////////////////////////////////
		std::wostringstream overloadnamebuilder;
		overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << sumtypeid;
		StringHandle sumtypeconstructorname = 0;
		for(std::map<StringHandle, VM::EpochTypeID>::const_iterator niter = SumTypes.NameToTypeMap.begin(); niter != SumTypes.NameToTypeMap.end(); ++niter)
		{
			if(niter->second == sumtypeid)
			{
				sumtypeconstructorname = niter->first;
				break;
			}
		}

		if(!sumtypeconstructorname)
			throw InternalException("Missing sum type name mapping");

		StringHandle overloadname = MyNamespace.Strings.Pool(overloadnamebuilder.str());
		MyNamespace.Session.FunctionOverloadNames[sumtypeconstructorname].insert(overloadname);

		FunctionSignature signature;
		signature.AddParameter(L"@id", VM::EpochType_Identifier, false);
		signature.AddParameter(L"@value", sumtypeid, false);

		// TODO - evil hackery here
		MyNamespace.Session.CompileTimeHelpers.insert(std::make_pair(sumtypeconstructorname, MyNamespace.Session.CompileTimeHelpers.find(MyNamespace.Strings.Find(L"integer"))->second));
		MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, signature));
		SumTypes.NameToConstructorMap[overloadname] = sumtypeconstructorname;
		//////////////////////////////////////

		for(std::set<StringHandle>::const_iterator btiter = iter->second.begin(); btiter != iter->second.end(); ++btiter)
		{
			StringHandle basetypename = *btiter;

			std::wostringstream overloadnamebuilder;
			overloadnamebuilder << L"@@sumtypeconstructor@" << sumtypeid << L"@" << MyNamespace.Strings.GetPooledString(basetypename);
			StringHandle sumtypeconstructorname = 0;
			for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = SumTypes.NameToTypeMap.begin(); iter != SumTypes.NameToTypeMap.end(); ++iter)
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

			MyNamespace.Session.CompileTimeHelpers.insert(std::make_pair(overloadname, MyNamespace.Session.CompileTimeHelpers.find(basetypename)->second));
			MyNamespace.Session.FunctionSignatures.insert(std::make_pair(overloadname, MyNamespace.Session.FunctionSignatures.find(basetypename)->second));
			SumTypes.NameToConstructorMap[overloadname] = basetypename;
		}
	}

	return true;
}

