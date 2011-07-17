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


namespace AST
{

	void intrusive_ptr_add_ref(struct Expression* expr);
	void intrusive_ptr_release(struct Expression* expr);

	void intrusive_ptr_add_ref(struct ExpressionComponent* expr);
	void intrusive_ptr_release(struct ExpressionComponent* expr);

	void intrusive_ptr_add_ref(struct ExpressionFragment* expr);
	void intrusive_ptr_release(struct ExpressionFragment* expr);

	void intrusive_ptr_add_ref(struct PreOperatorStatement* expr);
	void intrusive_ptr_release(struct PreOperatorStatement* expr);

	void intrusive_ptr_add_ref(struct PostOperatorStatement* expr);
	void intrusive_ptr_release(struct PostOperatorStatement* expr);


	template <typename T, typename PointerType = boost::shared_ptr<T> >
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
					PointerType copy(new T());
					Owner->Contents = copy;
				}

				return *(Owner->Contents);
			}

			T* operator -> ()
			{
				if(!Owner->Contents)
				{
					PointerType copy(new T());
					Owner->Contents = copy;
				}

				return Owner->Contents.get();
			}

			Deferred* Owner;
		} Content;

	private:
		PointerType Contents;
	};


	struct Undefined { Undefined() { } };

	typedef boost::iterator_range<std::wstring::const_iterator> IdentifierT;
	typedef boost::iterator_range<std::wstring::const_iterator> LiteralStringT;

	typedef boost::variant<Undefined, Integer32, UInteger32, Real32, LiteralStringT, bool> LiteralToken;

	typedef std::vector<AST::IdentifierT> MemberAccess;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<Deferred<struct PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> > >,
			boost::recursive_wrapper<Deferred<struct PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> > >,
			boost::recursive_wrapper<Deferred<struct Expression, boost::intrusive_ptr<Expression> > >
		> Parenthetical;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<Deferred<struct PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> > >,
			boost::recursive_wrapper<Deferred<struct PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> > >,
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
		Deferred<std::vector<IdentifierT> > UnaryPrefixes;
		ExpressionComponentInternal Component;

		long RefCount;

		ExpressionComponent()
			: RefCount(0)
		{ }

		ExpressionComponent(const LiteralToken& literaltoken)
			: Component(literaltoken),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		ExpressionComponent(const ExpressionComponent&);
		ExpressionComponent& operator = (const ExpressionComponent&);
	};

	struct ExpressionFragment
	{
		IdentifierT Operator;
		Deferred<ExpressionComponent, boost::intrusive_ptr<ExpressionComponent> > Component;

		ExpressionFragment()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		ExpressionFragment(const ExpressionFragment&);
		ExpressionFragment& operator = (const ExpressionFragment&);
	};

	struct Expression
	{
		Deferred<ExpressionComponent, boost::intrusive_ptr<ExpressionComponent> > First;
		std::vector<Deferred<ExpressionFragment, boost::intrusive_ptr<ExpressionFragment> > > Remaining;

		long RefCount;

		Expression()
			: RefCount(0)
		{ }

		Expression(const LiteralToken& token)
			: First(token),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		Expression(const Expression&);
		Expression& operator = (const Expression&);
	};

	struct Statement
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > Params;
	};

	struct PreOperatorStatement
	{
		IdentifierT Operator;
		Deferred<MemberAccess> Operand;

		PreOperatorStatement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		PreOperatorStatement(const PreOperatorStatement&);
		PreOperatorStatement& operator = (const PreOperatorStatement&);
	};

	struct PostOperatorStatement
	{
		Deferred<MemberAccess> Operand;
		IdentifierT Operator;

		PostOperatorStatement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		PostOperatorStatement(const PostOperatorStatement&);
		PostOperatorStatement& operator = (const PostOperatorStatement&);
	};


	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<Deferred<Expression, boost::intrusive_ptr<Expression> > >,
			boost::recursive_wrapper<struct Assignment>
		> ExpressionOrAssignment;


	struct Assignment
	{
		Deferred<std::vector<IdentifierT> > LHS;
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
			Deferred<Expression, boost::intrusive_ptr<Expression> >,
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
		std::vector<Deferred<CodeBlockEntry> > Entries;
	};

	struct ChainedEntity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > Parameters;
		CodeBlock Code;
	};

	struct Entity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > Parameters;
		CodeBlock Code;
		std::vector<ChainedEntity> Chain;
	};

	struct PostfixEntity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > Parameters;
		CodeBlock Code;
		IdentifierT PostfixIdentifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > PostfixParameters;
	};

	struct FunctionTag
	{
		IdentifierT TagName;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> > > Parameters;
	};

	typedef std::vector<FunctionTag> FunctionTagList;

	struct Function
	{
		IdentifierT Name;
		std::vector<FunctionParameter> Parameters;
		Deferred<Expression, boost::intrusive_ptr<Expression> > Return;
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
		Deferred<std::vector<IdentifierT> > ParamTypes;
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
		std::vector<StructureMember> Members;
	};

	struct FunctionReferenceSignature
	{
		IdentifierT Identifier;
		Deferred<std::vector<IdentifierT> > ParamTypes;
		IdentifierT ReturnType;
	};

	typedef boost::variant
		<
			Undefined,
			Structure,
			CodeBlock,
			Function
		> MetaEntity;

	typedef std::vector<IdentifierT> ParamTypeList;

	
	typedef std::vector<MetaEntity> Program;
}


