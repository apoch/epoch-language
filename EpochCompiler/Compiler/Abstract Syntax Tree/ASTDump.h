//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration that simply dumps the AST contents
// into human-readable textual format. Supports output to anything
// that is a std::wostream.
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/ASTTraversal.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"

#include <ostream>


namespace ASTTraverse
{

	//
	// Callback wrappers expose two sets of functors, one for entering
	// AST branches, and one for exiting them. These are passed to the
	// traverser itself by the calling code to accomplish traversal of
	// the AST. In this way the traversal logic remains fully generic,
	// and the application-specific callback logic can remain agnostic
	// about the actual structure of the AST.
	//
	struct DumpToStream
	{
		//
		// Construct a dump wrapper and bind it to the stream and traverser
		//
		DumpToStream(std::wostream& stream)
			: TheStream(stream),
			  Indentation(0)
		{
			Entry.self = this;
			Exit.self = this;
		}

		//
		// Node entry functor
		//
		// This functor is called (via overloads) every time an AST node is
		// entered by the tree traversal. It is primarily used for changing
		// the state of the callback wrapper according to the nature of the
		// current AST branch.
		//
		struct EntryHelper
		{
			//
			// Catch-all overload for handling literals that can be
			// output directly to the stream, generally because they
			// are leaf nodes and have no children that need to be
			// handled or formatted in the output.
			//
			// A compile error here indicates that something is trying
			// to pass an AST node type to the functor, but there is
			// no suitable overload matching that AST node type. Add a
			// function overload for that node type.
			//
			template <typename T>
			void operator () (T& node)
			{
				self->Indent();
				self->TheStream << node << std::endl;
			}

			//
			// Catch-all for unwrapping deferred AST nodes. See the
			// definition file for the Deferred wrapper for details.
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

		// Internal binding to the owning DumpToStream object
		private:
			DumpToStream* self;
			friend struct DumpToStream;
		} Entry;

		//
		// Node exit functor
		//
		// Used for resetting state in the callback wrapper when a branch
		// of the AST is done being traversed.
		//
		struct ExitHelper
		{
			//
			// Generic fallback; any node we don't recognize we
			// will just ignore. This means it is very important
			// to overload for every AST node type that affects
			// the state of the callback wrapper via EntryHelper.
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

		// Internal bindings to the owning DumpToStream object
		private:
			DumpToStream* self;
			friend struct DumpToStream;
		} Exit;

	// Internal helpers
	private:
		void Indent();

	// Internal state
	private:
		std::wostream& TheStream;
		unsigned Indentation;
	};

}

