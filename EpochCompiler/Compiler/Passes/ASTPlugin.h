//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration for the compilation pass which
// feeds the AST's contents into a compiler plugin. This is
// pretty much entirely for getting the self-hosted compiler
// bootstrapped.
//

#pragma once


// Forward declarations
namespace IRSemantics
{
	class Program;
	class Namespace;
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
#include "Compiler/Abstract Syntax Tree/TypeDefinitions.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"

#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/CompileTimeParams.h"

#include <stack>


namespace ASTTraverse
{

	//
	// Traverser for converting to plugin-compatible IR
	//
	struct ASTPlugin
	{
		//
		// Construct the traverser
		//
		ASTPlugin()
		{
			Entry.self = this;
			Exit.self = this;
		}

	// Non-copyable
	private:
		ASTPlugin(const ASTPlugin& rhs);
		ASTPlugin& operator = (const ASTPlugin& rhs);

	public:
		// Compile-time code execution
		bool CompileTimeCodeExecution();

		// Type inference
		bool TypeInference();

		// Validation
		bool Validate();

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
			void operator () (AST::Nothing& nothing);

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

			void operator () (AST::TypeAlias& alias);
			void operator () (AST::StrongTypeAlias& alias);
			void operator () (AST::SumType& sumtype);

			void operator () (AST::TemplateParameter& param);
			void operator () (AST::TemplateArgument& arg);

			//
			// Overloads for hint markers
			//

			void operator () (Markers::FunctionReturnExpression& marker);
			void operator () (Markers::ExpressionComponentPrefixes& marker);
			void operator () (Markers::FunctionSignatureParams& marker);
			void operator () (Markers::FunctionSignatureReturn& marker);
			void operator () (Markers::StructureFunctionParams& marker);
			void operator () (Markers::StructureFunctionReturn& marker);
			void operator () (Markers::TemplateArgs& marker);

			void operator () (AST::RefTag& tag);

		// Internal binding to the owning ASTPlugin object
		private:
			ASTPlugin* self;
			friend struct ASTPlugin;
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

			void operator () (AST::SumType& sumtype);

			//
			// Overloads for hint markers
			//

			void operator () (Markers::FunctionReturnExpression& marker);
			void operator () (Markers::ExpressionComponentPrefixes& marker);
			void operator () (Markers::FunctionSignatureParams& marker);
			void operator () (Markers::FunctionSignatureReturn& marker);
			void operator () (Markers::StructureFunctionParams& marker);
			void operator () (Markers::StructureFunctionReturn& marker);
			void operator () (Markers::TemplateArgs& marker);

		// Internal bindings to the owning ASTPlugin object
		private:
			ASTPlugin* self;
			friend struct ASTPlugin;
		} Exit;
	};

}


namespace CompilerPasses
{

	void HandASTToPlugin(AST::Program& program);

}

