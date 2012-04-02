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

#include "Metadata/CompileTimeParams.h"

#include <vector>


namespace IRSemantics
{

	// Forward declarations
	class Statement;
	class PreOpStatement;
	class PostOpStatement;
	class CodeBlock;
	class Program;
	struct InferenceContext;


	class ExpressionAtom
	{
	// Destruction
	public:
		virtual ~ExpressionAtom()
		{ }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const = 0;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index) = 0;
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr) = 0;
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const = 0;
	};


	class Expression
	{
	// Construction and destruction
	public:
		Expression();
		~Expression();

	// Type system
	public:
		EPOCHCOMPILER VM::EpochTypeID GetEpochType(const Program& program) const;

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);

	// Type inference
	public:
		bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);

	// Atom access and manipulation
	public:
		void AddAtom(ExpressionAtom* atom);

		const std::vector<ExpressionAtom*>& GetAtoms() const
		{ return Atoms; }

		std::vector<ExpressionAtom*>& GetAtoms()
		{ return Atoms; }

	// Internal helpers
	private:
		void Coalesce(Program& program, CodeBlock& activescope);

	// Internal state
	private:
		std::vector<ExpressionAtom*> Atoms;
		VM::EpochTypeID InferredType;
		bool Coalesced;
	};


	class Parenthetical
	{
	// Destruction
	public:
		virtual ~Parenthetical()
		{ }

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const = 0;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) const = 0;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr) = 0;
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

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);

	// State access
	public:
		const PreOpStatement* GetStatement() const
		{ return MyStatement; }

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

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) const;

	// State access
	public:
		const PostOpStatement* GetStatement() const
		{ return MyStatement; }

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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// State access
		const Parenthetical* GetParenthetical() const
		{ return MyParenthetical; }

	// Internal state
	private:
		Parenthetical* MyParenthetical;
	};

	class ExpressionAtomIdentifier : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomIdentifier(StringHandle identifier)
			: Identifier(identifier),
			  MyType(VM::EpochType_Error)
		{ }

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// Internal state
	private:
		StringHandle Identifier;
		VM::EpochTypeID MyType;
	};

	class ExpressionAtomIdentifierReference : public ExpressionAtomIdentifier
	{
	// Construction
	public:
		explicit ExpressionAtomIdentifierReference(StringHandle identifier)
			: ExpressionAtomIdentifier(identifier)
		{ }
	};

	class ExpressionAtomOperator : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomOperator(StringHandle identifier, bool ismemberaccess)
			: Identifier(identifier),
			  IsMemberAccessFlag(ismemberaccess)
		{ }

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

		void SetIdentifier(StringHandle identifier)
		{ Identifier = identifier; }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// Additional type inference support
	public:
		bool IsOperatorUnary(const Program& program) const;
		VM::EpochTypeID DetermineOperatorReturnType(Program& program, VM::EpochTypeID lhstype, VM::EpochTypeID rhstype) const;
		VM::EpochTypeID DetermineUnaryReturnType(Program& program, VM::EpochTypeID operandtype) const;

		bool IsMemberAccess() const
		{ return IsMemberAccessFlag; }

	// Internal state
	private:
		StringHandle Identifier;
		bool IsMemberAccessFlag;
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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

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

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom type for compile time parameter"); }

	// Accessors
	public:
		Statement& GetStatement() const
		{ return *MyStatement; }

	// Internal state
	private:
		Statement* MyStatement;
	};


	class ExpressionAtomCopyFromStructure : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomCopyFromStructure(VM::EpochTypeID type, StringHandle membername)
			: MyType(type),
			  MemberName(membername)
		{ }

	// Non-copyable
	private:
		ExpressionAtomCopyFromStructure(const ExpressionAtomCopyFromStructure& other);
		ExpressionAtomCopyFromStructure& operator = (const ExpressionAtomCopyFromStructure& rhs);

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program&) const
		{ return MyType; }

		virtual bool TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Program&, CodeBlock&, bool)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom type for compile time parameter"); }

	// Inspection
	public:
		StringHandle GetMemberName() const
		{ return MemberName; }

	// Internal state
	private:
		VM::EpochTypeID MyType;
		StringHandle MemberName;
	};


	class ExpressionAtomBindReference : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomBindReference(StringHandle identifier, VM::EpochTypeID membertype)
			: Identifier(identifier),
			  MyType(membertype)
		{ }

	// Non-copyable
	private:
		ExpressionAtomBindReference(const ExpressionAtomBindReference& other);
		ExpressionAtomBindReference& operator = (const ExpressionAtomBindReference& rhs);

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program&) const
		{ return MyType; }

		virtual bool TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Program&, CodeBlock&, bool)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom type for compile time parameter"); }

	// Inspection
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

	// Internal state
	private:
		StringHandle Identifier;
		VM::EpochTypeID MyType;
	};

}

