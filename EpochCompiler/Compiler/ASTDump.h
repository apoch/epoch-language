#pragma once


#include "Compiler/ASTTraversal.h"

#include <ostream>


namespace ASTTraverse
{

	struct DumpToStream
	{
		DumpToStream(std::wostream& stream, Traverser& traverse)
			: TheStream(stream),
			  TheTraverser(traverse),
			  Indentation(0)
		{
			Entry.self = this;
			Exit.self = this;
		}

		struct EntryHelper
		{
			template <typename T>
			void operator() (T& node)
			{
				UnknownNodeType(typeid(T).name());
			}

			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);
			void operator () (AST::NamedFunctionParameter& param);
			void operator () (AST::FunctionReferenceSignature& refsig);

			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			void operator () (AST::ExpressionFragment& exprfragment);

			void operator () (AST::Statement& statement);
			void operator () (AST::CodeBlockEntry& blockentry);
			void operator () (AST::CodeBlock& block);

			void operator () (AST::Entity& entity);

			void operator () (AST::AnyStatement&) { }

			void operator () (Markers::FunctionReturnExpression& marker);

			void UnknownNodeType(const char* name) const;

			DumpToStream* self;
		} Entry;

		struct ExitHelper
		{
			template <typename T>
			void operator() (T& node)
			{
				UnknownNodeType(typeid(T).name());
			}

			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);

			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			
			void operator () (AST::Statement& statement);
			void operator () (AST::CodeBlock& block);

			void operator () (AST::Entity& entity);

			void operator () (AST::AnyStatement&) { }

			void operator () (Markers::FunctionReturnExpression& marker);

			void UnknownNodeType(const char* name) const;

			DumpToStream* self;
		} Exit;

		Traverser& TheTraverser;

	private:
		void Indent();

	private:
		std::wostream& TheStream;
		unsigned Indentation;
	};

}

