//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing expressions
//

#pragma once


// Dependencies
#include "Compiler/ExportDef.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>


namespace IRSemantics
{

	// Forward declarations
	class Statement;
	class PreOpStatement;
	class PostOpStatement;
	class Program;


	class ExpressionAtom
	{
	// Destruction
	public:
		virtual ~ExpressionAtom()
		{ }
	};


	class ExpressionComponent
	{
	// Construction and destruction
	public:
		explicit ExpressionComponent(const std::vector<StringHandle>& prefixes);
		~ExpressionComponent();

	// Non-copyable
	private:
		ExpressionComponent(const ExpressionComponent& other);
		ExpressionComponent& operator = (const ExpressionComponent& rhs);

	// Adjustment of assigned atom
	public:
		void SetAtom(ExpressionAtom* atom);

	// Accessors
	public:
		const std::vector<StringHandle>& GetPrefixes() const
		{ return UnaryPrefixes; }

		const ExpressionAtom* GetAtom() const
		{ return Atom; }

	// Internal state
	private:
		std::vector<StringHandle> UnaryPrefixes;
		ExpressionAtom* Atom;
	};

	class ExpressionFragment
	{
	// Construction and destruction
	public:
		ExpressionFragment(StringHandle operatorname, ExpressionComponent* component);
		~ExpressionFragment();

	// Non-copyable
	private:
		ExpressionFragment(const ExpressionFragment& other);
		ExpressionFragment& operator = (const ExpressionFragment& rhs);

	// Internal state
	private:
		StringHandle OperatorName;
		ExpressionComponent* Component;
	};

	class Expression
	{
	// Construction and destruction
	public:
		explicit Expression(ExpressionComponent* first);
		~Expression();

	// Addition of fragments
	public:
		void AddFragment(ExpressionFragment* fragment);

	// Accessors
	public:
		const ExpressionComponent& GetFirst() const
		{ return *First; }

		const std::vector<ExpressionFragment*>& GetRemaining() const
		{ return Remaining; }

	// Type system
	public:
		EPOCHCOMPILER VM::EpochTypeID GetEpochType(const Program& program) const;

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program);

	// Internal state
	private:
		ExpressionComponent* First;
		std::vector<ExpressionFragment*> Remaining;
	};


	class Parenthetical
	{
	// Destruction
	public:
		virtual ~Parenthetical()
		{ }
	};


	class ParentheticalPreOp : public Parenthetical
	{
	// Construction and destruction
	public:
		explicit ParentheticalPreOp(PreOpStatement* statement);
		virtual ~ParentheticalPreOp();

	// Non-copyable
	private:
		ParentheticalPreOp(const ParentheticalPreOp& other);
		ParentheticalPreOp& operator = (const ParentheticalPreOp& rhs);

	// Internal state
	private:
		PreOpStatement* MyStatement;
	};

	class ParentheticalPostOp : public Parenthetical
	{
	// Construction and destruction
	public:
		explicit ParentheticalPostOp(PostOpStatement* statement);
		virtual ~ParentheticalPostOp();

	// Non-copyable
	private:
		ParentheticalPostOp(const ParentheticalPostOp& other);
		ParentheticalPostOp& operator = (const ParentheticalPostOp& rhs);

	// Internal state
	private:
		PostOpStatement* MyStatement;
	};


	class ExpressionAtomParenthetical : public ExpressionAtom
	{
	// Construction and destruction
	public:
		explicit ExpressionAtomParenthetical(Parenthetical* parenthetical);
		virtual ~ExpressionAtomParenthetical();

	// Non-copyable
	private:
		ExpressionAtomParenthetical(const ExpressionAtomParenthetical& other);
		ExpressionAtomParenthetical& operator = (const ExpressionAtomParenthetical& rhs);

	// Internal state
	private:
		Parenthetical* MyParenthetical;
	};

	class ExpressionAtomIdentifier : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomIdentifier(StringHandle identifier)
			: Identifier(identifier)
		{ }

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

	// Internal state
	private:
		StringHandle Identifier;
	};

	class ExpressionAtomLiteralString : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralString(StringHandle handle)
			: Handle(handle)
		{ }

	// Accessors
	public:
		StringHandle GetValue() const
		{ return Handle; }

	// Internal state
	private:
		StringHandle Handle;
	};

	class ExpressionAtomLiteralBoolean : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralBoolean(bool value)
			: Value(value)
		{ }

	// Accessors
	public:
		bool GetValue() const
		{ return Value; }

	// Internal state
	private:
		bool Value;
	};

	class ExpressionAtomLiteralInteger32 : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralInteger32(Integer32 value)
			: Value(value)
		{ }

	// Accessors
	public:
		Integer32 GetValue() const
		{ return Value; }

	// Internal state
	private:
		Integer32 Value;
	};

	class ExpressionAtomLiteralReal32 : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralReal32(Real32 value)
			: Value(value)
		{ }

	// Accessors
	public:
		Real32 GetValue() const
		{ return Value; }

	// Internal state
	private:
		Real32 Value;
	};

	class ExpressionAtomStatement : public ExpressionAtom
	{
	// Construction and destruction
	public:
		explicit ExpressionAtomStatement(Statement* statement);
		virtual ~ExpressionAtomStatement();

	// Non-copyable
	private:
		ExpressionAtomStatement(const ExpressionAtomStatement& other);
		ExpressionAtomStatement& operator = (const ExpressionAtomStatement& rhs);

	// Accessors
	public:
		Statement& GetStatement() const
		{ return *MyStatement; }

	// Internal state
	private:
		Statement* MyStatement;
	};

}

