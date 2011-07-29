#pragma once

#include "Compiler/Abstract Syntax Tree/Program.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"
#include "Compiler/Abstract Syntax Tree/AnyStatement.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"


namespace ASTTraverse
{

	namespace
	{
		template <typename T>
		struct IsVariantType
		{
			typedef char (&yes)[1];
			typedef char (&no)[2];

			template <typename U>
			static yes TestDummy(typename U::types*);

			template <typename U>
			static no TestDummy(...);

			enum PerformCheck
			{
				CheckResult = (sizeof(TestDummy<T>(NULL)) == sizeof(yes))
			};
		};

		struct EnableDummy
		{ };

		template <bool>
		struct EnableIfVariantHelper;

		template <>
		struct EnableIfVariantHelper<true>
		{
			typedef EnableDummy type;
		};

		template <>
		struct EnableIfVariantHelper<false>
		{ };

		template <typename T>
		struct EnableIfVariant : public EnableIfVariantHelper<IsVariantType<T>::CheckResult>
		{ };
	}

	namespace Markers
	{
		struct FunctionReturnExpression { };
	}

	struct Traverser
	{
		Traverser()
			: TheProgram(NULL)
		{ }

		explicit Traverser(AST::Program& program)
			: TheProgram(&program)
		{ }

		template <typename EntryActionT, typename ExitActionT>
		void PerformActions(EntryActionT& entryaction, ExitActionT& exitaction)
		{
			if(TheProgram)
				Do(entryaction, *TheProgram, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT, typename NodeT>
		void Do(EntryActionT& entryaction, NodeT& node, ExitActionT& exitaction, ...)
		{
			entryaction(node);
			exitaction(node);
		}

		template <typename EntryActionT, typename ExitActionT, typename NodeT>
		void Do(EntryActionT& entryaction, NodeT& node, ExitActionT& exitaction, typename EnableIfVariant<NodeT>::type& = EnableDummy())
		{
			VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
			boost::apply_visitor(visitor, node);			
		}

		template <typename EntryActionT, typename ExitActionT, typename T, typename PtrT>
		void Do(EntryActionT& entryaction, AST::Deferred<T, PtrT>& deferred, ExitActionT& exitaction)
		{
			Do(entryaction, *deferred.Content, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT, typename T, typename PtrT>
		void Do(EntryActionT& entryaction, AST::DeferredContainer<T, PtrT>& deferred, ExitActionT& exitaction)
		{
			Do(entryaction, deferred.Content->Container, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT, typename T, typename AllocT>
		void Do(EntryActionT& entryaction, std::vector<T, AllocT>& nodes, ExitActionT& exitaction)
		{
			for(std::vector<T, AllocT>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
				Do(entryaction, *iter, exitaction);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Structure& structure, ExitActionT& exitaction)
		{
			entryaction(structure);
			for(std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> >::iterator iter = structure.Members.begin(); iter != structure.Members.end(); ++iter)
			{
				VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
				boost::apply_visitor(visitor, *iter);
			}
			exitaction(structure);
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
		void Do(EntryActionT& entryaction, AST::ExpressionComponent& component, ExitActionT& exitaction)
		{
			entryaction(component);
			Do(entryaction, component.UnaryPrefixes, exitaction);
			Do(entryaction, component.Component.Content->V, exitaction);
			exitaction(component);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::ExpressionFragment& fragment, ExitActionT& exitaction)
		{
			entryaction(fragment);
			Do(entryaction, fragment.Operator, exitaction);
			Do(entryaction, fragment.Component.Content->Component.Content->V, exitaction);
			exitaction(fragment);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Assignment& assignment, ExitActionT& exitaction)
		{
			entryaction(assignment);
			Do(entryaction, assignment.LHS, exitaction);
			Do(entryaction, assignment.RHS, exitaction);
			exitaction(assignment);
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
		void Do(EntryActionT& entryaction, AST::PostfixEntity& entity, ExitActionT& exitaction)
		{
			entryaction(entity);
			Do(entryaction, entity.Parameters, exitaction);
			Do(entryaction, entity.Code, exitaction);
			Do(entryaction, entity.PostfixIdentifier, exitaction);
			Do(entryaction, entity.PostfixParameters, exitaction);
			exitaction(entity);
		}

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::ChainedEntity& entity, ExitActionT& exitaction)
		{
			entryaction(entity);
			Do(entryaction, entity.Identifier, exitaction);
			Do(entryaction, entity.Parameters, exitaction);
			Do(entryaction, entity.Code, exitaction);
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
		void Do(EntryActionT& entryaction, AST::FunctionParameter& parameter, ExitActionT& exitaction)
		{
			VariantVisitor<EntryActionT, ExitActionT> visitor(*this, entryaction, exitaction);
			boost::apply_visitor(visitor, parameter.V);
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

		template <typename EntryActionT, typename ExitActionT>
		void Do(EntryActionT& entryaction, AST::Program& program, ExitActionT& exitaction)
		{
			entryaction(program);
			Do(entryaction, program.MetaEntities, exitaction);
			exitaction(program);
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
		AST::Program* TheProgram;
	};

}

