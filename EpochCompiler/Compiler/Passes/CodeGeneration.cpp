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

#include "Utility/StringPool.h"


namespace
{
	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program, ByteCodeEmitter& emitter);
	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program);


	void BindReference(ByteCodeEmitter& emitter, const std::vector<StringHandle>& identifiers)
	{
		if(identifiers.empty())
			throw std::exception("Cannot bind reference; no lvalue");		// TODO - better exceptions

		emitter.BindReference(identifiers[0]);
		
		for(size_t i = 1; i < identifiers.size(); ++i)
			emitter.BindStructureReference(identifiers[i]);
	}

	void PushValue(ByteCodeEmitter& emitter, const std::vector<StringHandle>& identifiers, const IRSemantics::Program& program, const IRSemantics::CodeBlock& activescope)
	{
		if(identifiers.empty())
			throw std::exception("Cannot push value; no rvalue");			// TODO - better exceptions

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

	void EmitExpressionAtom(ByteCodeEmitter& emitter, const IRSemantics::ExpressionAtom* rawatom, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program)
	{
		if(!rawatom)
			throw std::exception("Bogus lack of atom");			// TODO - better exceptions
		else if(const IRSemantics::ExpressionAtomParenthetical* atom = dynamic_cast<const IRSemantics::ExpressionAtomParenthetical*>(rawatom))
		{
			// TODO - emit parenthetical
		}
		else if(const IRSemantics::ExpressionAtomIdentifier* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifier*>(rawatom))
		{
			if(program.HasFunction(atom->GetIdentifier()))
				emitter.PushStringLiteral(atom->GetIdentifier());
			else
				emitter.PushVariableValue(atom->GetIdentifier(), activescope.GetVariableTypeByID(atom->GetIdentifier()));
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
		else
			throw std::exception("Bogus expression atom");		// TODO - better exceptions
	}

	void EmitExpression(ByteCodeEmitter& emitter, const IRSemantics::Expression& expression, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program)
	{
		// TODO - have semantic layer reorder the expression for operator precedence prior to code generation

		const std::vector<IRSemantics::ExpressionAtom*>& rawatoms = expression.GetAtoms();
		for(std::vector<IRSemantics::ExpressionAtom*>::const_iterator iter = rawatoms.begin(); iter != rawatoms.end(); ++iter)
			EmitExpressionAtom(emitter, *iter, activescope, program);
	}

	void EmitStatement(ByteCodeEmitter& emitter, const IRSemantics::Statement& statement, const IRSemantics::CodeBlock& activescope, const IRSemantics::Program& program)
	{
		const std::vector<IRSemantics::Expression*>& params = statement.GetParameters();
		for(std::vector<IRSemantics::Expression*>::const_iterator paramiter = params.begin(); paramiter != params.end(); ++paramiter)
			EmitExpression(emitter, **paramiter, activescope, program);

		// TODO - handle overloads
		emitter.Invoke(statement.GetName());
	}

	void GenerateAssignment(ByteCodeEmitter& emitter, const IRSemantics::Assignment& assignment, const IRSemantics::Program& program, const IRSemantics::CodeBlock& activescope)
	{
		bool pushlhs = false;

		const IRSemantics::AssignmentChain* rhs = assignment.GetRHS();
		if(!rhs)
			throw std::exception("Bogus data in assignment");	// TODO - better exceptions
		else if(const IRSemantics::AssignmentChainExpression* rhsexpression = dynamic_cast<const IRSemantics::AssignmentChainExpression*>(rhs))
		{
			EmitExpression(emitter, rhsexpression->GetExpression(), activescope, program);
		}
		else if(const IRSemantics::AssignmentChainAssignment* rhsassignment = dynamic_cast<const IRSemantics::AssignmentChainAssignment*>(rhs))
		{
			GenerateAssignment(emitter, rhsassignment->GetAssignment(), program, activescope);
			pushlhs = true;
		}

		if(program.GetString(assignment.GetOperatorName()) != L"=")
		{
			PushValue(emitter, assignment.GetLHS(), program, activescope);
			// TODO - handle overloads
			emitter.Invoke(assignment.GetOperatorName());
		}

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
		const std::vector<IRSemantics::Expression*>& params = entity.GetParameters();
		for(std::vector<IRSemantics::Expression*>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
			EmitExpression(emitter, **iter, activescope, program);

		// TODO - handle postfix entities

		emitter.EnterEntity(program.GetEntityTag(entity.GetName()), program.FindLexicalScopeName(&entity.GetCode()));
		Generate(entity.GetCode(), program, emitter);
		emitter.ExitEntity();
	}

	void Generate(const IRSemantics::CodeBlock& codeblock, const IRSemantics::Program& program, ByteCodeEmitter& emitter)
	{
		const std::vector<IRSemantics::CodeBlockEntry*>& entries = codeblock.GetEntries();
		for(std::vector<IRSemantics::CodeBlockEntry*>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
		{
			const IRSemantics::CodeBlockEntry* baseentry = *iter;
			
			if(!baseentry)
				throw std::exception("Bogus data in parse tree");		// TODO - better exceptions
			else if(const IRSemantics::CodeBlockAssignmentEntry* entry = dynamic_cast<const IRSemantics::CodeBlockAssignmentEntry*>(baseentry))
			{
				GenerateAssignment(emitter, entry->GetAssignment(), program, codeblock);
			}
			else if(const IRSemantics::CodeBlockStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockStatementEntry*>(baseentry))
			{
				EmitStatement(emitter, entry->GetStatement(), codeblock, program);
			}
			else if(const IRSemantics::CodeBlockPreOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPreOpStatementEntry*>(baseentry))
			{
				PushValue(emitter, entry->GetStatement().GetOperand(), program, codeblock);
				// TODO - handle overloads
				emitter.Invoke(entry->GetStatement().GetOperatorName());
				BindReference(emitter, entry->GetStatement().GetOperand());
				emitter.AssignVariable();
				PushValue(emitter, entry->GetStatement().GetOperand(), program, codeblock);
			}
			else if(const IRSemantics::CodeBlockPostOpStatementEntry* entry = dynamic_cast<const IRSemantics::CodeBlockPostOpStatementEntry*>(baseentry))
			{
				// Yes, we need to push this twice! (Once, the value is passed on to the operator
				// itself for invocation; the second push [or rather the one which happens first,
				// and appears lower on the stack] is used to hold the initial value of the expression
				// so that the subsequent code can read off the value safely, in keeping with the
				// traditional semantics of a post operator.)
				PushValue(emitter, entry->GetStatement().GetOperand(), program, codeblock);
				PushValue(emitter, entry->GetStatement().GetOperand(), program, codeblock);

				// TODO - handle overloads
				emitter.Invoke(entry->GetStatement().GetOperatorName());
				BindReference(emitter, entry->GetStatement().GetOperand());
				emitter.AssignVariable();
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
				throw std::exception("Unrecognized entry type");		// TODO - better exceptions
			}
		}
	}
}


bool CompilerPasses::GenerateCode(const IRSemantics::Program& program, ByteCodeEmitter& emitter)
{
	const StringPoolManager& strings = program.GetStringPool();
	const std::map<StringHandle, std::wstring>& stringpool = strings.GetInternalPool();

	for(std::map<StringHandle, std::wstring>::const_iterator iter = stringpool.begin(); iter != stringpool.end(); ++iter)
		emitter.PoolString(iter->first, iter->second);

	const std::map<StringHandle, IRSemantics::Structure*>& structures = program.GetStructures();
	for(std::map<StringHandle, IRSemantics::Structure*>::const_iterator iter = structures.begin(); iter != structures.end(); ++iter)
	{
		const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = iter->second->GetMembers();
		emitter.DefineStructure(iter->first, members.size());
		for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
			emitter.StructureMember(memberiter->first, memberiter->second->GetEpochType(program));
	}


	// TODO - emit lexical scope metadata
	// TODO - emit function tag metadata


	size_t numglobalblocks = program.GetNumGlobalCodeBlocks();
	for(size_t i = 0; i < numglobalblocks; ++i)
	{
		emitter.EnterEntity(Bytecode::EntityTags::Globals, program.GetGlobalCodeBlockName(i));
		Generate(program.GetGlobalCodeBlock(i), program, emitter);
	}

	emitter.Invoke(strings.Find(L"entrypoint"));
	emitter.Halt();
	
	const std::map<StringHandle, IRSemantics::Function*>& functions = program.GetFunctions();
	for(std::map<StringHandle, IRSemantics::Function*>::const_iterator iter = functions.begin(); iter != functions.end(); ++iter)
	{
		emitter.EnterFunction(iter->first);

		const IRSemantics::CodeBlock* code = iter->second->GetCode();
		if(code)
			Generate(*code, program, emitter);

		emitter.ExitFunction();
	}

	for(size_t i = 0; i < numglobalblocks; ++i)
		emitter.ExitEntity();

	return true;
}

