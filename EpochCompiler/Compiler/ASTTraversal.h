#pragma once

#include "Compiler/AbstractSyntaxTree.h"

namespace ASTTraverse
{

	namespace Markers
	{
		struct FunctionReturnExpression { };
	}

	struct Traverser
	{
		Traverser(AST::Program& program)
			: TheProgram(program)
		{
		}

		template <typename EntryActionT, typename ExitActionT>
		void PerformActions(EntryActionT& entryaction, ExitActionT& exitaction)
		{
			Do(entryaction, TheProgram, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT, typename NodeT>
		void Do(EntryActionT& entryaction, NodeT& node, ExitActionT& exitaction)
		{
			entryaction(node);
			exitaction(node);
		}

		template <typename EntryActionT, typename ExitActionT, typename T>
		void Do(EntryActionT& entryaction, std::list<T>& nodes, ExitActionT& exitaction)
		{
			for(std::list<T>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
				Do(entryaction, *iter, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Statement& statement, ExitActionT& exitaction)
		{
			entryaction(statement);
			Do(entryaction, statement.Params, exitaction);
			exitaction(statement);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Expression& expression, ExitActionT& exitaction)
		{
			entryaction(expression);
			Do(entryaction, expression.First, exitaction);
			Do(entryaction, expression.Remaining, exitaction);
			exitaction(expression);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::CodeBlockEntry& codeblockentry, ExitActionT& exitaction)
		{
			VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
			boost::apply_visitor(visitor, codeblockentry);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::CodeBlock& codeblock, ExitActionT& exitaction)
		{
			entryaction(codeblock);
			Do(entryaction, codeblock.Entries, exitaction);
			exitaction(codeblock);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Entity& entity, ExitActionT& exitaction)
		{
			entryaction(entity);
			Do(entryaction, entity.Parameters, exitaction);
			Do(entryaction, entity.Code, exitaction);
			Do(entryaction, entity.Chain, exitaction);
			exitaction(entity);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Function& function, ExitActionT& exitaction)
		{
			entryaction(function);
			Do(entryaction, function.Parameters, exitaction);
			entryaction(Markers::FunctionReturnExpression());
			Do(entryaction, function.Return, exitaction);
			exitaction(Markers::FunctionReturnExpression());
			Do(entryaction, function.Code, exitaction);
			exitaction(function);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::AnyStatement& statement, ExitActionT& exitaction)
		{
			VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
			boost::apply_visitor(visitor, statement);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::MetaEntity& metaentity, ExitActionT& exitaction)
		{
			VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
			boost::apply_visitor(visitor, metaentity);
		}

	private:
		template <typename EntryActionT, typename ExitActionT>
		struct VariantVisitor : public boost::static_visitor<>
		{
			VariantVisitor(Traverser& traverser, EntryActionT& entry, ExitActionT& exit)
				: self(traverser),
				  Entry(entry),
				  Exit(exit)
			{ }

			template <typename T>
			void operator () (T& value)
			{
				self.Do(Entry, value, Exit);
			}

		private:
			Traverser& self;
			EntryActionT& Entry;
			ExitActionT& Exit;
		};

	private:
		AST::Program& TheProgram;
	};

}

