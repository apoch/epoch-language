//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Node classes for constructing abstract syntax trees for Epoch programs
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"

#include <boost/shared_ptr.hpp>


namespace AST
{

	template <typename T>
	struct Deferred
	{
		Deferred()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		Deferred(const T& expr)
			: Contents(new T(expr))
		{
			Content.Owner = this;
		}

		Deferred(const Deferred& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		template <typename VariantContentT>
		Deferred(const VariantContentT& content)
			: Contents(new T(content))
		{
			Content.Owner = this;
		}

		operator T ()
		{
			if(Contents)
				return *Contents;

			return T();
		}

		operator T () const
		{
			if(Contents)
				return *Contents;

			return T();
		}

		Deferred& operator = (const Deferred& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		struct SafeContentAccess
		{
			T& operator * () const
			{
				if(!Owner->Contents)
				{
					boost::shared_ptr<T> copy(new T());
					Owner->Contents = copy;
				}

				return *(Owner->Contents);
			}

			T* operator -> ()
			{
				if(!Owner->Contents)
				{
					boost::shared_ptr<T> copy(new T());
					Owner->Contents = copy;
				}

				return Owner->Contents.get();
			}

			Deferred* Owner;
		} Content;

	private:
		boost::shared_ptr<T> Contents;
	};


	struct Undefined { Undefined() { } };

	typedef boost::iterator_range<std::wstring::const_iterator> IdentifierT;
	typedef boost::iterator_range<std::wstring::const_iterator> LiteralStringT;

	typedef boost::variant<Undefined, Integer32, UInteger32, Real32, LiteralStringT, bool> LiteralToken;

	typedef std::list<AST::IdentifierT> MemberAccess;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<struct PreOperatorStatement>,
			boost::recursive_wrapper<struct PostOperatorStatement>,
			boost::recursive_wrapper<struct Expression>
		> Parenthetical;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<struct PreOperatorStatement>,
			boost::recursive_wrapper<struct PostOperatorStatement>,
			boost::recursive_wrapper<struct Statement>
		> AnyStatement;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<IdentifierT>,
			boost::recursive_wrapper<LiteralToken>,
			boost::recursive_wrapper<struct Statement>,
			boost::recursive_wrapper<Parenthetical>
		> ExpressionComponentInternal;

	struct ExpressionComponent
	{
		Deferred<std::list<IdentifierT> > UnaryPrefixes;
		ExpressionComponentInternal Component;

		ExpressionComponent() { }
		ExpressionComponent(const LiteralToken& literaltoken)
			: Component(literaltoken)
		{ }
	};

	struct ExpressionFragment
	{
		IdentifierT Operator;
		ExpressionComponent Component;
	};

	struct Expression
	{
		ExpressionComponent First;
		std::list<Deferred<ExpressionFragment> > Remaining;

		Expression() { }

		Expression(const LiteralToken& token)
			: First(token)
		{ }

		Expression(const Deferred<ExpressionComponent>& component)
			: First(*component.Content)
		{ }
	};

	struct Statement
	{
		IdentifierT Identifier;
		std::list<Deferred<Expression> > Params;
	};

	struct PreOperatorStatement
	{
		IdentifierT Operator;
		Deferred<MemberAccess> Operand;
	};

	struct PostOperatorStatement
	{
		Deferred<MemberAccess> Operand;
		IdentifierT Operator;
	};


	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<Expression>,
			boost::recursive_wrapper<struct Assignment>
		> ExpressionOrAssignment;


	struct Assignment
	{
		std::list<IdentifierT> LHS;
		IdentifierT Operator;
		ExpressionOrAssignment RHS;
	};

	struct NamedFunctionParameter
	{
		IdentifierT Type;
		IdentifierT Name;
	};

	typedef boost::variant
		<
			Undefined,
			NamedFunctionParameter,
			Expression,
			boost::recursive_wrapper<struct FunctionReferenceSignature>
		> FunctionParameter;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<struct Entity>,
			boost::recursive_wrapper<struct PostfixEntity>
		> AnyEntity;

	typedef boost::variant
		<
			Undefined,
			AnyEntity,
			Assignment,
			AnyStatement,
			boost::recursive_wrapper<struct CodeBlock>
		> CodeBlockEntry;

	struct CodeBlock
	{
		std::list<Deferred<CodeBlockEntry> > Entries;
	};

	struct ChainedEntity
	{
		IdentifierT Identifier;
		std::list<Expression> Parameters;
		CodeBlock Code;
	};

	struct Entity
	{
		IdentifierT Identifier;
		std::list<Expression> Parameters;
		CodeBlock Code;
		std::list<ChainedEntity> Chain;
	};

	struct PostfixEntity
	{
		IdentifierT Identifier;
		std::list<Expression> Parameters;
		CodeBlock Code;
		IdentifierT PostfixIdentifier;
		std::list<Expression> PostfixParameters;
	};

	struct FunctionTag
	{
		IdentifierT TagName;
		std::list<Expression> Parameters;
	};

	typedef std::list<FunctionTag> FunctionTagList;

	struct Function
	{
		IdentifierT Name;
		std::list<FunctionParameter> Parameters;
		Expression Return;
		FunctionTagList Tags;
		CodeBlock Code;
	};

	struct StructureMemberVariable
	{
		IdentifierT Type;
		IdentifierT Name;
	};

	struct StructureMemberFunctionRef
	{
		IdentifierT Name;
		std::list<IdentifierT> ParamTypes;
		IdentifierT ReturnType;
	};


	typedef boost::variant
		<
			Undefined,
			StructureMemberVariable,
			StructureMemberFunctionRef
		> StructureMember;


	struct Structure
	{
		IdentifierT Identifier;
		std::list<StructureMember> Members;
	};

	struct FunctionReferenceSignature
	{
		IdentifierT Identifier;
		std::list<IdentifierT> ParamTypes;
		IdentifierT ReturnType;
	};

	typedef boost::variant
		<
			Undefined,
			Structure,
			CodeBlock,
			Function
		> MetaEntity;

	typedef std::list<IdentifierT> ParamTypeList;

	
	typedef std::list<MetaEntity> Program;
}


BOOST_FUSION_ADAPT_STRUCT
(
	AST::Function,
	(AST::IdentifierT, Name)
	(std::list<AST::FunctionParameter>, Parameters)
	(AST::Expression, Return)
	(AST::FunctionTagList, Tags)
	(AST::CodeBlock, Code)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::ExpressionComponent>,
	(AST::Deferred<std::list<AST::IdentifierT> >, Content->UnaryPrefixes)
	(AST::ExpressionComponentInternal, Content->Component)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::ExpressionFragment>,
	(AST::IdentifierT, Content->Operator)
	(AST::ExpressionComponent, Content->Component)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Expression,
	(AST::ExpressionComponent, First)
	(std::list<AST::Deferred<AST::ExpressionFragment> >, Remaining)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Statement,
	(AST::IdentifierT, Identifier)
	(std::list<AST::Deferred<AST::Expression> >, Params)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::PreOperatorStatement>,
	(AST::IdentifierT, Content->Operator)
	(AST::Deferred<std::list<AST::IdentifierT> >, Content->Operand)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::PostOperatorStatement>,
	(AST::Deferred<std::list<AST::IdentifierT> >, Content->Operand)
	(AST::IdentifierT, Content->Operator)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Assignment,
	(std::list<AST::IdentifierT>, LHS)
	(AST::IdentifierT, Operator)
	(AST::ExpressionOrAssignment, RHS)
)




BOOST_FUSION_ADAPT_STRUCT
(
	AST::ChainedEntity,
	(AST::IdentifierT, Identifier)
	(std::list<AST::Expression>, Parameters)
	(AST::CodeBlock, Code)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Entity,
	(AST::IdentifierT, Identifier)
	(std::list<AST::Expression>, Parameters)
	(AST::CodeBlock, Code)
	(std::list<AST::ChainedEntity>, Chain)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::PostfixEntity,
	(AST::IdentifierT, Identifier)
	(std::list<AST::Expression>, Parameters)
	(AST::CodeBlock, Code)
	(AST::IdentifierT, PostfixIdentifier)
	(std::list<AST::Expression>, PostfixParameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::StructureMemberFunctionRef,
	(AST::IdentifierT, Name)
	(std::list<AST::IdentifierT>, ParamTypes)
	(AST::IdentifierT, ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::StructureMemberVariable,
	(AST::IdentifierT, Type)
	(AST::IdentifierT, Name)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::NamedFunctionParameter,
	(AST::IdentifierT, Type)
	(AST::IdentifierT, Name)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Structure,
	(AST::IdentifierT, Identifier)
	(std::list<AST::StructureMember>, Members)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionReferenceSignature,
	(AST::IdentifierT, Identifier)
	(std::list<AST::IdentifierT>, ParamTypes)
	(AST::IdentifierT, ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionTag,
	(AST::IdentifierT, TagName)
	(std::list<AST::Expression>, Parameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::CodeBlock,
	(std::list<AST::Deferred<AST::CodeBlockEntry> >, Entries)
)

