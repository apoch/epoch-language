//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Pass for converting a semantic validation IR
// into output bytecode for the Epoch VM.
//

#include "pch.h"

#include "Compiler/Passes/CodeGeneration.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"

#include "Compiler/ByteCodeEmitter.h"

#include "Compiler/Session.h"

#include "Compiler/Exceptions.h"

#include "Utility/StringPool.h"
#include "Utility/DependencyGraph.h"


namespace
{
	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Namespace& curnamespace, ByteCodeEmitter& emitter);
	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Namespace& curnamespace);
	void EmitExpression(ByteCodeEmitter& emitter, const IRSemantics::Expression& expression, const IRSemantics::CodeBlock& activescope, const IRSemantics::Namespace& curnamespace);

	void BindReference(ByteCodeEmitter& emitter, const std::vector<StringHandle>& identifiers)
	{
		if(identifiers.empty())
		{
			//
			// This represents a failure of the parser and/or semantic
			// validation passes (or later passes) over the AST/IRs.
			//
			// A reference can only be bound if there is an identifier
			// or structure member access (potentially nested) to act
			// as an appropriate l-value.
			//
			throw InternalException("Cannot bind reference to non-existent l-value");
		}

		emitter.BindReference(identifiers[0]);
		
		for(size_t i = 1; i < identifiers.size(); ++i)
			emitter.BindStructureReference(identifiers[i]);
	}

	void PushValue(ByteCodeEmitter& emitter, const std::vector<StringHandle>& identifiers, const IRSemantics::Namespace& curnamespace, const IRSemantics::CodeBlock& activescope)
	{
		if(identifiers.empty())
		{
			//
			// This represents a failure of the parser and/or semantic
			// validation passes (or later passes) over the AST/IRs.
			//
			// In order to push a value onto the stack, we need either
			// an immediate value or an r-value expression. This function
			// is only intended to handle r-values which are primitive
			// variables or structure member accesses (which might be
			// nested).
			//
			throw InternalException("Cannot push the value referred to by a non-existent r-value");
		}

		if(identifiers.size() == 1)
		{
			emitter.PushVariableValue(identifiers[0], activescope.GetVariableTypeByID(identifiers[0]));
		}
		else
		{
			emitter.PushVariableValueNoCopy(identifiers[0]);

			Metadata::EpochTypeID structuretype = activescope.GetVariableTypeByID(identifiers[0]);
			for(size_t i = 1; i < identifiers.size(); ++i)
			{
				StringHandle structurename = curnamespace.Types.GetNameOfType(structuretype);
				StringHandle overloadidhandle = curnamespace.Functions.FindStructureMemberAccessOverload(structurename, identifiers[i]);

				emitter.PushStringLiteral(identifiers[i]);
				emitter.Invoke(overloadidhandle);

				structuretype = curnamespace.Types.Structures.GetMemberType(structurename, identifiers[i]);
			}

			if(structuretype == Metadata::EpochType_Buffer)
				emitter.CopyBuffer();
			else if(Metadata::GetTypeFamily(structuretype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(structuretype) == Metadata::EpochTypeFamily_TemplateInstance)
				emitter.CopyStructure();
		}
	}

	void EmitPreOpStatement(ByteCodeEmitter& emitter, const IRSemantics::PreOpStatement& statement, const IRSemantics::CodeBlock& codeblock, const IRSemantics::Namespace& curnamespace)
	{
		PushValue(emitter, statement.GetOperand(), curnamespace, codeblock);
		emitter.Invoke(statement.GetOperatorName());
		BindReference(emitter, statement.GetOperand());
		emitter.AssignVariable();
		PushValue(emitter, statement.GetOperand(), curnamespace, codeblock);
	}

	void EmitPostOpStatement(ByteCodeEmitter& emitter, const IRSemantics::PostOpStatement& statement, const IRSemantics::CodeBlock& codeblock, const IRSemantics::Namespace& curnamespace)
	{
		// Yes, we need to push this twice! (Once, the value is passed on to the operator
		// itself for invocation; the second push [or rather the one which happens first,
		// and appears lower on the stack] is used to hold the initial value of the expression
		// so that the subsequent code can read off the value safely, in keeping with the
		// traditional semantics of a post operator.)
		PushValue(emitter, statement.GetOperand(), curnamespace, codeblock);
		PushValue(emitter, statement.GetOperand(), curnamespace, codeblock);

		emitter.Invoke(statement.GetOperatorName());
		BindReference(emitter, statement.GetOperand());
		emitter.AssignVariable();
	}

	bool EmitExpressionAtom(ByteCodeEmitter& emitter, const IRSemantics::ExpressionAtom* rawatom, const IRSemantics::CodeBlock& activescope, const IRSemantics::Namespace& curnamespace, bool firstmember)
	{
		if(!rawatom)
		{
			//
			// This is a failure of the AST traversal/IR generation.
			//
			// Expressions are composed of atoms in the final IR,
			// and a null atom should never occur. No-ops should
			// be represented by a special class of atom should
			// they be necessary.
			//
			throw InternalException("IR contains a null expression atom");
		}
		else if(const IRSemantics::ExpressionAtomParenthetical* atom = dynamic_cast<const IRSemantics::ExpressionAtomParenthetical*>(rawatom))
		{
			const IRSemantics::Parenthetical* parenthetical = atom->GetParenthetical();
			if(const IRSemantics::ParentheticalPreOp* preop = dynamic_cast<const IRSemantics::ParentheticalPreOp*>(parenthetical))
				EmitPreOpStatement(emitter, *preop->GetStatement(), activescope, curnamespace);
			else if(const IRSemantics::ParentheticalPostOp* postop = dynamic_cast<const IRSemantics::ParentheticalPostOp*>(parenthetical))
				EmitPostOpStatement(emitter, *postop->GetStatement(), activescope, curnamespace);
			else if(const IRSemantics::ParentheticalExpression* expr = dynamic_cast<const IRSemantics::ParentheticalExpression*>(parenthetical))
				EmitExpression(emitter, expr->GetExpression(), activescope, curnamespace);
			else
			{
				// TODO - document
				throw InternalException("Invalid parenthetical contents");
			}
		}
		else if(const IRSemantics::ExpressionAtomIdentifierReference* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifierReference*>(rawatom))
		{
			emitter.BindReference(atom->GetIdentifier());
		}
		else if(const IRSemantics::ExpressionAtomIdentifier* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifier*>(rawatom))
		{
			if(atom->GetEpochType(curnamespace) == Metadata::EpochType_Nothing)
			{
				// Do nothing!
			}
			else
			{
				if(curnamespace.Functions.Exists(atom->GetIdentifier()) || (curnamespace.Types.GetTypeByName(atom->GetIdentifier()) != Metadata::EpochType_Error))
					emitter.PushStringLiteral(atom->GetIdentifier());
				else
				{
					if(atom->GetEpochType(curnamespace) == Metadata::EpochType_Identifier || atom->GetEpochType(curnamespace) == Metadata::EpochType_Function)
						emitter.PushStringLiteral(atom->GetIdentifier());
					else
						emitter.PushVariableValue(atom->GetIdentifier(), activescope.GetVariableTypeByID(atom->GetIdentifier()));
				}
			}
		}
		else if(const IRSemantics::ExpressionAtomOperator* atom = dynamic_cast<const IRSemantics::ExpressionAtomOperator*>(rawatom))
		{
			emitter.Invoke(atom->GetIdentifier());
		}
		else if(const IRSemantics::ExpressionAtomLiteralString* atom = dynamic_cast<const IRSemantics::ExpressionAtomLiteralString*>(rawatom))
		{
			emitter.PushStringLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomLiteralBoolean* atom = dynamic_cast<const IRSemantics::ExpressionAtomLiteralBoolean*>(rawatom))
		{
			emitter.PushBooleanLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomLiteralInteger32* atom = dynamic_cast<const IRSemantics::ExpressionAtomLiteralInteger32*>(rawatom))
		{
			if(atom->GetEpochType(curnamespace) == Metadata::EpochType_Integer16)
				emitter.PushInteger16Literal(atom->GetValue());
			else
				emitter.PushIntegerLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomLiteralReal32* atom = dynamic_cast<const IRSemantics::ExpressionAtomLiteralReal32*>(rawatom))
		{
			emitter.PushRealLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomStatement* atom = dynamic_cast<const IRSemantics::ExpressionAtomStatement*>(rawatom))
		{
			EmitStatement(emitter, atom->GetStatement(), activescope, curnamespace);
		}
		else if(const IRSemantics::ExpressionAtomCopyFromStructure* atom = dynamic_cast<const IRSemantics::ExpressionAtomCopyFromStructure*>(rawatom))
		{
			emitter.CopyFromStructure(curnamespace.Strings.Find(L"identifier"), atom->GetMemberName());
		}
		else if(const IRSemantics::ExpressionAtomBindReference* atom = dynamic_cast<const IRSemantics::ExpressionAtomBindReference*>(rawatom))
		{
			if(firstmember)
				emitter.BindStructureReferenceByHandle(atom->GetIdentifier());
			else
				emitter.BindStructureReference(atom->GetIdentifier());
			return true;
		}
		else if(const IRSemantics::ExpressionAtomTypeAnnotation* atom = dynamic_cast<const IRSemantics::ExpressionAtomTypeAnnotation*>(rawatom))
		{
			emitter.PushTypeAnnotation(atom->GetEpochType(curnamespace));
		}
		else if(const IRSemantics::ExpressionAtomTypeAnnotationFromRegister* atom = dynamic_cast<const IRSemantics::ExpressionAtomTypeAnnotationFromRegister*>(rawatom))
		{
			emitter.TypeAnnotationFromRegister();
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

		return false;
	}

	void EmitExpression(ByteCodeEmitter& emitter, const IRSemantics::Expression& expression, const IRSemantics::CodeBlock& activescope, const IRSemantics::Namespace& curnamespace)
	{
		bool needsrefbind = false;
		const std::vector<IRSemantics::ExpressionAtom*>& rawatoms = expression.GetAtoms();
		for(std::vector<IRSemantics::ExpressionAtom*>::const_iterator iter = rawatoms.begin(); iter != rawatoms.end(); ++iter)
		{
			ByteBuffer atombuffer;
			ByteCodeEmitter atomemitter(atombuffer);
			bool thisatomneedsrefbind = EmitExpressionAtom(atomemitter, *iter, activescope, curnamespace, !needsrefbind);
			if(thisatomneedsrefbind)
				needsrefbind = true;
			else if(needsrefbind)
			{
				emitter.ReadReferenceOntoStack();
				needsrefbind = false;
			}

			emitter.EmitBuffer(atombuffer);
		}

		if(needsrefbind)
			emitter.ReadReferenceOntoStack();
	}

	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Namespace& curnamespace)
	{
		const std::vector<IRSemantics::Expression*>& params = statement.GetParameters();
		for(std::vector<IRSemantics::Expression*>::const_iterator paramiter = params.begin(); paramiter != params.end(); ++paramiter)
		{
			if(!(*paramiter)->AtomsArePatternMatchedLiteral)
				EmitExpression(emitter, **paramiter, activescope, curnamespace);
		}

		if(activescope.GetScope()->HasVariable(statement.GetName()) && activescope.GetScope()->GetVariableTypeByID(statement.GetName()) == Metadata::EpochType_Function)
			emitter.InvokeIndirect(statement.GetName());
		else if(curnamespace.Functions.FunctionNeedsDynamicPatternMatching(statement.GetName()))
			emitter.Invoke(curnamespace.Functions.GetDynamicPatternMatcherForFunction(statement.GetName()));
		else if(Metadata::GetTypeFamily(curnamespace.Types.GetTypeByName(statement.GetName())) == Metadata::EpochTypeFamily_SumType)
			emitter.ConstructSumType();
		else
			emitter.Invoke(statement.GetName());
	}

	void GenerateAssignment(ByteCodeEmitter& emitter, const IRSemantics::Assignment& assignment, const IRSemantics::Namespace& curnamespace, const IRSemantics::CodeBlock& activescope)
	{
		bool pushlhs = false;

		// TODO - eliminate these hard-coded string hacks
		if(curnamespace.Strings.GetPooledString(assignment.GetOperatorName()) != L"=")
			PushValue(emitter, assignment.GetLHS(), curnamespace, activescope);

		const IRSemantics::AssignmentChain* rhs = assignment.GetRHS();
		if(!rhs)
		{
			//
			// This is an IR generation failure. Assignments must
			// always have a valid RHS of some nature. Null pointers
			// are not legitimate occurrences here.
			//
			throw InternalException("Assignment detected with no right hand side");
		}
		else if(const IRSemantics::AssignmentChainExpression* rhsexpression = dynamic_cast<const IRSemantics::AssignmentChainExpression*>(rhs))
		{
			EmitExpression(emitter, rhsexpression->GetExpression(), activescope, curnamespace);
		}
		else if(const IRSemantics::AssignmentChainAssignment* rhsassignment = dynamic_cast<const IRSemantics::AssignmentChainAssignment*>(rhs))
		{
			GenerateAssignment(emitter, rhsassignment->GetAssignment(), curnamespace, activescope);
			pushlhs = true;
		}
		else
		{
			//
			// This is likely an incomplete language feature.
			//
			// Assignment right hand sides should be of a type
			// handled by one of the above detection checks. A
			// RHS which does not fit is either bogus or needs
			// to be implemented correctly.
			//
			throw InternalException("Assignment right hand side is not recognized in IR");
		}

		if(curnamespace.Strings.GetPooledString(assignment.GetOperatorName()) != L"=")
			emitter.Invoke(assignment.GetOperatorName());

		if(assignment.WantsTypeAnnotation)
			emitter.PushTypeAnnotation(assignment.GetRHS()->GetEpochType(curnamespace));

		BindReference(emitter, assignment.GetLHS());

		if(Metadata::GetTypeFamily(assignment.GetLHSType()) == Metadata::EpochTypeFamily_SumType)
			emitter.AssignSumTypeVariable();
		else
			emitter.AssignVariable();

		if(pushlhs)
		{
			BindReference(emitter, assignment.GetLHS());
			emitter.ReadReferenceOntoStack();
		}
	}

	void Generate(const IRSemantics::Entity& entity, const IRSemantics::Namespace& curnamespace, const IRSemantics::CodeBlock& activescope, ByteCodeEmitter& emitter)
	{
		if(!entity.GetPostfixIdentifier())
		{
			const std::vector<IRSemantics::Expression*>& params = entity.GetParameters();
			for(std::vector<IRSemantics::Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
				EmitExpression(emitter, **iter, activescope, curnamespace);
		}

		emitter.EnterEntity(curnamespace.GetEntityTag(entity.GetName()), curnamespace.FindLexicalScopeName(&entity.GetCode()));
		Generate(entity.GetCode(), curnamespace, emitter);

		if(entity.GetPostfixIdentifier())
		{
			const std::vector<IRSemantics::Expression*>& params = entity.GetParameters();
			for(std::vector<IRSemantics::Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
				EmitExpression(emitter, **iter, activescope, curnamespace);
			emitter.InvokeMetacontrol(curnamespace.GetEntityCloserTag(entity.GetPostfixIdentifier()));
		}

		emitter.ExitEntity();
	}

	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Namespace& curnamespace, ByteCodeEmitter& emitter)
	{
		const std::vector<IRSemantics::CodeBlockEntry*>& entries = codeblock.GetEntries();
		for(std::vector<IRSemantics::CodeBlockEntry*>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
		{
			const IRSemantics::CodeBlockEntry* baseentry = *iter;
			
			if(!baseentry)
			{
				//
				// This is a failure of IR generation.
				//
				// Code blocks should never contain pointers which
				// are null. Ensure that the entry is being parsed
				// correctly and that IR generation is implemented
				// for any associated language features.
				//
				throw InternalException("Code block contains a null entry");
			}
			else if(const IRSemantics::CodeBlockAssignmentEntry* entry = dynamic_cast<const IRSemantics::CodeBlockAssignmentEntry*>(baseentry))
			{
				GenerateAssignment(emitter, entry->GetAssignment(), curnamespace, codeblock);
			}
			else if(const IRSemantics::CodeBlockStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockStatementEntry*>(baseentry))
			{
				EmitStatement(emitter, entry->GetStatement(), codeblock, curnamespace);
				Metadata::EpochTypeID rettype = entry->GetStatement().GetEpochType(curnamespace);
				if(rettype != Metadata::EpochType_Void)
					emitter.PopStack(rettype);
			}
			else if(const IRSemantics::CodeBlockPreOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPreOpStatementEntry*>(baseentry))
			{
				EmitPreOpStatement(emitter, entry->GetStatement(), codeblock, curnamespace);
				emitter.PopStack(entry->GetStatement().GetEpochType(curnamespace));
			}
			else if(const IRSemantics::CodeBlockPostOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPostOpStatementEntry*>(baseentry))
			{
				EmitPostOpStatement(emitter, entry->GetStatement(), codeblock, curnamespace);
				emitter.PopStack(entry->GetStatement().GetEpochType(curnamespace));
			}
			else if(const IRSemantics::CodeBlockInnerBlockEntry* entry = dynamic_cast<const IRSemantics::CodeBlockInnerBlockEntry*>(baseentry))
			{
				StringHandle anonymousnamehandle = curnamespace.FindLexicalScopeName(&entry->GetCode());

				emitter.EnterEntity(Bytecode::EntityTags::FreeBlock, anonymousnamehandle);
				Generate(entry->GetCode(), curnamespace, emitter);
				emitter.ExitEntity();
			}
			else if(const IRSemantics::CodeBlockEntityEntry* entry = dynamic_cast<const IRSemantics::CodeBlockEntityEntry*>(baseentry))
			{
				emitter.BeginChain();
				Generate(entry->GetEntity(), curnamespace, codeblock, emitter);

				const std::vector<IRSemantics::Entity*>& chain = entry->GetEntity().GetChain();
				for(std::vector<IRSemantics::Entity*>::const_iterator chainiter = chain.begin(); chainiter != chain.end(); ++chainiter)
					Generate(**chainiter, curnamespace, codeblock, emitter);

				emitter.EndChain();
			}
			else
			{
				//
				// This is probably a missing language feature.
				//
				// IR generation has produced an entry in a code block
				// which is not recognized by any of the above detection
				// checks. Ensure that the IR generator is correct and
				// that any associated features are implemented above.
				//
				throw InternalException("Unrecognized entry in code block");
			}
		}
	}

	void EmitConstructor(ByteCodeEmitter& emitter, StringHandle name, StringHandle rawname, const IRSemantics::Structure& structure, const CompileTimeParameterVector& templateargs, const IRSemantics::Namespace& curnamespace)
	{
		emitter.DefineLexicalScope(name, 0, structure.GetMembers().size() + 1);
		emitter.LexicalScopeEntry(curnamespace.Strings.Find(L"identifier"), Metadata::EpochType_Identifier, true, VARIABLE_ORIGIN_PARAMETER);
		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
		{
			Metadata::EpochTypeID membertype = structure.GetMembers()[i].second->GetEpochType(curnamespace);
			if(structure.IsTemplate() && !templateargs.empty())
				membertype = structure.SubstituteTemplateParams(structure.GetMembers()[i].first, templateargs, curnamespace);

			emitter.LexicalScopeEntry(structure.GetMembers()[i].first, membertype, false, VARIABLE_ORIGIN_PARAMETER);
		}

		emitter.EnterFunction(name);
		emitter.AllocateStructure(curnamespace.Types.GetTypeByName(rawname));
		emitter.BindReference(curnamespace.Strings.Find(L"identifier"));
		emitter.AssignVariable();

		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
		{
			Metadata::EpochTypeID membertype = structure.GetMembers()[i].second->GetEpochType(curnamespace);
			if(structure.IsTemplate() && !templateargs.empty())
				membertype = structure.SubstituteTemplateParams(structure.GetMembers()[i].first, templateargs, curnamespace);

			emitter.PushVariableValue(structure.GetMembers()[i].first, membertype);
			emitter.AssignStructure(curnamespace.Strings.Find(L"identifier"), structure.GetMembers()[i].first);
		}

		emitter.ExitFunction();
	}

	void EmitAnonConstructor(ByteCodeEmitter& emitter, StringHandle name, StringHandle rawname, const IRSemantics::Structure& structure, const CompileTimeParameterVector& templateargs, const IRSemantics::Namespace& curnamespace)
	{
		emitter.DefineLexicalScope(name, 0, structure.GetMembers().size() + 1);
		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
		{
			Metadata::EpochTypeID membertype = structure.GetMembers()[i].second->GetEpochType(curnamespace);
			if(structure.IsTemplate() && !templateargs.empty())
				membertype = structure.SubstituteTemplateParams(structure.GetMembers()[i].first, templateargs, curnamespace);

			emitter.LexicalScopeEntry(structure.GetMembers()[i].first, membertype, false, VARIABLE_ORIGIN_PARAMETER);
		}
		emitter.LexicalScopeEntry(name, curnamespace.Types.GetTypeByName(rawname), false, VARIABLE_ORIGIN_RETURN);

		emitter.EnterFunction(name);
		emitter.AllocateStructure(curnamespace.Types.GetTypeByName(rawname));
		emitter.BindReference(name);
		emitter.AssignVariable();

		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
		{
			Metadata::EpochTypeID membertype = structure.GetMembers()[i].second->GetEpochType(curnamespace);
			if(structure.IsTemplate() && !templateargs.empty())
				membertype = structure.SubstituteTemplateParams(structure.GetMembers()[i].first, templateargs, curnamespace);

			emitter.PushVariableValue(structure.GetMembers()[i].first, membertype);
			emitter.AssignStructure(name, structure.GetMembers()[i].first);
		}

		emitter.SetReturnRegister(name);
		emitter.ExitFunction();
	}

	bool Generate(const IRSemantics::Namespace& curnamespace, ByteCodeEmitter& emitter)
	{
		std::map<StringHandle, bool> isconstructor;

		std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> > sumtypes = curnamespace.Types.SumTypes.GetDefinitions();
		for(std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> >::const_iterator iter = sumtypes.begin(); iter != sumtypes.end(); ++iter)
		{
			// TODO - omit template types
			emitter.DefineSumType(iter->first, iter->second);
		}


		DependencyGraph<Metadata::EpochTypeID> structuredependencies;
		std::map<Metadata::EpochTypeID, IRSemantics::Structure*> typemap;

		const std::map<StringHandle, IRSemantics::Structure*>& structures = curnamespace.Types.Structures.GetDefinitions();
		for(std::map<StringHandle, IRSemantics::Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			Metadata::EpochTypeID type = curnamespace.Types.GetTypeByName(iter->first);
			structuredependencies.Register(type);

			typemap.insert(std::make_pair(type, iter->second));

			const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = iter->second->GetMembers();
			for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
			{
				Metadata::EpochTypeID membertype = memberiter->second->GetEpochType(curnamespace);
				if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_TemplateInstance)
					structuredependencies.AddDependency(type, membertype);
			}
		}

		std::map<Metadata::EpochTypeID, const CompileTimeParameterVector*> templateargmap;

		const IRSemantics::InstantiationMap& templateinsts = curnamespace.Types.Templates.GetInstantiations();
		for(IRSemantics::InstantiationMap::const_iterator iter = templateinsts.begin(); iter != templateinsts.end(); ++iter)
		{
			IRSemantics::Structure& structure = *structures.find(iter->first)->second;
			if(!structure.IsTemplate())
				continue;

			const IRSemantics::InstancesAndArguments& instances = iter->second;
			for(IRSemantics::InstancesAndArguments::const_iterator institer = instances.begin(); institer != instances.end(); ++institer)
			{
				Metadata::EpochTypeID type = curnamespace.Types.GetTypeByName(institer->first);
				structuredependencies.Register(type);

				typemap.insert(std::make_pair(type, &structure));
				templateargmap.insert(std::make_pair(type, &institer->second));

				const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = structure.GetMembers();
				for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
				{
					Metadata::EpochTypeID membertype = structure.SubstituteTemplateParams(memberiter->first, institer->second, curnamespace);
					if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_TemplateInstance)
						structuredependencies.AddDependency(type, membertype);
				}
			}
		}

		std::vector<Metadata::EpochTypeID> typeorder = structuredependencies.Resolve();
		for(std::vector<Metadata::EpochTypeID>::const_iterator iter = typeorder.begin(); iter != typeorder.end(); ++iter)
		{
			const IRSemantics::Structure* structure = typemap.find(*iter)->second;
			const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = structure->GetMembers();

			emitter.DefineStructure(*iter, members.size());
			for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
			{
				if(structure->IsTemplate())
				{
					const CompileTimeParameterVector& args = *templateargmap.find(*iter)->second;
					emitter.StructureMember(memberiter->first, structure->SubstituteTemplateParams(memberiter->first, args, curnamespace));
				}
				else
					emitter.StructureMember(memberiter->first, memberiter->second->GetEpochType(curnamespace));
			}
		}

		const boost::unordered_map<StringHandle, IRSemantics::Function*>& functions = curnamespace.Functions.GetDefinitions();
		for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			const std::vector<IRSemantics::FunctionTag>& tags = iter->second->GetTags();
			for(std::vector<IRSemantics::FunctionTag>::const_iterator tagiter = tags.begin(); tagiter != tags.end(); ++tagiter)
			{
				if(curnamespace.Strings.GetPooledString(tagiter->TagName) == L"constructor")
					isconstructor[iter->first] = true;

				if(curnamespace.FunctionTags.Exists(tagiter->TagName))
				{
					TagHelperReturn help = curnamespace.FunctionTags.GetHelper(tagiter->TagName)(iter->first, tagiter->Parameters, true);
					if(!help.MetaTag.empty())
						emitter.TagData(iter->first, help.MetaTag, help.MetaTagData);
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
		}


		const IRSemantics::ScopePtrMap& scopes = curnamespace.GetScopes();
		DependencyGraph<StringHandle> scopedependencies;
		for(IRSemantics::ScopePtrMap::const_iterator iter = scopes.begin(); iter != scopes.end(); ++iter)
		{
			scopedependencies.Register(iter->first);
			if(iter->second->ParentScope && curnamespace.FindLexicalScopeName(iter->second->ParentScope))
				scopedependencies.AddDependency(iter->first, curnamespace.FindLexicalScopeName(iter->second->ParentScope));
		}

		std::vector<StringHandle> scopeorder = scopedependencies.Resolve();
		for(std::vector<StringHandle>::const_iterator orderiter = scopeorder.begin(); orderiter != scopeorder.end(); ++orderiter)
		{
			IRSemantics::ScopePtrMap::const_iterator iter = scopes.find(*orderiter);
			emitter.DefineLexicalScope(iter->first, curnamespace.FindLexicalScopeName(iter->second->ParentScope), iter->second->GetVariableCount());
			for(size_t i = 0; i < iter->second->GetVariableCount(); ++i)
			{
				VariableOrigin origin = iter->second->GetVariableOrigin(i);
				if(isconstructor[*orderiter] && origin == VARIABLE_ORIGIN_RETURN)
					origin = VARIABLE_ORIGIN_LOCAL;

				Metadata::EpochTypeID vartype = iter->second->GetVariableTypeByIndex(i);
				if(Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Unit)
					vartype = curnamespace.Types.Aliases.GetStrongRepresentation(vartype);
				emitter.LexicalScopeEntry(curnamespace.Strings.Find(iter->second->GetVariableName(i)), vartype, iter->second->IsReference(i), origin);
			}
		}


		size_t numglobalblocks = curnamespace.GetNumGlobalCodeBlocks();
		for(size_t i = 0; i < numglobalblocks; ++i)
		{
			emitter.EnterEntity(Bytecode::EntityTags::Globals, curnamespace.FindLexicalScopeName(curnamespace.GetGlobalCodeBlock(i).GetScope()));
			Generate(curnamespace.GetGlobalCodeBlock(i), curnamespace, emitter);
		}

		emitter.Invoke(curnamespace.Strings.Find(L"entrypoint"));
		emitter.Halt();
	
		for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			emitter.EnterFunction(iter->first);
			Metadata::EpochTypeID rettype = iter->second->GetReturnType(curnamespace);
			if(rettype != Metadata::EpochType_Void || isconstructor[iter->first])
				EmitExpression(emitter, *iter->second->GetReturnExpression(), *iter->second->GetCode(), curnamespace);

			const std::vector<IRSemantics::FunctionTag>& tags = iter->second->GetTags();
			for(std::vector<IRSemantics::FunctionTag>::const_iterator tagiter = tags.begin(); tagiter != tags.end(); ++tagiter)
			{
				if(curnamespace.FunctionTags.Exists(tagiter->TagName))
				{
					TagHelperReturn help = curnamespace.FunctionTags.GetHelper(tagiter->TagName)(iter->first, tagiter->Parameters, false);
					if(!help.InvokeRuntimeFunction.empty())
						emitter.Invoke(curnamespace.Strings.Find(help.InvokeRuntimeFunction));
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

			const IRSemantics::CodeBlock* code = iter->second->GetCode();
			if(code)
				Generate(*code, curnamespace, emitter);

			if(iter->second->GetReturnType(curnamespace) != Metadata::EpochType_Void || isconstructor[iter->first])
			{
				if(!code)
				{
					//
					// This is probably a bug in the IR generation.
					//
					// At the IR level, all functions have code bodies, even if
					// they are empty. This is done to support the lexical scope
					// data needed for parameters and return values to exist on
					// the stack when invoking such a function.
					//
					// Most of the code generation logic assumes that this is
					// upheld by the IR layer, so a missing code body is a fatal
					// failure at this point.
					//
					throw InternalException("Function has no attached code body");
				}

				if(!iter->second->IsReturnRegisterSuppressed())
				{
					const ScopeDescription* scope = code->GetScope();
					for(size_t i = 0; i < scope->GetVariableCount(); ++i)
					{
						if(scope->GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
						{
							if(isconstructor[iter->first])
							{
								emitter.PushVariableValueNoCopy(scope->GetVariableNameHandle(i));
								emitter.PushVariableValue(curnamespace.Strings.Find(L"@id"), Metadata::EpochType_Identifier);
								emitter.PushIntegerLiteral(scope->GetVariableTypeByIndex(i));
								emitter.AssignVariableThroughIdentifier();
							}
							else
							{
								StringHandle varname = scope->GetVariableNameHandle(i);

								if(iter->second->HasAnonymousReturn())
								{
									emitter.BindReference(varname);
									emitter.AssignVariable();
								}

								emitter.SetReturnRegister(varname);
							}
							break;
						}
					}
				}
			}

			emitter.ExitFunction();
		}

		// Generate constructors for structures
		for(std::map<StringHandle, IRSemantics::Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
		{
			if(iter->second->IsTemplate())
				continue;

			EmitConstructor(emitter, iter->second->GetConstructorName(), iter->first, *iter->second, CompileTimeParameterVector(), curnamespace);
			EmitAnonConstructor(emitter, iter->second->GetAnonymousConstructorName(), iter->first, *iter->second, CompileTimeParameterVector(), curnamespace);
		}

		for(IRSemantics::InstantiationMap::const_iterator iter = templateinsts.begin(); iter != templateinsts.end(); ++iter)
		{
			IRSemantics::Structure& structure = *structures.find(iter->first)->second;
			if(!structure.IsTemplate())
				continue;

			const IRSemantics::InstancesAndArguments& instances = iter->second;
			for(IRSemantics::InstancesAndArguments::const_iterator institer = instances.begin(); institer != instances.end(); ++institer)
			{
				EmitConstructor(emitter, curnamespace.Types.Templates.FindConstructorName(institer->first), institer->first, structure, institer->second, curnamespace);
				EmitAnonConstructor(emitter, curnamespace.Types.Templates.FindAnonConstructorName(institer->first), institer->first, structure, institer->second, curnamespace);
			}
		}


		const std::set<StringHandle>& funcsneedingpatternmatching = curnamespace.Functions.GetFunctionsNeedingDynamicPatternMatching();
		for(std::set<StringHandle>::const_iterator iter = funcsneedingpatternmatching.begin(); iter != funcsneedingpatternmatching.end(); ++iter)
		{
			const IRSemantics::Function* thisfunc = functions.find(*iter)->second;
			const FunctionSignature thisfuncsig = thisfunc->GetFunctionSignature(curnamespace);
			StringHandle patternmatchername = curnamespace.Functions.GetDynamicPatternMatcherForFunction(*iter);
			emitter.EnterPatternResolver(patternmatchername);
			size_t numoverloads = curnamespace.Functions.GetNumOverloads(thisfunc->GetRawName());
			for(size_t pass = 0; pass < 2; ++pass)
			{
				bool preferliteralmatches = (pass == 0);

				for(size_t i = 0; i < numoverloads; ++i)
				{
					StringHandle overloadname = curnamespace.Functions.GetOverloadName(thisfunc->GetRawName(), i);
					const IRSemantics::Function* thisoverload = functions.find(overloadname)->second;
					const FunctionSignature overloadsig = thisoverload->GetFunctionSignature(curnamespace);
					if(overloadsig.MatchesDynamicPattern(thisfuncsig))
					{
						bool hasliterals = false;
						for(size_t j = 0; j < overloadsig.GetNumParameters(); ++j)
						{
							if(overloadsig.GetParameter(j).HasPayload)
							{
								hasliterals = true;
								break;
							}
						}

						if(hasliterals != preferliteralmatches)
							continue;

						emitter.ResolvePattern(overloadname, overloadsig);
					}
				}
			}
			emitter.ExitPatternResolver();
			emitter.DefineLexicalScope(patternmatchername, 0, 0);
		}

		const std::map<StringHandle, std::map<StringHandle, FunctionSignature> >& requiredtypematchers = curnamespace.Functions.GetRequiredTypeMatchers();
		for(std::map<StringHandle, std::map<StringHandle, FunctionSignature> >::const_iterator iter = requiredtypematchers.begin(); iter != requiredtypematchers.end(); ++iter)
		{
			emitter.DefineLexicalScope(iter->first, 0, 0);
			emitter.EnterTypeResolver(iter->first);
			for(std::map<StringHandle, FunctionSignature>::const_iterator overloaditer = iter->second.begin(); overloaditer != iter->second.end(); ++overloaditer)
			{
				const FunctionSignature& sig = overloaditer->second;
				emitter.ResolveTypes(overloaditer->first, sig);
			}
			emitter.ExitTypeResolver();
		}

		for(size_t i = 0; i < numglobalblocks; ++i)
			emitter.ExitEntity();

		return true;
	}
}


bool CompilerPasses::GenerateCode(const IRSemantics::Program& program, ByteCodeEmitter& emitter)
{
	const StringPoolManager& strings = program.GetStringPool();
	const boost::unordered_map<StringHandle, std::wstring>& stringpool = strings.GetInternalPool();

	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = stringpool.begin(); iter != stringpool.end(); ++iter)
		emitter.PoolString(iter->first, iter->second);

	return Generate(program.GlobalNamespace, emitter);
}

