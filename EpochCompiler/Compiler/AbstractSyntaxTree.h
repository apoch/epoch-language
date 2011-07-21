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

#include "Utility/Memory/OneWayAllocator.h"


namespace AST
{

	template <typename T>
	inline T* Allocate()
	{
		return new T;
	}

	template <typename T>
	void Deallocate(T* ptr)
	{
		delete ptr;
	}

	struct Expression;
	struct ExpressionComponent;
	struct ExpressionFragment;
	struct PreOperatorStatement;
	struct PostOperatorStatement;
	struct IdentifierListRaw;
	struct ExpressionComponentInternal;
	struct Statement;
	struct FunctionReferenceSignature;
	struct Assignment;
	struct ChainedEntity;
	struct FunctionParameter;
	struct PostfixEntity;
	struct CodeBlock;

	template<> inline Expression* Allocate<struct Expression>() { return Memory::OneWayAllocateObject<Expression>(1); }
	template<> inline ExpressionComponent* Allocate<struct ExpressionComponent>() { return Memory::OneWayAllocateObject<ExpressionComponent>(1); }
	template<> inline ExpressionFragment* Allocate<struct ExpressionFragment>() { return Memory::OneWayAllocateObject<ExpressionFragment>(1); }
	template<> inline PreOperatorStatement* Allocate<struct PreOperatorStatement>() { return Memory::OneWayAllocateObject<PreOperatorStatement>(1); }
	template<> inline PostOperatorStatement* Allocate<struct PostOperatorStatement>() { return Memory::OneWayAllocateObject<PostOperatorStatement>(1); }
	template<> inline IdentifierListRaw* Allocate<struct IdentifierListRaw>() { return Memory::OneWayAllocateObject<IdentifierListRaw>(1); }
	template<> inline ExpressionComponentInternal* Allocate<struct ExpressionComponentInternal>() { return Memory::OneWayAllocateObject<ExpressionComponentInternal>(1); }
	template<> inline Statement* Allocate<struct Statement>() { return Memory::OneWayAllocateObject<Statement>(1); }
	template<> inline FunctionReferenceSignature* Allocate<FunctionReferenceSignature>() { return Memory::OneWayAllocateObject<FunctionReferenceSignature>(1); }
	template<> inline Assignment* Allocate<Assignment>() { return Memory::OneWayAllocateObject<Assignment>(1); }
	template<> inline ChainedEntity* Allocate<ChainedEntity>() { return Memory::OneWayAllocateObject<ChainedEntity>(1); }
	template<> inline FunctionParameter* Allocate<FunctionParameter>() { return Memory::OneWayAllocateObject<FunctionParameter>(1); }
	template<> inline PostfixEntity* Allocate<PostfixEntity>() { return Memory::OneWayAllocateObject<PostfixEntity>(1); }
	template<> inline CodeBlock* Allocate<CodeBlock>() { return Memory::OneWayAllocateObject<CodeBlock>(1); }

	template<> inline void Deallocate(Expression* p) { Memory::OneWayRecordDeallocObject<Expression>(p); }
	template<> inline void Deallocate(ExpressionComponent* p) { Memory::OneWayRecordDeallocObject<ExpressionComponent>(p); }
	template<> inline void Deallocate(ExpressionFragment* p) { Memory::OneWayRecordDeallocObject<ExpressionFragment>(p); }
	template<> inline void Deallocate(PreOperatorStatement* p) { Memory::OneWayRecordDeallocObject<PreOperatorStatement>(p); }
	template<> inline void Deallocate(PostOperatorStatement* p) { Memory::OneWayRecordDeallocObject<PostOperatorStatement>(p); }
	template<> inline void Deallocate(IdentifierListRaw* p) { Memory::OneWayRecordDeallocObject<IdentifierListRaw>(p); }
	template<> inline void Deallocate(ExpressionComponentInternal* p) { Memory::OneWayRecordDeallocObject<ExpressionComponentInternal>(p); }
	template<> inline void Deallocate(Statement* p) { Memory::OneWayRecordDeallocObject<Statement>(p); }
	template<> inline void Deallocate(FunctionReferenceSignature* p) { Memory::OneWayRecordDeallocObject<FunctionReferenceSignature>(p); }
	template<> inline void Deallocate(Assignment* p) { Memory::OneWayRecordDeallocObject<Assignment>(p); }
	template<> inline void Deallocate(ChainedEntity* p) { Memory::OneWayRecordDeallocObject<ChainedEntity>(p); }
	template<> inline void Deallocate(FunctionParameter* p) { Memory::OneWayRecordDeallocObject<FunctionParameter>(p); }
	template<> inline void Deallocate(PostfixEntity* p) { Memory::OneWayRecordDeallocObject<PostfixEntity>(p); }
	template<> inline void Deallocate(CodeBlock* p) { Memory::OneWayRecordDeallocObject<CodeBlock>(p); }

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

