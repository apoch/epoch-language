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

	void intrusive_ptr_add_ref(struct IdentifierListRaw* expr);
	void intrusive_ptr_release(struct IdentifierListRaw* expr);

	void intrusive_ptr_add_ref(struct ExpressionComponentInternal* expr);
	void intrusive_ptr_release(struct ExpressionComponentInternal* expr);

	void intrusive_ptr_add_ref(struct Statement* expr);
	void intrusive_ptr_release(struct Statement* expr);


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

			T* operator -> () const
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

	protected:
		mutable PointerType Contents;
	};

	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct DeferredContainer
	{
		typedef typename T::ContainedT::value_type value_type;
		typedef typename T::ContainedT::iterator iterator;

		DeferredContainer()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		DeferredContainer(const T& expr)
			: Contents(new T(expr))
		{
			Content.Owner = this;
		}

		DeferredContainer(const DeferredContainer& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		template <typename VariantContentT>
		DeferredContainer(const VariantContentT& content)
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

		DeferredContainer& operator = (const DeferredContainer& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		void insert(const iterator& pos, const value_type& value)
		{
			Content->Container.insert(pos, value);
		}

		iterator begin()
		{
			return Content->Container.begin();
		}

		iterator end()
		{
			return Content->Container.end();
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

			T* operator -> () const
			{
				if(!Owner->Contents)
				{
					PointerType copy(new T());
					Owner->Contents = copy;
				}

				return Owner->Contents.get();
			}

			DeferredContainer* Owner;
		} Content;

	protected:
		mutable PointerType Contents;
	};


	struct Undefined
	{
		Undefined() { }
		Undefined(const boost::spirit::unused_type&) { }
	};

	typedef boost::iterator_range<std::wstring::const_iterator> IdentifierT;
	typedef boost::iterator_range<std::wstring::const_iterator> LiteralStringT;

	typedef boost::variant<Undefined, Integer32, UInteger32, Real32, LiteralStringT, bool> LiteralToken;

	struct IdentifierListRaw
	{
		typedef std::vector<AST::IdentifierT> ContainedT;

		ContainedT Container;

		long RefCount;

		IdentifierListRaw()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		IdentifierListRaw(const IdentifierListRaw&);
		IdentifierListRaw& operator = (const IdentifierListRaw&);
	};

	typedef DeferredContainer<IdentifierListRaw, boost::intrusive_ptr<IdentifierListRaw> > IdentifierList;
	typedef IdentifierList MemberAccess;

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
			boost::recursive_wrapper<Deferred<struct Statement, boost::intrusive_ptr<Statement> > >
		> AnyStatement;

	typedef boost::variant
		<
			Undefined,
			boost::recursive_wrapper<IdentifierT>,
			boost::recursive_wrapper<LiteralToken>,
			boost::recursive_wrapper<Deferred<struct Statement, boost::intrusive_ptr<Statement> > >,
			boost::recursive_wrapper<Parenthetical>
		> ExpressionComponentInternalVariant;

	struct ExpressionComponentInternal
	{
		ExpressionComponentInternalVariant V;

		long RefCount;

		ExpressionComponentInternal()
			: RefCount(0)
		{ }

		template <typename T>
		ExpressionComponentInternal(const T& v)
			: V(v),
			  RefCount(0)
		{
		}

	// Non-copyable
	private:
		ExpressionComponentInternal(const ExpressionComponentInternal&);
		ExpressionComponentInternal& operator = (const ExpressionComponentInternal&);
	};

	struct ExpressionComponent
	{
		IdentifierList UnaryPrefixes;
		Deferred<ExpressionComponentInternal, boost::intrusive_ptr<ExpressionComponentInternal> > Component;

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

		Statement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		Statement(const Statement&);
		Statement& operator = (const Statement&);
	};

	struct PreOperatorStatement
	{
		IdentifierT Operator;
		MemberAccess Operand;

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
		MemberAccess Operand;
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
		IdentifierList LHS;
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
		IdentifierList ParamTypes;
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
		IdentifierList ParamTypes;
		IdentifierT ReturnType;
	};

	typedef boost::variant
		<
			Undefined,
			Structure,
			CodeBlock,
			Function
		> MetaEntity;

	typedef IdentifierList ParamTypeList;

	
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

typedef AST::Deferred<AST::ExpressionComponentInternal, boost::intrusive_ptr<AST::ExpressionComponentInternal> > DeferredECI;

BOOST_FUSION_ADAPT_STRUCT
(
	DeferredECI,
	(AST::ExpressionComponentInternalVariant, Content->V)
)

typedef AST::Deferred<AST::ExpressionComponent, boost::intrusive_ptr<AST::ExpressionComponent> > ExpressionComponentDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	ExpressionComponentDeferred,
	(AST::IdentifierList, Content->UnaryPrefixes)
	(DeferredECI, Content->Component)
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
typedef AST::Deferred<AST::Statement, boost::intrusive_ptr<AST::Statement> > DeferredStatement;

BOOST_FUSION_ADAPT_STRUCT
(
	DeferredStatement,
	(AST::IdentifierT, Content->Identifier)
	(ExpressionListT, Content->Params)
)

typedef AST::Deferred<AST::PreOperatorStatement, boost::intrusive_ptr<AST::PreOperatorStatement> > PreOperatorStatementDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	PreOperatorStatementDeferred,
	(AST::IdentifierT, Content->Operator)
	(AST::IdentifierList, Content->Operand)
)

typedef AST::Deferred<AST::PostOperatorStatement, boost::intrusive_ptr<AST::PostOperatorStatement> > PostOperatorStatementDeferred;

BOOST_FUSION_ADAPT_STRUCT
(
	PostOperatorStatementDeferred,
	(AST::IdentifierList, Content->Operand)
	(AST::IdentifierT, Content->Operator)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Assignment,
	(AST::IdentifierList, LHS)
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
	(AST::IdentifierList, ParamTypes)
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
	(AST::IdentifierList, ParamTypes)
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

BOOST_FUSION_ADAPT_STRUCT
(
	AST::IdentifierList,
	(std::vector<AST::IdentifierT>, Content->Container)
)
