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
	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program, ByteCodeEmitter& emitter);
	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program);
	void EmitExpression(ByteCodeEmitter& emitter, const IRSemantics::Expression& expression, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program);

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

	void PushValue(ByteCodeEmitter& emitter, const std::vector<StringHandle>& identifiers, const IRSemantics::Program& program, const IRSemantics::CodeBlock& activescope)
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

			VM::EpochTypeID structuretype = activescope.GetVariableTypeByID(identifiers[0]);
			for(size_t i = 1; i < identifiers.size(); ++i)
			{
				StringHandle structurename = program.GetNameOfStructureType(structuretype);
				StringHandle overloadidhandle = program.FindStructureMemberAccessOverload(structurename, identifiers[i]);

				emitter.PushStringLiteral(identifiers[i]);
				emitter.Invoke(overloadidhandle);

				structuretype = program.GetStructureMemberType(structurename, identifiers[i]);
			}

			if(structuretype == VM::EpochType_Buffer)
				emitter.CopyBuffer();
			else if(structuretype > VM::EpochType_CustomBase)
				emitter.CopyStructure();
		}
	}

	void EmitPreOpStatement(ByteCodeEmitter& emitter, const IRSemantics::PreOpStatement& statement, const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program)
	{
		PushValue(emitter, statement.GetOperand(), program, codeblock);
		emitter.Invoke(statement.GetOperatorName());
		BindReference(emitter, statement.GetOperand());
		emitter.AssignVariable();
		PushValue(emitter, statement.GetOperand(), program, codeblock);
	}

	void EmitPostOpStatement(ByteCodeEmitter& emitter, const IRSemantics::PostOpStatement& statement, const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program)
	{
		// Yes, we need to push this twice! (Once, the value is passed on to the operator
		// itself for invocation; the second push [or rather the one which happens first,
		// and appears lower on the stack] is used to hold the initial value of the expression
		// so that the subsequent code can read off the value safely, in keeping with the
		// traditional semantics of a post operator.)
		PushValue(emitter, statement.GetOperand(), program, codeblock);
		PushValue(emitter, statement.GetOperand(), program, codeblock);

		emitter.Invoke(statement.GetOperatorName());
		BindReference(emitter, statement.GetOperand());
		emitter.AssignVariable();
	}

	bool EmitExpressionAtom(ByteCodeEmitter& emitter, const IRSemantics::ExpressionAtom* rawatom, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program, bool firstmember)
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
				EmitPreOpStatement(emitter, *preop->GetStatement(), activescope, program);
			else if(const IRSemantics::ParentheticalPostOp* postop = dynamic_cast<const IRSemantics::ParentheticalPostOp*>(parenthetical))
				EmitPostOpStatement(emitter, *postop->GetStatement(), activescope, program);
			else if(const IRSemantics::ParentheticalExpression* expr = dynamic_cast<const IRSemantics::ParentheticalExpression*>(parenthetical))
				EmitExpression(emitter, expr->GetExpression(), activescope, program);
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
			if(program.HasFunction(atom->GetIdentifier()) || (program.LookupType(atom->GetIdentifier()) != VM::EpochType_Error))
				emitter.PushStringLiteral(atom->GetIdentifier());
			else
			{
				if(atom->GetEpochType(program) == VM::EpochType_Identifier || atom->GetEpochType(program) == VM::EpochType_Function)
					emitter.PushStringLiteral(atom->GetIdentifier());
				else
					emitter.PushVariableValue(atom->GetIdentifier(), activescope.GetVariableTypeByID(atom->GetIdentifier()));
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
			emitter.PushIntegerLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomLiteralReal32* atom = dynamic_cast<const IRSemantics::ExpressionAtomLiteralReal32*>(rawatom))
		{
			emitter.PushRealLiteral(atom->GetValue());
		}
		else if(const IRSemantics::ExpressionAtomStatement* atom = dynamic_cast<const IRSemantics::ExpressionAtomStatement*>(rawatom))
		{
			EmitStatement(emitter, atom->GetStatement(), activescope, program);
		}
		else if(const IRSemantics::ExpressionAtomCopyFromStructure* atom = dynamic_cast<const IRSemantics::ExpressionAtomCopyFromStructure*>(rawatom))
		{
			emitter.CopyFromStructure(program.FindString(L"identifier"), atom->GetMemberName());
		}
		else if(const IRSemantics::ExpressionAtomBindReference* atom = dynamic_cast<const IRSemantics::ExpressionAtomBindReference*>(rawatom))
		{
			if(firstmember)
				emitter.BindStructureReferenceByHandle(atom->GetIdentifier());
			else
				emitter.BindStructureReference(atom->GetIdentifier());
			return true;
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

	void EmitExpression(ByteCodeEmitter& emitter, const IRSemantics::Expression& expression, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program)
	{
		bool needsrefbind = false;
		const std::vector<IRSemantics::ExpressionAtom*>& rawatoms = expression.GetAtoms();
		for(std::vector<IRSemantics::ExpressionAtom*>::const_iterator iter = rawatoms.begin(); iter != rawatoms.end(); ++iter)
		{
			ByteBuffer atombuffer;
			ByteCodeEmitter atomemitter(atombuffer);
			bool thisatomneedsrefbind = EmitExpressionAtom(atomemitter, *iter, activescope, program, !needsrefbind);
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

	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program)
	{
		const std::vector<IRSemantics::Expression*>& params = statement.GetParameters();
		for(std::vector<IRSemantics::Expression*>::const_iterator paramiter = params.begin(); paramiter != params.end(); ++paramiter)
		{
			if(!(*paramiter)->AtomsArePatternMatchedLiteral)
				EmitExpression(emitter, **paramiter, activescope, program);
		}

		if(activescope.GetScope()->HasVariable(statement.GetName()) && activescope.GetScope()->GetVariableTypeByID(statement.GetName()) == VM::EpochType_Function)
			emitter.InvokeIndirect(statement.GetName());
		else if(program.FunctionNeedsDynamicPatternMatching(statement.GetName()))
			emitter.Invoke(program.GetDynamicPatternMatcherForFunction(statement.GetName()));
		else
			emitter.Invoke(statement.GetName());
	}

	void GenerateAssignment(ByteCodeEmitter& emitter, const IRSemantics::Assignment& assignment, const IRSemantics::Program& program, const IRSemantics::CodeBlock& activescope)
	{
		bool pushlhs = false;

		if(program.GetString(assignment.GetOperatorName()) != L"=")
			PushValue(emitter, assignment.GetLHS(), program, activescope);

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
			EmitExpression(emitter, rhsexpression->GetExpression(), activescope, program);
		}
		else if(const IRSemantics::AssignmentChainAssignment* rhsassignment = dynamic_cast<const IRSemantics::AssignmentChainAssignment*>(rhs))
		{
			GenerateAssignment(emitter, rhsassignment->GetAssignment(), program, activescope);
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

		if(program.GetString(assignment.GetOperatorName()) != L"=")
			emitter.Invoke(assignment.GetOperatorName());

		BindReference(emitter, assignment.GetLHS());
		emitter.AssignVariable();

		if(pushlhs)
		{
			BindReference(emitter, assignment.GetLHS());
			emitter.ReadReferenceOntoStack();
		}
	}

	void Generate(const IRSemantics::Entity& entity, const IRSemantics::Program& program, const IRSemantics::CodeBlock& activescope, ByteCodeEmitter& emitter)
	{
		if(!entity.GetPostfixIdentifier())
		{
			const std::vector<IRSemantics::Expression*>& params = entity.GetParameters();
			for(std::vector<IRSemantics::Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
				EmitExpression(emitter, **iter, activescope, program);
		}

		emitter.EnterEntity(program.GetEntityTag(entity.GetName()), program.FindLexicalScopeName(&entity.GetCode()));
		Generate(entity.GetCode(), program, emitter);

		if(entity.GetPostfixIdentifier())
		{
			const std::vector<IRSemantics::Expression*>& params = entity.GetParameters();
			for(std::vector<IRSemantics::Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
				EmitExpression(emitter, **iter, activescope, program);
			emitter.InvokeMetacontrol(program.GetEntityCloserTag(entity.GetPostfixIdentifier()));
		}

		emitter.ExitEntity();
	}

	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program, ByteCodeEmitter& emitter)
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
				GenerateAssignment(emitter, entry->GetAssignment(), program, codeblock);
			}
			else if(const IRSemantics::CodeBlockStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockStatementEntry*>(baseentry))
			{
				EmitStatement(emitter, entry->GetStatement(), codeblock, program);
				VM::EpochTypeID rettype = entry->GetStatement().GetEpochType(program);
				if(rettype != VM::EpochType_Void)
					emitter.PopStack(rettype);
			}
			else if(const IRSemantics::CodeBlockPreOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPreOpStatementEntry*>(baseentry))
			{
				EmitPreOpStatement(emitter, entry->GetStatement(), codeblock, program);
				emitter.PopStack(entry->GetStatement().GetEpochType(program));
			}
			else if(const IRSemantics::CodeBlockPostOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPostOpStatementEntry*>(baseentry))
			{
				EmitPostOpStatement(emitter, entry->GetStatement(), codeblock, program);
				emitter.PopStack(entry->GetStatement().GetEpochType(program));
			}
			else if(const IRSemantics::CodeBlockInnerBlockEntry* entry = dynamic_cast<const IRSemantics::CodeBlockInnerBlockEntry*>(baseentry))
			{
				StringHandle anonymousnamehandle = program.FindLexicalScopeName(&entry->GetCode());

				emitter.EnterEntity(Bytecode::EntityTags::FreeBlock, anonymousnamehandle);
				Generate(entry->GetCode(), program, emitter);
				emitter.ExitEntity();
			}
			else if(const IRSemantics::CodeBlockEntityEntry* entry = dynamic_cast<const IRSemantics::CodeBlockEntityEntry*>(baseentry))
			{
				emitter.BeginChain();
				Generate(entry->GetEntity(), program, codeblock, emitter);

				const std::vector<IRSemantics::Entity*>& chain = entry->GetEntity().GetChain();
				for(std::vector<IRSemantics::Entity*>::const_iterator chainiter = chain.begin(); chainiter != chain.end(); ++chainiter)
					Generate(**chainiter, program, codeblock, emitter);

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

	void EmitConstructor(ByteCodeEmitter& emitter, StringHandle name, const IRSemantics::Structure& structure, const IRSemantics::Program& program)
	{
		emitter.DefineLexicalScope(name, 0, structure.GetMembers().size() + 1);
		emitter.LexicalScopeEntry(program.FindString(L"identifier"), VM::EpochType_Identifier, true, VARIABLE_ORIGIN_PARAMETER);
		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
			emitter.LexicalScopeEntry(structure.GetMembers()[i].first, structure.GetMembers()[i].second->GetEpochType(program), false, VARIABLE_ORIGIN_PARAMETER);

		emitter.EnterFunction(name);
		emitter.AllocateStructure(program.LookupType(name));
		emitter.BindReference(program.FindString(L"identifier"));
		emitter.AssignVariable();

		for(size_t i = 0; i < structure.GetMembers().size(); ++i)
		{
			emitter.PushVariableValue(structure.GetMembers()[i].first, structure.GetMembers()[i].second->GetEpochType(program));
			emitter.AssignStructure(program.FindString(L"identifier"), structure.GetMembers()[i].first);
		}

		emitter.ExitFunction();
	}
}


bool CompilerPasses::GenerateCode(const IRSemantics::Program& program, ByteCodeEmitter& emitter)
{
	const StringPoolManager& strings = program.GetStringPool();
	const boost::unordered_map<StringHandle, std::wstring>& stringpool = strings.GetInternalPool();

	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = stringpool.begin(); iter != stringpool.end(); ++iter)
		emitter.PoolString(iter->first, iter->second);

	DependencyGraph<VM::EpochTypeID> structuredependencies;
	std::map<VM::EpochTypeID, IRSemantics::Structure*> typemap;

	const std::map<StringHandle, IRSemantics::Structure*>& structures = program.GetStructures();
	for(std::map<StringHandle, IRSemantics::Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
	{
		VM::EpochTypeID type = program.LookupType(iter->first);
		structuredependencies.Register(type);

		typemap.insert(std::make_pair(type, iter->second));

		const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = iter->second->GetMembers();
		for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
		{
			VM::EpochTypeID membertype = memberiter->second->GetEpochType(program);
			if(membertype > VM::EpochType_CustomBase)
				structuredependencies.AddDependency(type, membertype);
		}
	}

	std::vector<VM::EpochTypeID> typeorder = structuredependencies.Resolve();
	for(std::vector<VM::EpochTypeID>::const_iterator iter = typeorder.begin(); iter != typeorder.end(); ++iter)
	{
		const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = typemap.find(*iter)->second->GetMembers();

		emitter.DefineStructure(*iter, members.size());
		for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
			emitter.StructureMember(memberiter->first, memberiter->second->GetEpochType(program));
	}


	const IRSemantics::ScopePtrMap& scopes = program.GetScopes();
	DependencyGraph<StringHandle> scopedependencies;
	for(IRSemantics::ScopePtrMap::const_iterator iter = scopes.begin(); iter != scopes.end(); ++iter)
	{
		scopedependencies.Register(iter->first);
		if(iter->second->ParentScope && program.FindLexicalScopeName(iter->second->ParentScope))
			scopedependencies.AddDependency(iter->first, program.FindLexicalScopeName(iter->second->ParentScope));
	}

	std::vector<StringHandle> scopeorder = scopedependencies.Resolve();
	for(std::vector<StringHandle>::const_iterator orderiter = scopeorder.begin(); orderiter != scopeorder.end(); ++orderiter)
	{
		IRSemantics::ScopePtrMap::const_iterator iter = scopes.find(*orderiter);
		emitter.DefineLexicalScope(iter->first, program.FindLexicalScopeName(iter->second->ParentScope), iter->second->GetVariableCount());
		for(size_t i = 0; i < iter->second->GetVariableCount(); ++i)
			emitter.LexicalScopeEntry(strings.Find(iter->second->GetVariableName(i)), iter->second->GetVariableTypeByIndex(i), iter->second->IsReference(i), iter->second->GetVariableOrigin(i));
	}


	const boost::unordered_map<StringHandle, IRSemantics::Function*>& functions = program.GetFunctions();
	for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
	{
		const std::vector<IRSemantics::FunctionTag>& tags = iter->second->GetTags();
		for(std::vector<IRSemantics::FunctionTag>::const_iterator tagiter = tags.begin(); tagiter != tags.end(); ++tagiter)
		{
			FunctionTagHelperTable::const_iterator helperiter = program.Session.FunctionTagHelpers.find(program.GetString(tagiter->TagName));
			if(helperiter != program.Session.FunctionTagHelpers.end())
			{
				TagHelperReturn help = helperiter->second(iter->first, tagiter->Parameters, true);
				emitter.TagData(iter->first, help.MetaTag, help.MetaTagData);
			}
			else
			{
				// TODO - flag semantic error instead
				throw std::runtime_error("Unrecognized function tag");
			}
		}
	}


	size_t numglobalblocks = program.GetNumGlobalCodeBlocks();
	for(size_t i = 0; i < numglobalblocks; ++i)
	{
		emitter.EnterEntity(Bytecode::EntityTags::Globals, program.FindLexicalScopeName(program.GetGlobalCodeBlock(i).GetScope()));
		Generate(program.GetGlobalCodeBlock(i), program, emitter);
	}

	emitter.Invoke(strings.Find(L"entrypoint"));
	emitter.Halt();
	
	for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
	{
		emitter.EnterFunction(iter->first);
		if(iter->second->GetReturnType(program) != VM::EpochType_Void)
			EmitExpression(emitter, *iter->second->GetReturnExpression(), *iter->second->GetCode(), program);

		const std::vector<IRSemantics::FunctionTag>& tags = iter->second->GetTags();
		for(std::vector<IRSemantics::FunctionTag>::const_iterator tagiter = tags.begin(); tagiter != tags.end(); ++tagiter)
		{
			FunctionTagHelperTable::const_iterator helperiter = program.Session.FunctionTagHelpers.find(program.GetString(tagiter->TagName));
			if(helperiter != program.Session.FunctionTagHelpers.end())
			{
				TagHelperReturn help = helperiter->second(iter->first, tagiter->Parameters, false);
				if(!help.InvokeRuntimeFunction.empty())
					emitter.Invoke(program.FindString(help.InvokeRuntimeFunction));
			}
			else
			{
				// TODO - flag semantic error instead
				throw std::runtime_error("Unrecognized function tag");
			}
		}

		const IRSemantics::CodeBlock* code = iter->second->GetCode();
		if(code)
			Generate(*code, program, emitter);

		if(iter->second->GetReturnType(program) != VM::EpochType_Void)
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
						emitter.SetReturnRegister(scope->GetVariableNameHandle(i));
						break;
					}
				}
			}
		}

		emitter.ExitFunction();
	}

	// Generate constructors for structures
	// TODO - implement anonymous temporaries
	for(std::map<StringHandle, IRSemantics::Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
		EmitConstructor(emitter, iter->first, *iter->second, program);

	const std::set<StringHandle>& funcsneedingpatternmatching = program.GetFunctionsNeedingDynamicPatternMatching();
	for(std::set<StringHandle>::const_iterator iter = funcsneedingpatternmatching.begin(); iter != funcsneedingpatternmatching.end(); ++iter)
	{
		const IRSemantics::Function* thisfunc = functions.find(*iter)->second;
		const FunctionSignature thisfuncsig = thisfunc->GetFunctionSignature(program);
		StringHandle patternmatchername = program.GetDynamicPatternMatcherForFunction(*iter);
		emitter.EnterPatternResolver(patternmatchername);
		size_t numoverloads = program.GetNumFunctionOverloads(thisfunc->GetRawName());
		for(size_t i = 0; i < numoverloads; ++i)
		{
			StringHandle overloadname = program.GetFunctionOverloadName(thisfunc->GetRawName(), i);
			const IRSemantics::Function* thisoverload = functions.find(overloadname)->second;
			const FunctionSignature overloadsig = thisoverload->GetFunctionSignature(program);
			if(overloadsig.MatchesDynamicPattern(thisfuncsig))
				emitter.ResolvePattern(overloadname, overloadsig);
		}
		emitter.ExitPatternResolver();
		emitter.DefineLexicalScope(patternmatchername, 0, 0);
	}

	for(size_t i = 0; i < numglobalblocks; ++i)
		emitter.ExitEntity();

	return true;
}

