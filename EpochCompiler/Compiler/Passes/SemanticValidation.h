//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration for the compilation pass which
// performs semantic validation and type inference on the
// input program code. This traverser converts the AST into
// an intermediate representation (IR) which is then used
// by the compiler to perform semantic checks etc.
//

#pragma once


// Forward declarations
namespace IRSemantics
{
	class Program;
	class Structure;
	class StructureMemberFunctionReference;
	class Function;
	class Expression;
	class ExpressionAtom;
	class Assignment;
	class Statement;
	class CodeBlock;
	class Entity;
	class FunctionParamFuncRef;
	struct FunctionTag;
}

class StringPoolManager;
class CompileSession;


// Dependencies
#include "Compiler/Abstract Syntax Tree/ASTTraversal.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"

#include <stack>


namespace ASTTraverse
{

	//
	// Traverser for converting to the IR used for semantic validation
	//
	struct CompilePassSemantics : public CompileErrorContextualizer
	{
		//
		// Construct the traverser
		//
		CompilePassSemantics(StringPoolManager& strings, CompileSession& session, const std::wstring::const_iterator& sourcebegin, const std::wstring::const_iterator& sourceend)
			: CurrentProgram(NULL),
			  InFunctionReturn(false),
			  Strings(strings),
			  Session(session),
			  SourceBegin(sourcebegin),
			  SourceEnd(sourceend),
			  ErrorContext(NULL)
		{
			Entry.self = this;
			Exit.self = this;

			StateStack.push(STATE_UNKNOWN);

			Errors.GetContextFrom(this);
		}

		// Destruction
		~CompilePassSemantics();

	// Non-copyable
	private:
		CompilePassSemantics(const CompilePassSemantics& rhs);
		CompilePassSemantics& operator = (const CompilePassSemantics& rhs);

	public:
		// Compile-time code execution
		bool CompileTimeCodeExecution();

		// Type inference
		bool TypeInference();

		// Validation
		bool Validate();

		// Attached program retrieval
		IRSemantics::Program* DetachProgram();

		//
		// Node entry functor
		//
		struct EntryHelper
		{
			//
			// Catch-all overload
			//
			template <typename T>
			void operator () (T&)
			{
				//
				// This is a mismatch between the parser and the
				// IR generation/AST traversal logic. An AST node
				// class exists and has been traversed but it is
				// not recognized correctly by any of the overloads
				// below. Ensure that the AST feature in question
				// is fully implemented.
				//
				throw InternalException("Unrecognized AST node type");
			}

			//
			// Catch-all for unwrapping deferred AST nodes
			//
			template <typename T, typename PtrT>
			void operator () (AST::Deferred<T, PtrT>& deferred)
			{
				(*this)(*deferred.Content);
			}

			//
			// Overloads for handling all the node types we recognize
			//

			void operator () (AST::Undefined& undefined);

			void operator () (AST::IdentifierT& identifier);
			void operator () (AST::Structure& structure);
			void operator () (AST::StructureMemberVariable& variable);
			void operator () (AST::StructureMemberFunctionRef& funcref);

			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);
			void operator () (AST::NamedFunctionParameter& param);
			void operator () (AST::FunctionReferenceSignature& refsig);
			void operator () (AST::FunctionTag& tag);

			void operator () (AST::Assignment& assignment);
			void operator () (AST::Initialization& initialization);

			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			void operator () (AST::ExpressionFragment& exprfragment);

			void operator () (AST::Statement& statement);
			void operator () (AST::PreOperatorStatement& statement);
			void operator () (AST::PostOperatorStatement& statement);
			void operator () (AST::CodeBlock& block);

			void operator () (AST::Entity& entity);
			void operator () (AST::PostfixEntity& entity);
			void operator () (AST::ChainedEntity& entity);

			//
			// Overloads for hint markers
			//

			void operator () (Markers::FunctionReturnExpression& marker);
			void operator () (Markers::ExpressionComponentPrefixes& marker);
			void operator () (Markers::FunctionSignatureParams& marker);
			void operator () (Markers::FunctionSignatureReturn& marker);
			void operator () (Markers::StructureFunctionParams& marker);
			void operator () (Markers::StructureFunctionReturn& marker);

			void operator () (AST::RefTag& tag);

		// Internal binding to the owning CompilePassSemantics object
		private:
			CompilePassSemantics* self;
			friend struct CompilePassSemantics;
		} Entry;

		//
		// Node exit functor
		//
		struct ExitHelper
		{
			//
			// Generic fallback; ignore anything we don't recognize
			//
			template <typename T>
			void operator() (T&)
			{
				// Ignored!
			}

			//
			// Helper for unwrapping deferred AST nodes
			//
			template <typename T, typename PtrT>
			void operator () (AST::Deferred<T, PtrT>& deferred)
			{
				(*this)(*deferred.Content);
			}