	void intrusive_ptr_add_ref(struct FunctionReferenceSignature* expr);
	void intrusive_ptr_release(struct FunctionReferenceSignature* expr);

	void intrusive_ptr_add_ref(struct Assignment* expr);
	void intrusive_ptr_release(struct Assignment* expr);

	void intrusive_ptr_add_ref(struct ChainedEntity* entity);
	void intrusive_ptr_release(struct ChainedEntity* entity);

	void intrusive_ptr_add_ref(struct FunctionParameter* param);
	void intrusive_ptr_release(struct FunctionParameter* param);

	void intrusive_ptr_add_ref(struct PostfixEntity* entity);
	void intrusive_ptr_release(struct PostfixEntity* entity);

	void intrusive_ptr_add_ref(struct CodeBlock* block);
	void intrusive_ptr_release(struct CodeBlock* block);


	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct Deferred
	{
		Deferred()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		Deferred(const T& expr)
			: Contents(new (Allocate<T>()) T(expr))
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
			: Contents(new (Allocate<T>()) T(content))
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
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

			T* operator -> () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

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
			: Contents(new (Allocate<T>()) T(expr))
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
			: Contents(new (Allocate<T>()) T(content))
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
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

			T* operator -> () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

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
		typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > ContainedT;

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
			Deferred<struct PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> >,
			Deferred<struct PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> >,
			Deferred<struct Expression, boost::intrusive_ptr<Expression> >
		> Parenthetical;

	typedef boost::variant
		<
			Undefined,
			Deferred<struct PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> >,
			Deferred<struct PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> >,
			Deferred<struct Statement, boost::intrusive_ptr<Statement> >
		> AnyStatement;

	typedef boost::variant
		<
			Undefined,
			IdentifierT,
			LiteralToken,
			Deferred<struct Statement, boost::intrusive_ptr<Statement> >,
			Parenthetical
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
		std::vector<Deferred<ExpressionFragment, boost::intrusive_ptr<ExpressionFragment> >, Memory::OneWayAlloc<Deferred<ExpressionFragment, boost::intrusive_ptr<ExpressionFragment> > > > Remaining;

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
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Params;

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
			Deferred<Expression, boost::intrusive_ptr<Expression> >,
			Deferred<Assignment, boost::intrusive_ptr<Assignment> >
		> ExpressionOrAssignment;


	struct Assignment
	{
		IdentifierList LHS;
		IdentifierT Operator;
		ExpressionOrAssignment RHS;

		long RefCount;

		Assignment()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		Assignment(const Assignment&);
		Assignment& operator = (const Assignment&);
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
			Deferred<struct FunctionReferenceSignature, boost::intrusive_ptr<FunctionReferenceSignature> >
		> FunctionParameterVariant;

	struct FunctionParameter
	{
		FunctionParameterVariant V;

		long RefCount;

		FunctionParameter()
			: RefCount(0)
		{ }

		FunctionParameter(const FunctionParameterVariant& v)
			: V(v),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		FunctionParameter(const FunctionParameter&);
		FunctionParameter& operator = (const FunctionParameter&);
	};

	typedef Deferred<FunctionParameter, boost::intrusive_ptr<FunctionParameter> > FunctionParameterDeferred;

	typedef boost::variant
		<
			Undefined,
			Deferred<struct Entity>,
			Deferred<struct PostfixEntity, boost::intrusive_ptr<PostfixEntity> >
		> AnyEntity;

	typedef boost::variant
		<
			Undefined,
			AnyEntity,
			Deferred<Assignment, boost::intrusive_ptr<Assignment> >,
			AnyStatement,
			Deferred<CodeBlock, boost::intrusive_ptr<CodeBlock> >
		> CodeBlockEntry;

	struct CodeBlock
	{
		std::vector<Deferred<CodeBlockEntry>, Memory::OneWayAlloc<Deferred<CodeBlockEntry> > > Entries;

		long RefCount;

		CodeBlock()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		CodeBlock(const CodeBlock&);
		CodeBlock& operator = (const CodeBlock&);
	};

	typedef Deferred<CodeBlock, boost::intrusive_ptr<CodeBlock> > CodeBlockDeferred;

	struct ChainedEntity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Parameters;
		CodeBlockDeferred Code;

		long RefCount;

		ChainedEntity()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		ChainedEntity(const ChainedEntity&);
		ChainedEntity& operator = (const ChainedEntity&);
	};

	typedef Deferred<ChainedEntity, boost::intrusive_ptr<ChainedEntity> > ChainedEntityDeferred;

	struct Entity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Parameters;
		CodeBlockDeferred Code;
		std::vector<ChainedEntityDeferred, Memory::OneWayAlloc<ChainedEntityDeferred> > Chain;
	};

	struct PostfixEntity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Parameters;
		CodeBlockDeferred Code;
		IdentifierT PostfixIdentifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > PostfixParameters;

