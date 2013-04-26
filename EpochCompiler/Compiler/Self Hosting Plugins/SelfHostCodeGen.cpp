//
// Stub - shell out to self-hosting code generator
//

#include "pch.h"

#include "Compiler/Self Hosting Plugins/SelfHostCodeGen.h"
#include "Compiler/Self Hosting Plugins/Plugin.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"

#include "Utility/StringPool.h"


using namespace IRSemantics;


namespace
{

	void RegisterStructure(const Namespace& ns, StringHandle name, const Structure& definition)
	{
		const std::vector<std::pair<StringHandle, StructureMember*> >& members = definition.GetMembers();
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		{
			const StructureMember* raw = iter->second;
			if(const StructureMemberVariable* membervar = dynamic_cast<const StructureMemberVariable*>(raw))
			{
				Metadata::EpochTypeID type = membervar->GetEpochType(ns);
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterStructureMemVar", name, iter->first, type);

				// TODO - register template args
			}
			else if(const StructureMemberFunctionReference* memberfuncref = dynamic_cast<const StructureMemberFunctionReference*>(raw))
			{
				// TODO - register member function references
			}
			else
				throw FatalException("Unsupported structure member type");
		}
	}

	void RegisterNamespace(const Namespace& ns)
	{
		const std::map<StringHandle, Structure*>& structures = ns.Types.Structures.GetDefinitions();
		for(std::map<StringHandle, Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
			RegisterStructure(ns, iter->first, *iter->second);

		// TODO - register template instances
		// TODO - register sum types
		// TODO - register type aliases
		// TODO - register function signatures
		// TODO - register function tags
		// TODO - register scopes
		// TODO - register global blocks
		// TODO - register functions
		// TODO - register type matchers
		// TODO - register pattern matchers
	}

}



bool CompilerPasses::GenerateCodeSelfHosted(const IRSemantics::Program& program)
{
	const StringPoolManager& strings = program.GetStringPool();
	const boost::unordered_map<StringHandle, std::wstring>& stringpool = strings.GetInternalPool();

	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = stringpool.begin(); iter != stringpool.end(); ++iter)
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterString", iter->first, iter->second.c_str());

	RegisterNamespace(program.GlobalNamespace);

	// TODO - should we care about potential failure status from the plugin?
	Plugins.InvokeVoidPluginFunction(L"PluginCodeGenProcessProgram");
	return true;
}