			//
			// Overloads for AST nodes we care about
			//

			void operator () (AST::Structure& structure);
			void operator () (AST::StructureMemberFunctionRef& funcref);

			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);
			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			void operator () (AST::ExpressionFragment& exprfragment);

			void operator () (AST::FunctionReferenceSignature& refsig);

			void operator () (AST::NamedFunctionParameter& param);
			void operator () (AST::FunctionTag& tag);

			void operator () (AST::Assignment& assignment);
			void operator () (AST::Initialization& initialization);

			void operator () (AST::Statement& statement);
			void operator () (AST::PreOperatorStatement& statement);
			void operator () (AST::PostOperatorStatement& statement);
			void operator () (AST::CodeBlock& block);

			void operator () (AST::Entity& entity);
			void operator () (AST::PostfixEntity& entity);
			void operator () (AST::ChainedEntity& entity);

			//
			// Overloads for hint markers
			//

			void operator () (Markers::FunctionReturnExpression& marker);
			void operator () (Markers::ExpressionComponentPrefixes& marker);
			void operator () (Markers::FunctionSignatureParams& marker);
			void operator () (Markers::FunctionSignatureReturn& marker);
			void operator () (Markers::StructureFunctionParams& marker);
			void operator () (Markers::StructureFunctionReturn& marker);

		// Internal bindings to the owning CompilePassSemantics object
		private:
			CompilePassSemantics* self;
			friend struct CompilePassSemantics;
		} Exit;

	// Additional helper routes
	public:
		size_t FindLine(const AST::IdentifierT& identifier) const;
		size_t FindColumn(const AST::IdentifierT& identifier) const;
		std::wstring FindSource(const AST::IdentifierT& identifier) const;

	// Internal state
	private:
		const std::wstring::const_iterator& SourceBegin;
		const std::wstring::const_iterator& SourceEnd;

		IRSemantics::Program* CurrentProgram;

		std::vector<IRSemantics::Structure*> CurrentStructures;
		std::vector<IRSemantics::Function*> CurrentFunctions;
		std::vector<IRSemantics::Expression*> CurrentExpressions;
		std::vector<IRSemantics::Assignment*> CurrentAssignments;
		std::vector<IRSemantics::Statement*> CurrentStatements;
		std::vector<IRSemantics::CodeBlock*> CurrentCodeBlocks;
		std::vector<IRSemantics::Entity*> CurrentEntities;
		std::vector<IRSemantics::Entity*> CurrentChainedEntities;
		std::vector<IRSemantics::Entity*> CurrentPostfixEntities;
		std::vector<IRSemantics::FunctionParamFuncRef*> CurrentFunctionSignatures;
		std::vector<IRSemantics::StructureMemberFunctionReference*> CurrentStructureFunctions;
		std::vector<IRSemantics::FunctionTag*> CurrentFunctionTags;

		enum States
		{
			STATE_UNKNOWN,
			STATE_PROGRAM,
			STATE_FUNCTION,
			STATE_FUNCTION_PARAM,
			STATE_FUNCTION_RETURN,
			STATE_FUNCTION_SIGNATURE,
			STATE_FUNCTION_SIGNATURE_PARAMS,
			STATE_FUNCTION_SIGNATURE_RETURN,
			STATE_FUNCTION_TAG,
			STATE_FUNCTION_TAG_PARAM,
			STATE_EXPRESSION,
			STATE_EXPRESSION_COMPONENT,
			STATE_EXPRESSION_COMPONENT_PREFIXES,
			STATE_EXPRESSION_FRAGMENT,
			STATE_STATEMENT,
			STATE_PREOP_STATEMENT,
			STATE_POSTOP_STATEMENT,
			STATE_ASSIGNMENT,
			STATE_CODE_BLOCK,
			STATE_ENTITY,
			STATE_POSTFIX_ENTITY,
			STATE_CHAINED_ENTITY,
			STATE_STRUCTURE_FUNCTION,
			STATE_STRUCTURE_FUNCTION_PARAMS,
			STATE_STRUCTURE_FUNCTION_RETURN,
		};

		std::stack<States> StateStack;

		bool InFunctionReturn;
		bool IsParamRef;

		StringPoolManager& Strings;
		CompileSession& Session;

		AST::IdentifierT* ErrorContext;

	public:
		CompileErrors Errors;
		virtual void UpdateContext(CompileErrors& errors) const;
		virtual void UpdateFromContext(CompileErrors& errors, const AST::IdentifierT& context) const;
	};

}


namespace CompilerPasses
{

	IRSemantics::Program* ValidateSemantics(AST::Program& program, const std::wstring::const_iterator& sourcebegin, const std::wstring::const_iterator& sourceend, StringPoolManager& strings, CompileSession& session);

}