		long RefCount;

		PostfixEntity()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		PostfixEntity(const PostfixEntity&);
		PostfixEntity& operator = (const PostfixEntity&);
	};

	struct FunctionTag
	{
		IdentifierT TagName;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Parameters;
	};

	typedef std::vector<FunctionTag, Memory::OneWayAlloc<FunctionTag> > FunctionTagList;

	struct Function
	{
		IdentifierT Name;
		std::vector<FunctionParameterDeferred, Memory::OneWayAlloc<FunctionParameterDeferred> > Parameters;
		Deferred<Expression, boost::intrusive_ptr<Expression> > Return;
		FunctionTagList Tags;
		CodeBlockDeferred Code;
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
		std::vector<StructureMember, Memory::OneWayAlloc<StructureMember> > Members;
	};

	struct FunctionReferenceSignature
	{
		IdentifierT Identifier;
		IdentifierList ParamTypes;
		IdentifierT ReturnType;

		FunctionReferenceSignature()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		FunctionReferenceSignature(const FunctionReferenceSignature&);
		FunctionReferenceSignature& operator = (const FunctionReferenceSignature&);
	};

	typedef boost::variant
		<
			Undefined,
			Structure,
			CodeBlockDeferred,
			Function
		> MetaEntity;

	typedef IdentifierList ParamTypeList;

	
	typedef std::vector<MetaEntity, Memory::OneWayAlloc<MetaEntity> > Program;
}


typedef AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> > ExpressionDeferred;
typedef std::vector<AST::FunctionParameterDeferred, Memory::OneWayAlloc<AST::FunctionParameterDeferred> > FunctionParamVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Function,
	(AST::IdentifierT, Name)
	(FunctionParamVec, Parameters)
	(ExpressionDeferred, Return)
	(AST::FunctionTagList, Tags)
	(AST::CodeBlockDeferred, Code)
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

typedef std::vector<ExpressionFragmentDeferred, Memory::OneWayAlloc<ExpressionFragmentDeferred> > ExpressionFragmentDeferredVec;

BOOST_FUSION_ADAPT_STRUCT
(
	ExpressionDeferred,
	(ExpressionComponentDeferred, Content->First)
	(ExpressionFragmentDeferredVec, Content->Remaining)
)

typedef std::vector<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >, Memory::OneWayAlloc<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> > > > ExpressionListT;
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

typedef AST::Deferred<AST::Assignment, boost::intrusive_ptr<AST::Assignment> > DeferredAssignment;

BOOST_FUSION_ADAPT_STRUCT
(
	DeferredAssignment,
	(AST::IdentifierList, Content->LHS)
	(AST::IdentifierT, Content->Operator)
	(AST::ExpressionOrAssignment, Content->RHS)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::ChainedEntityDeferred,
	(AST::IdentifierT, Content->Identifier)
	(ExpressionListT, Content->Parameters)
	(AST::CodeBlockDeferred, Content->Code)
)

typedef std::vector<AST::ChainedEntityDeferred, Memory::OneWayAlloc<AST::ChainedEntityDeferred> > ChainedEntityVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::Entity>,
	(AST::IdentifierT, Content->Identifier)
	(ExpressionListT, Content->Parameters)
	(AST::CodeBlockDeferred, Content->Code)
	(ChainedEntityVec, Content->Chain)
)

typedef AST::Deferred<AST::PostfixEntity, boost::intrusive_ptr<AST::PostfixEntity> > DeferredPostfixEntity;

BOOST_FUSION_ADAPT_STRUCT
(
	DeferredPostfixEntity,
	(AST::IdentifierT, Content->Identifier)
	(ExpressionListT, Content->Parameters)
	(AST::CodeBlockDeferred, Content->Code)
	(AST::IdentifierT, Content->PostfixIdentifier)
	(ExpressionListT, Content->PostfixParameters)
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

typedef std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> > StructureMemberVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Structure,
	(AST::IdentifierT, Identifier)
	(StructureMemberVec, Members)
)

typedef AST::Deferred<AST::FunctionReferenceSignature, boost::intrusive_ptr<AST::FunctionReferenceSignature> > DeferredFunctionRefSig;

BOOST_FUSION_ADAPT_STRUCT
(
	DeferredFunctionRefSig,
	(AST::IdentifierT, Content->Identifier)
	(AST::IdentifierList, Content->ParamTypes)
	(AST::IdentifierT, Content->ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionTag,
	(AST::IdentifierT, TagName)
	(ExpressionListT, Parameters)
)

typedef std::vector<AST::Deferred<AST::CodeBlockEntry>, Memory::OneWayAlloc<AST::Deferred<AST::CodeBlockEntry> > > CodeBlockEntryVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::CodeBlockDeferred,
	(CodeBlockEntryVec, Content->Entries)
)

typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > IdentifierVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::IdentifierList,
	(IdentifierVec, Content->Container)
)
