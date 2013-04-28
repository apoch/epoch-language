//
// Stub - shell out to self-hosting code generator
//

#include "pch.h"

#include "Compiler/Self Hosting Plugins/SelfHostCodeGen.h"
#include "Compiler/Self Hosting Plugins/Plugin.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"

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

	void RegisterScope(const Namespace& ns, StringHandle scopename, const ScopeDescription& desc)
	{
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterScope", scopename, ns.FindLexicalScopeName(desc.ParentScope));
		for(size_t i = 0; i < desc.GetVariableCount(); ++i)
		{
			StringHandle varname = desc.GetVariableNameHandle(i);
			Metadata::EpochTypeID vartype = desc.GetVariableTypeByIndex(i);
			bool varref = desc.IsReference(i);
			VariableOrigin varorigin = desc.GetVariableOrigin(i);

			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterVariable", scopename, varname, vartype, static_cast<Integer32>(varref), static_cast<Integer32>(varorigin));
		}
	}

	void RegisterCodeBlock(const CodeBlock& code)
	{
		const std::vector<CodeBlockEntry*>& entries = code.GetEntries();
		for(std::vector<CodeBlockEntry*>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
		{
			if(const CodeBlockStatementEntry* statement = dynamic_cast<const CodeBlockStatementEntry*>(*iter))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterStatement", statement->GetStatement().GetName());
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
			}
			else
				throw NotImplementedException("TODO - add code gen support for this entry type");
		}
	}

	void RegisterFunction(StringHandle funcname, const Function& funcdef)
	{
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunction", funcname);
		if(funcdef.GetCode())
		{
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterFunctionBody", funcname);
			RegisterCodeBlock(*funcdef.GetCode());
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
		}
	}

	void RegisterNamespace(const Namespace& ns)
	{
		// Register structure definitions
		const std::map<StringHandle, Structure*>& structures = ns.Types.Structures.GetDefinitions();
		for(std::map<StringHandle, Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
			RegisterStructure(ns, iter->first, *iter->second);

		// TODO - register template instances
		// TODO - register sum types
		// TODO - register type aliases
		// TODO - register function signatures
		// TODO - register function tags

		// Register scopes
		const IRSemantics::ScopePtrMap& scopes = ns.GetScopes();
		for(IRSemantics::ScopePtrMap::const_iterator iter = scopes.begin(); iter != scopes.end(); ++iter)
			RegisterScope(ns, iter->first, *iter->second);

		// TODO - register global blocks
		
		// Register functions
		const boost::unordered_map<StringHandle, IRSemantics::Function*>& functions = ns.Functions.GetDefinitions();
		for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			if(iter->second->IsCodeEmissionSupressed())
				continue;

			RegisterFunction(iter->first, *iter->second);
		}


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


