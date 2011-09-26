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
	class Function;
	class Expression;
	class ExpressionComponent;
	class ExpressionFragment;
	class Assignment;
	class Statement;
	class CodeBlock;
	class Entity;
}

class StringPoolManager;
struct CompilerInfoTable;


// Dependencies
#include "Compiler/Abstract Syntax Tree/ASTTraversal.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"

#include <stack>


namespace ASTTraverse
{

	//
	// Traverser for converting to the IR used for semantic validation
	//
	struct CompilePassSemantics
	{
		//
		// Construct the traverser
		//
		CompilePassSemantics(StringPoolManager& strings, CompilerInfoTable& infotable)
			: CurrentProgram(NULL),
			  InFunctionReturn(false),
			  Strings(strings),
			  InfoTable(infotable)
		{
			Entry.self = this;
			Exit.self = this;

			StateStack.push(STATE_UNKNOWN);
		}

		// Destruction
		~CompilePassSemantics();

		// Compile-time code execution
		bool CompileTimeCodeExecution();

		// Validation
		bool Validate() const;

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
			void operator () (T& node)
			{
				// TODO - better exceptions
				throw std::exception("Failure in parser");
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

			void operator () (AST::Assignment& assignment);

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
			void operator() (T& node)
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
			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);
			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			void operator () (AST::ExpressionFragment& exprfragment);

			void operator () (AST::Assignment& assignment);

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

		// Internal bindings to the owning CompilePassSemantics object
		private:
			CompilePassSemantics* self;
			friend struct CompilePassSemantics;
		} Exit;

	// Internal state
	private:
		IRSemantics::Program* CurrentProgram;

		std::list<IRSemantics::Structure*> CurrentStructures;
		std::list<IRSemantics::Function*> CurrentFunctions;
		std::list<IRSemantics::Expression*> CurrentExpressions;
		std::list<IRSemantics::ExpressionComponent*> CurrentExpressionComponents;
		std::list<IRSemantics::ExpressionFragment*> CurrentExpressionFragments;
		std::list<IRSemantics::Assignment*> CurrentAssignments;
		std::list<IRSemantics::Statement*> CurrentStatements;
		std::list<IRSemantics::CodeBlock*> CurrentCodeBlocks;
		std::list<IRSemantics::Entity*> CurrentEntities;
		std::list<IRSemantics::Entity*> CurrentChainedEntities;
		std::list<IRSemantics::Entity*> CurrentPostfixEntities;

		enum States
		{
			STATE_UNKNOWN,
			STATE_PROGRAM,
			STATE_FUNCTION,
			STATE_FUNCTION_PARAM,
			STATE_FUNCTION_RETURN,
			STATE_EXPRESSION,
			STATE_EXPRESSION_COMPONENT,
			STATE_EXPRESSION_FRAGMENT,
			STATE_STATEMENT,
			STATE_PREOP_STATEMENT,
			STATE_POSTOP_STATEMENT,
			STATE_ASSIGNMENT,
			STATE_CODE_BLOCK,
			STATE_ENTITY,
			STATE_POSTFIX_ENTITY,
			STATE_CHAINED_ENTITY,
		};

		std::stack<States> StateStack;

		bool InFunctionReturn;

		StringPoolManager& Strings;
		CompilerInfoTable& InfoTable;
	};

}


namespace CompilerPasses
{

	IRSemantics::Program* ValidateSemantics(AST::Program& program, StringPoolManager& strings, CompilerInfoTable& infotable);

}

