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

	void RegisterStatementParams(const Namespace& ns, const Statement& statement);
	void RegisterCodeBlock(const Namespace& ns, const CodeBlock& code);


	void RegisterStructure(const Namespace& ns, StringHandle name, const Structure& definition)
	{
		Metadata::EpochTypeID structuretype = ns.Types.GetTypeByName(name);

		const std::vector<std::pair<StringHandle, StructureMember*> >& members = definition.GetMembers();
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		{
			const StructureMember* raw = iter->second;
			if(const StructureMemberVariable* membervar = dynamic_cast<const StructureMemberVariable*>(raw))
			{
				Metadata::EpochTypeID type = membervar->GetEpochType(ns);
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterStructureMemVar", name, structuretype, iter->first, type);

				// TODO - register template args
			}
			else if(const StructureMemberFunctionReference* memberfuncref = dynamic_cast<const StructureMemberFunctionReference*>(raw))
			{
				// TODO - register member function references
			}
			else
				throw FatalException("Unsupported structure member type");
		}

		StringHandle ctorname = definition.GetConstructorName();
		StringHandle anonname = definition.GetAnonymousConstructorName();
		StringHandle copyname = definition.GetCopyConstructorName();
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterConstructors", name, ctorname, anonname, copyname);
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

	void RegisterExpression(const Namespace& ns, const Expression& expr)
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
				throw NotImplementedException("Parenthetical atom type not supported yet");
			}
			else if(const ExpressionAtomIdentifierReference* atom = dynamic_cast<const ExpressionAtomIdentifierReference*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterAtomIdentifierReference", atom->GetIdentifier());
			}
			else if(const ExpressionAtomIdentifier* atom = dynamic_cast<const ExpressionAtomIdentifier*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterAtomIdentifier", atom->GetIdentifier(), atom->GetEpochType(ns));
			}
			else if(const ExpressionAtomOperator* atom = dynamic_cast<const ExpressionAtomOperator*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterOperatorInvoke", atom->GetIdentifier());
			}
			else if(const ExpressionAtomLiteralString* atom = dynamic_cast<const ExpressionAtomLiteralString*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterLiteralString", atom->GetValue());
			}
			else if(const ExpressionAtomLiteralBoolean* atom = dynamic_cast<const ExpressionAtomLiteralBoolean*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterLiteralBoolean", static_cast<Integer32>(atom->GetValue()));
			}
			else if(const ExpressionAtomLiteralInteger32* atom = dynamic_cast<const ExpressionAtomLiteralInteger32*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterLiteralInteger", atom->GetValue(), atom->GetEpochType(ns));
			}
			else if(const ExpressionAtomLiteralReal32* atom = dynamic_cast<const ExpressionAtomLiteralReal32*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterLiteralReal", atom->GetValue());
			}
			else if(const ExpressionAtomStatement* atom = dynamic_cast<const ExpressionAtomStatement*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterSubStatement");
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterStatement", atom->GetStatement().GetName(), atom->GetStatement().GetEpochType(ns));
				RegisterStatementParams(ns, atom->GetStatement());
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
			}
			else if(const ExpressionAtomCopyFromStructure* atom = dynamic_cast<const ExpressionAtomCopyFromStructure*>(rawatom))
			{
				// TODO - register copy from structure atom
				throw NotImplementedException("Copy from structure atom type not supported yet");
			}
			else if(const ExpressionAtomBindReference* atom = dynamic_cast<const ExpressionAtomBindReference*>(rawatom))
			{
				StringHandle identifier = atom->GetIdentifier();
				bool isref = atom->IsReference();
				bool inputref = atom->OverrideInputAsReference();
				StringHandle structidentifier = atom->GetStructureName();

				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterRefBinding", identifier, structidentifier, isref, inputref);
			}
			else if(const ExpressionAtomTypeAnnotation* atom = dynamic_cast<const ExpressionAtomTypeAnnotation*>(rawatom))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterTypeAnnotation", static_cast<Integer32>(atom->GetEpochType(ns)));
			}
			else if(const ExpressionAtomTempReferenceFromRegister* atom = dynamic_cast<const ExpressionAtomTempReferenceFromRegister*>(rawatom))
			{
				// TODO - register temp reference binding atom
				throw NotImplementedException("Atom type not supported yet");
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

	void RegisterStatementParams(const Namespace& ns, const Statement& statement)
	{
		const std::vector<Expression*>& params = statement.GetParameters();
		for(std::vector<Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
			RegisterExpression(ns, **iter);
	}

	void RegisterAssignmentChain(const Namespace& ns, const AssignmentChain& chain)
	{
		if(const AssignmentChainExpression* chainexpr = dynamic_cast<const AssignmentChainExpression*>(&chain))
			RegisterExpression(ns, chainexpr->GetExpression());
		else
			throw NotImplementedException("TODO - implement assignment chain support");
	}

	void RegisterEntity(const Namespace& ns, const Entity& entity)
	{
		Bytecode::EntityTag postfixtag = entity.GetPostfixIdentifier() ? ns.GetEntityCloserTag(entity.GetPostfixIdentifier()) : 0;

		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterEntity", ns.GetEntityTag(entity.GetName()), postfixtag);
		if(!entity.GetParameters().empty())		// TODO - multi-param entities?
			RegisterExpression(ns, *entity.GetParameters()[0]);
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterEntityCode");
		RegisterCodeBlock(ns, entity.GetCode());
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
	}

	void RegisterCodeBlock(const Namespace& ns, const CodeBlock& code)
	{
		const std::vector<CodeBlockEntry*>& entries = code.GetEntries();
		for(std::vector<CodeBlockEntry*>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
		{
			if(const CodeBlockStatementEntry* statement = dynamic_cast<const CodeBlockStatementEntry*>(*iter))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterStatement", statement->GetStatement().GetName(), statement->GetStatement().GetEpochType(ns));
				RegisterStatementParams(ns, statement->GetStatement());
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
			}
			else if(const CodeBlockAssignmentEntry* assignment = dynamic_cast<const CodeBlockAssignmentEntry*>(*iter))
			{
				if(assignment->GetAssignment().GetLHS().size() == 1)
				{
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterAssignment", assignment->GetAssignment().GetLHS().front());
					RegisterAssignmentChain(ns, *assignment->GetAssignment().GetRHS());
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
				}
				else
				{
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterAssignmentCompound", assignment->GetAssignment().GetLHS().front());
					for(size_t i = 1; i < assignment->GetAssignment().GetLHS().size(); ++i)
						Plugins.InvokeVoidPluginFunction(L"PluginCodeGenAssignmentCompoundMember", assignment->GetAssignment().GetLHS()[i]);
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenAssignmentCompoundEnd");
					RegisterAssignmentChain(ns, *assignment->GetAssignment().GetRHS());
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
				}
			}
			else if(const CodeBlockEntityEntry* entity = dynamic_cast<const CodeBlockEntityEntry*>(*iter))
			{
				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterChain");
				RegisterEntity(ns, entity->GetEntity());

				const std::vector<Entity*>& chain = entity->GetEntity().GetChain();
				for(std::vector<Entity*>::const_iterator chainiter = chain.begin(); chainiter != chain.end(); ++chainiter)
					RegisterEntity(ns, **chainiter);

				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
			}
			else
				throw NotImplementedException("TODO - add code gen support for this entry type");
		}
	}

	void RegisterFunction(const Namespace& ns, StringHandle funcname, const Function& funcdef)
	{
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunction", funcname);

		const std::vector<IRSemantics::FunctionTag>& tags = funcdef.GetTags();
		for(std::vector<IRSemantics::FunctionTag>::const_iterator tagiter = tags.begin(); tagiter != tags.end(); ++tagiter)
		{
			if(ns.FunctionTags.Exists(tagiter->TagName))
			{
				TagHelperReturn help = ns.FunctionTags.GetHelper(tagiter->TagName)(funcname, tagiter->Parameters, false);
				if(!help.InvokeRuntimeFunction.empty())
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunctionInvokeTag", funcname, ns.Strings.Find(help.InvokeRuntimeFunction));

				Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunctionTag", funcname, ns.Strings.GetPooledString(tagiter->TagName).c_str());
				for(size_t i = 0; i < tagiter->Parameters.size(); ++i)
					Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterFunctionTagParam", funcname, ns.Strings.GetPooledString(tagiter->TagName).c_str(), tagiter->Parameters[i].StringPayload.c_str());
			}
			else
			{
				//
				// This is a failure of the semantic validation pass
				// to correctly catch and flag an error on the tag.
				//
				throw InternalException("Unrecognized function tag");
			}
		}

		if(funcdef.GetCode())
			RegisterScope(ns, funcname, *funcdef.GetCode()->GetScope());
		
		if(funcdef.GetReturnExpression())
		{
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterFunctionReturn", funcname, funcdef.HasAnonymousReturn());
			RegisterExpression(ns, *funcdef.GetReturnExpression());
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
		}

		if(funcdef.GetCode())
		{
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterFunctionBody", funcname);
			RegisterCodeBlock(ns, *funcdef.GetCode());
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
		}
	}

	void RegisterSumType(StringHandle sumtypename, Metadata::EpochTypeID sumtypeid, const std::set<Metadata::EpochTypeID>& basetypeids)
	{
		for(std::set<Metadata::EpochTypeID>::const_iterator iter = basetypeids.begin(); iter != basetypeids.end(); ++iter)
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterSumTypeBase", sumtypename, sumtypeid, *iter);
	}

	void RegisterTypeMatchSignature(StringHandle matchername, StringHandle overloadname, const FunctionSignature& sig)
	{
		for(size_t i = 0; i < sig.GetNumParameters(); ++i)
		{
			const CompileTimeParameter& param = sig.GetParameter(i);
			StringHandle paramname = 1;		// TODO - stupid hack
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterTypeMatchParam", matchername, overloadname, paramname, param.Type, static_cast<Integer32>(param.IsReference));
		}
	}

	void RegisterTypeMatcher(StringHandle matchername, const std::map<StringHandle, FunctionSignature>& signatures)
	{
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenEnterTypeMatcher", matchername);

		for(std::map<StringHandle, FunctionSignature>::const_iterator overloaditer = signatures.begin(); overloaditer != signatures.end(); ++overloaditer)
		{
			const FunctionSignature& sig = overloaditer->second;
			RegisterTypeMatchSignature(matchername, overloaditer->first, sig);
		}

		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenExit");
	}

	void RegisterNamespace(const Namespace& ns)
	{
		// Register structure definitions
		const std::map<StringHandle, Structure*>& structures = ns.Types.Structures.GetDefinitions();
		for(std::map<StringHandle, Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
			RegisterStructure(ns, iter->first, *iter->second);

		// TODO - register template instances
		
		// Register sum types
		std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> > sumtypes = ns.Types.SumTypes.GetDefinitions();
		for(std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> >::const_iterator iter = sumtypes.begin(); iter != sumtypes.end(); ++iter)
		{
			if(ns.Types.SumTypes.IsTemplate(ns.Types.GetNameOfType(iter->first)))
				continue;

			RegisterSumType(ns.Types.GetNameOfType(iter->first), iter->first, iter->second);
		}


		// TODO - register type aliases
		// TODO - register function signatures
		// TODO - register global blocks
		
		// Register functions
		const boost::unordered_map<StringHandle, Function*>& functions = ns.Functions.GetDefinitions();
		for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			if(iter->second->IsCodeEmissionSupressed())
				continue;

			RegisterFunction(ns, iter->first, *iter->second);
		}


		// Register type matchers
		const std::map<StringHandle, std::map<StringHandle, FunctionSignature> >& requiredtypematchers = ns.Functions.GetRequiredTypeMatchers();
		for(std::map<StringHandle, std::map<StringHandle, FunctionSignature> >::const_iterator iter = requiredtypematchers.begin(); iter != requiredtypematchers.end(); ++iter)
			RegisterTypeMatcher(iter->first, iter->second);

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