typedef AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> > ExpressionDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Function,
	(AST::IdentifierT, Name)
	(std::vector<AST::FunctionParameter>, Parameters)
	(ExpressionDeferred, Return)
	(AST::FunctionTagList, Tags)
	(AST::CodeBlock, Code)
)

typedef AST::Deferred<AST::ExpressionComponent, boost::intrusive_ptr<AST::ExpressionComponent> > ExpressionComponentDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	ExpressionComponentDeferred,
	(AST::Deferred<std::vector<AST::IdentifierT> >, Content->UnaryPrefixes)
	(AST::ExpressionComponentInternal, Content->Component)
)

typedef AST::Deferred<AST::ExpressionFragment, boost::intrusive_ptr<AST::ExpressionFragment> > ExpressionFragmentDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	ExpressionFragmentDeferred,
	(AST::IdentifierT, Content->Operator)
	(ExpressionComponentDeferred, Content->Component)
)

BOOST_FUSION_ADAPT_STRUCT
(
	ExpressionDeferred,
	(ExpressionComponentDeferred, Content->First)
	(std::vector<ExpressionFragmentDeferred>, Content->Remaining)
)

typedef std::vector<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> > > ExpressionListT;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Statement,
	(AST::IdentifierT, Identifier)
	(ExpressionListT, Params)
)

typedef AST::Deferred<AST::PreOperatorStatement, boost::intrusive_ptr<AST::PreOperatorStatement> > PreOperatorStatementDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	PreOperatorStatementDeferred,
	(AST::IdentifierT, Content->Operator)
	(AST::Deferred<std::vector<AST::IdentifierT> >, Content->Operand)
)

typedef AST::Deferred<AST::PostOperatorStatement, boost::intrusive_ptr<AST::PostOperatorStatement> > PostOperatorStatementDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	PostOperatorStatementDeferred,
	(AST::Deferred<std::vector<AST::IdentifierT> >, Content->Operand)
	(AST::IdentifierT, Content->Operator)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Assignment,
	(AST::Deferred<std::vector<AST::IdentifierT> >, LHS)
	(AST::IdentifierT, Operator)
	(AST::ExpressionOrAssignment, RHS)
)




BOOST_FUSION_ADAPT_STRUCT
(
	AST::ChainedEntity,
	(AST::IdentifierT, Identifier)
	(ExpressionListT, Parameters)
	(AST::CodeBlock, Code)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Entity,
	(AST::IdentifierT, Identifier)
	(ExpressionListT, Parameters)
	(AST::CodeBlock, Code)
	(std::vector<AST::ChainedEntity>, Chain)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::PostfixEntity,
	(AST::IdentifierT, Identifier)
	(ExpressionListT, Parameters)
	(AST::CodeBlock, Code)
	(AST::IdentifierT, PostfixIdentifier)
	(ExpressionListT, PostfixParameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::StructureMemberFunctionRef,
	(AST::IdentifierT, Name)
	(AST::Deferred<std::vector<AST::IdentifierT> >, ParamTypes)
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
	(std::vector<AST::StructureMember>, Members)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionReferenceSignature,
	(AST::IdentifierT, Identifier)
	(AST::Deferred<std::vector<AST::IdentifierT> >, ParamTypes)
	(AST::IdentifierT, ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionTag,
	(AST::IdentifierT, TagName)
	(ExpressionListT, Parameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::CodeBlock,
	(std::vector<AST::Deferred<AST::CodeBlockEntry> >, Entries)
)

