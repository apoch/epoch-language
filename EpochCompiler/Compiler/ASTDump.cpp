#include "pch.h"

#include "Compiler/ASTDump.h"

#include <iterator>


using namespace ASTTraverse;


namespace
{
	struct VariantDumpVisitor : public boost::static_visitor<>
	{
		VariantDumpVisitor(DumpToStream& dumper, std::wostream& stream)
			: TheDumper(dumper),
			  TheStream(stream){ }

		template <typename T>
		void Recurse(T& node)
		{
			VariantDumpVisitor subvisitor(TheDumper, TheStream);
			boost::apply_visitor(subvisitor, node);
		}

		template <typename T>
		void operator () (T& value)
		{
			TheStream << L"Literal \"" << value << L"\"";
		}

		template <typename T, typename PtrT>
		void operator () (AST::Deferred<T, PtrT>& deferred)
		{
			(*this)(*deferred.Content);
		}

		void operator () (AST::Undefined&)
		{
			TheStream << L"**UNDEFINED**";
		}

		void operator () (AST::Expression& expression)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, expression, TheDumper.Exit);
		}

		void operator () (AST::PreOperatorStatement& statement)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, statement, TheDumper.Exit);
		}

		void operator () (AST::PostOperatorStatement& statement)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, statement, TheDumper.Exit);
		}

		void operator () (AST::Statement& statement)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, statement, TheDumper.Exit);
		}

		void operator () (AST::Entity& entity)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, entity, TheDumper.Exit);
		}

		void operator () (AST::PostfixEntity& entity)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, entity, TheDumper.Exit);
		}

		void operator () (AST::CodeBlock& block)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, block, TheDumper.Exit);
		}

		void operator () (AST::Assignment& assignment)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, assignment, TheDumper.Exit);
		}

		void operator () (AST::NamedFunctionParameter& parameter)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, parameter, TheDumper.Exit);
		}

		void operator () (AST::FunctionReferenceSignature& signature)
		{
			TheDumper.TheTraverser.Do(TheDumper.Entry, signature, TheDumper.Exit);
		}

		void operator () (AST::LiteralToken& token)
		{
			Recurse(token);
		}

		void operator () (AST::Parenthetical& parenthetical)
		{
			Recurse(parenthetical);
		}

		void operator () (AST::CodeBlockEntry& codeblockentry)
		{
			Recurse(codeblockentry);
		}

		void operator () (AST::AnyEntity& anyentity)
		{
			Recurse(anyentity);
		}

		void operator () (AST::AnyStatement& anystatement)
		{
			Recurse(anystatement);
		}

	private:
		DumpToStream& TheDumper;
		std::wostream& TheStream;
	};
}


void DumpToStream::EntryHelper::operator () (AST::Program& program)
{
	self->Indent();
	self->TheStream << L"Program AST" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Program& program)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of program" << std::endl;
}



void DumpToStream::EntryHelper::operator () (AST::Function& function)
{
	self->Indent();
	self->TheStream << L"Function " << function.Name << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Function& function)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of function " << function.Name << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::FunctionParameter& param)
{
	self->Indent();
	self->TheStream << L"Parameter" << std::endl;
	++self->Indentation;

	VariantDumpVisitor visitor(*self, self->TheStream);
	boost::apply_visitor(visitor, param);
}

void DumpToStream::ExitHelper::operator () (AST::FunctionParameter& param)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of parameter" << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::NamedFunctionParameter& param)
{
	self->Indent();
	self->TheStream << param.Name << L" of type " << param.Type << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::Expression& expression)
{
	self->Indent();
	self->TheStream << L"Expression" << std::endl;
	++self->Indentation;
	self->Indent();
}

void DumpToStream::ExitHelper::operator () (AST::Expression& expression)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of expression" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	for(std::vector<AST::IdentifierT>::const_iterator iter = exprcomponent.UnaryPrefixes.Content->begin(); iter != exprcomponent.UnaryPrefixes.Content->end(); ++iter)
		self->TheStream << *iter << L" ";

	VariantDumpVisitor visitor(*self, self->TheStream);
	boost::apply_visitor(visitor, exprcomponent.Component);
}

void DumpToStream::ExitHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	self->TheStream << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->TheStream << exprfragment.Operator;
	(*this)(exprfragment.Component);
}


void DumpToStream::EntryHelper::operator () (AST::Statement& statement)
{
	self->Indent();
	self->TheStream << L"Statement " << statement.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Statement& statement)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of statement" << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::CodeBlockEntry& blockentry)
{
	self->Indent();

	VariantDumpVisitor visitor(*self, self->TheStream);
	boost::apply_visitor(visitor, blockentry);

	self->TheStream << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::CodeBlock& block)
{
	self->Indent();
	self->TheStream << L"Code block" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::CodeBlock& block)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of block" << std::endl;
}



void DumpToStream::EntryHelper::operator () (AST::Entity& entity)
{
	self->TheStream << L"Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Entity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of entity";
}


void DumpToStream::EntryHelper::operator () (AST::FunctionReferenceSignature& refsig)
{
	self->Indent();
	self->TheStream << L"Function signature named " << refsig.Identifier;
	self->TheStream << L" with parameter types:" << std::endl;
	++self->Indentation;

	for(std::vector<AST::IdentifierT>::const_iterator iter = refsig.ParamTypes.Content->begin(); iter != refsig.ParamTypes.Content->end(); ++iter)
	{
		self->Indent();
		self->TheStream << *iter << std::endl;
	}

	--self->Indentation;
	self->Indent();
	self->TheStream << L"Returning " << refsig.ReturnType << std::endl;
}


void DumpToStream::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	self->Indent();
	self->TheStream << L"Function return" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of return" << std::endl;
}

void DumpToStream::EntryHelper::UnknownNodeType(const char* name) const
{
	self->Indent();
	self->TheStream << L"*** Unknown node type " << name << std::endl;
}

void DumpToStream::ExitHelper::UnknownNodeType(const char* name) const
{
}



void DumpToStream::Indent()
{
	std::fill_n(std::ostream_iterator<wchar_t, wchar_t>(TheStream), Indentation, L' ');
}
