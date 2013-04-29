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

	void RegisterExpression(const Expression& expr)
	{
		const std::vector<ExpressionAtom*>& atoms = expr.GetAtoms();
		for(std::vector<ExpressionAtom*>::const_iterator iter = atoms.begin(); iter != atoms.end(); ++iter)
		{
			const ExpressionAtom* rawatom = *iter;
			if(const ExpressionAtomParenthetical* atom = dynamic_cast<const ExpressionAtomParenthetical*>(rawatom))
			{
				// TODO - register parenthetical
				/*
				const Parenthetical* parenthetical = atom->GetParenthetical();
				if(const ParentheticalPreOp* preop = dynamic_cast<const ParentheticalPreOp*>(parenthetical))
					EmitPreOpStatement(emitter, *preop->GetStatement(), activescope, curnamespace, true);
				else if(const ParentheticalPostOp* postop = dynamic_cast<const ParentheticalPostOp*>(parenthetical))
					EmitPostOpStatement(emitter, *postop->GetStatement(), activescope, curnamespace, true);
				else if(const ParentheticalExpression* expr = dynamic_cast<const ParentheticalExpression*>(parenthetical))
					EmitExpression(emitter, expr->GetExpression(), activescope, curnamespace);
				else
				{
					//
					// This is probably an incomplete language feature.
					//
					// As written, this code supports only a limited number of kinds
					// of things within a parenthetical expression, although frankly
					// they should be sufficient for any real usage. The contents of
					// parentheticals are limited to simplify parsing and traversal.
					//
					throw InternalException("Invalid parenthetical contents");
				}
				*/
			}
			else if(const ExpressionAtomIdentifierReference* atom = dynamic_cast<const ExpressionAtomIdentifierReference*>(rawatom))
			{
				// TODO - register identifier which should be bound as a reference
			}
			else if(const ExpressionAtomIdentifier* atom = dynamic_cast<const ExpressionAtomIdentifier*>(rawatom))
			{
				// TODO - register identifier atom
				/*
				if(atom->GetEpochType(curnamespace) == Metadata::EpochType_Nothing)
				{
					emitter.PushIntegerLiteral(0);
				}
				else
				{
					if(curnamespace.Functions.IRExists(atom->GetIdentifier()))
						emitter.PushFunctionNameLiteral(atom->GetIdentifier());
					else if(curnamespace.Types.GetTypeByName(atom->GetIdentifier()) != Metadata::EpochType_Error)
						emitter.PushStringLiteral(atom->GetIdentifier());
					else
					{
						if(atom->GetEpochType(curnamespace) == Metadata::EpochType_Identifier)
						{
							if(!constructorcall)
								emitter.PushStringLiteral(atom->GetIdentifier());
							else
							{
								size_t frames = 0;
								size_t index = activescope.GetScope()->FindVariable(atom->GetIdentifier(), frames);
								frames = CheckForGlobalFrame(curnamespace, activescope, frames);
								emitter.BindReference(frames, index);
							}
						}
						else if(Metadata::GetTypeFamily(atom->GetEpochType(curnamespace)) == Metadata::EpochTypeFamily_Function)
							emitter.PushFunctionNameLiteral(atom->GetIdentifier());
						else
							PushFast(emitter, curnamespace, activescope, atom->GetIdentifier());
					}
				}
				*/
			}
			else if(const ExpressionAtomOperator* atom = dynamic_cast<const ExpressionAtomOperator*>(rawatom))
			{
				// TODO - register operator invocation atom
			}
			else if(const ExpressionAtomLiteralString* atom = dynamic_cast<const ExpressionAtomLiteralString*>(rawatom))
			{
				// TODO - register string literal atom
			}
			else if(const ExpressionAtomLiteralBoolean* atom = dynamic_cast<const ExpressionAtomLiteralBoolean*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterLiteralBoolean", static_cast<Integer32>(atom->GetValue()));
			}
			else if(const ExpressionAtomLiteralInteger32* atom = dynamic_cast<const ExpressionAtomLiteralInteger32*>(rawatom))
			{
				// TODO - register integer (or integer16!) literal
			}
			else if(const ExpressionAtomLiteralReal32* atom = dynamic_cast<const ExpressionAtomLiteralReal32*>(rawatom))
			{
				// TODO - register real literal
			}
			else if(const ExpressionAtomStatement* atom = dynamic_cast<const ExpressionAtomStatement*>(rawatom))
			{
				// TODO - register statement atom
			}
			else if(const ExpressionAtomCopyFromStructure* atom = dynamic_cast<const ExpressionAtomCopyFromStructure*>(rawatom))
			{
				// TODO - register copy from structure atom
			}
			else if(const ExpressionAtomBindReference* atom = dynamic_cast<const ExpressionAtomBindReference*>(rawatom))
			{
				// TODO - register reference binding atom
				/*
				if(firstmember && !atom->IsReference() && !atom->OverrideInputAsReference())
					emitter.BindStructureReferenceByHandle(atom->GetIdentifier());
				else
				{
					Metadata::EpochTypeID membertype = curnamespace.Types.Structures.GetMemberType(atom->GetStructureName(), atom->GetIdentifier());
					size_t memberoffset = curnamespace.Types.Structures.GetMemberOffset(atom->GetStructureName(), atom->GetIdentifier());
					emitter.BindStructureReference(membertype, memberoffset);
				}
				return !atom->IsReference();
				*/
			}
			else if(const ExpressionAtomTypeAnnotation* atom = dynamic_cast<const ExpressionAtomTypeAnnotation*>(rawatom))
			{
				// TODO - register type annotation atom
			}
			else if(const ExpressionAtomTempReferenceFromRegister* atom = dynamic_cast<const ExpressionAtomTempReferenceFromRegister*>(rawatom))
			{
				// TODO - register temp reference binding atom
			}
			else
			{
				//
				// This is most likely to occur in the presence of
				// incompletely implemented language features. An
				// expression atom is of a type not deduced by any
				// of the above handlers, which represents a gap in
				// the implementation.
				//
				// Implement the correct handler for the atom type
				// or fix the IR generation which produces the bogus
				// results.
				//
				throw InternalException("IR contains an unrecognized expression atom");
			}
		}
	}

	void RegisterStatementParams(const Statement& statement)
	{
		const std::vector<Expression*>& params = statement.GetParameters();
		for(std::vector<Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
			RegisterExpression(**iter);
	}

	void RegisterCodeBlock(const CodeBlock& code)
	{
		const std::vector<CodeBlockEntry*>& entries = code.GetEntries();
		for(std::vector<CodeBlockEntry*>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
		{
			if(const CodeBlockStatementEntry* statement = dynamic_cast<const CodeBlockStatementEntry*>(*iter))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterStatement", statement->GetStatement().GetName());
				RegisterStatementParams(statement->GetStatement());
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
			}
			else
				throw NotImplementedException("TODO - add code gen support for this entry type");
		}
	}

	void RegisterFunction(StringHandle funcname, const Function& funcdef)
	{
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunction", funcname);
		// TODO - register function return expression
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
		const ScopePtrMap& scopes = ns.GetScopes();
		for(ScopePtrMap::const_iterator iter = scopes.begin(); iter != scopes.end(); ++iter)
			RegisterScope(ns, iter->first, *iter->second);

		// TODO - register global blocks
		
		// Register functions
		const boost::unordered_map<StringHandle, Function*>& functions = ns.Functions.GetDefinitions();
		for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
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


