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
			/*
			template <typename T>
			void operator () (T& node)
			{
				UnknownNodeType(typeid(T).name());
			}*/

			template <typename T, typename PtrT>
			void operator () (AST::Deferred<T, PtrT>& deferred)
			{
				(*this)(*deferred.Content);
			}

			template <typename T>
			void operator () (std::vector<T>& container)
			{
				for(std::vector<T>::iterator iter = container.begin(); iter != container.end(); ++iter)
					(*this)(*iter);
			}

			void operator () (AST::Undefined& undefined);

			void operator () (AST::IdentifierT& identifier);
			void operator () (AST::IdentifierListRaw& identifiers)
			{
				(*this)(identifiers.Container);
			}

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
			void operator () (AST::CodeBlockEntry& blockentry);
			void operator () (AST::CodeBlock& block);

			void operator () (AST::Entity& entity);
			void operator () (AST::PostfixEntity& entity);
			void operator () (AST::ChainedEntity& entity);

			void operator () (Markers::FunctionReturnExpression& marker);

			void UnknownNodeType(const char* name) const;

			DumpToStream* self;
		} Entry;

		struct ExitHelper
		{
			/*template <typename T>
			void operator() (T& node)
			{
				UnknownNodeType(typeid(T).name());
			}
			*/

			template <typename T, typename PtrT>
			void operator () (AST::Deferred<T, PtrT>& deferred)
			{
				(*this)(*deferred.Content);
			}

			void operator () (AST::Undefined& undefined) { }

			void operator () (AST::IdentifierT& identifier);
			void operator () (AST::IdentifierListRaw& identifiers) { }

			void operator () (AST::Structure& structure);
			void operator () (AST::StructureMemberVariable& variable) { }
			void operator () (AST::StructureMemberFunctionRef& funcref) { }

			void operator () (AST::Program& program);
			void operator () (AST::Function& function);
			void operator () (AST::FunctionParameter& param);
			void operator () (AST::NamedFunctionParameter& param) { }
			void operator () (AST::FunctionReferenceSignature& param) { }

			void operator () (AST::Expression& expression);
			void operator () (AST::ExpressionComponent& exprcomponent);
			void operator () (AST::ExpressionFragment& exprfragment) { }

			void operator () (AST::Assignment& assignment);
			
			void operator () (AST::Statement& statement);
			void operator () (AST::PreOperatorStatement& statement);
			void operator () (AST::PostOperatorStatement& statement);
			void operator () (AST::CodeBlock& block);
			void operator () (AST::CodeBlockEntry& entry) { }

			void operator () (AST::Entity& entity);
			void operator () (AST::PostfixEntity& entity);
			void operator () (AST::ChainedEntity& entity);

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

