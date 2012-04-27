//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing expressions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include "Compiler/ExportDef.h"
#include "Compiler/Exceptions.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/CompileTimeParams.h"

#include <vector>


// Forward declarations
class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Statement;
	class PreOpStatement;
	class PostOpStatement;
	class CodeBlock;
	class Program;
	struct InferenceContext;


	//
	// Base interface for all expression atoms
	//
	// Expressions are divided into atomic units in order to
	// make manipulation, type inference, etc. simpler. They
	// are converted from the component/fragment form of the
	// AST into atomic form during the semantic IR pass, and
	// manipulated as such during semantic analysis.
	//
	class ExpressionAtom
	{
	// Destruction
	public:
		virtual ~ExpressionAtom()
		{ }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const = 0;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors) = 0;
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors) = 0;
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const = 0;
	};


	//
	// IR node class for wrapping a complete expression
	//
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
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);

	// Atom access and manipulation
	public:
		void AddAtom(ExpressionAtom* atom);

		const std::vector<ExpressionAtom*>& GetAtoms() const
		{ return Atoms; }

		std::vector<ExpressionAtom*>& GetAtoms()
		{ return Atoms; }

	// Pattern matching helpers
	public:
		bool AtomsArePatternMatchedLiteral;

	// Internal helpers
	private:
		void Coalesce(Program& program, CodeBlock& activescope, CompileErrors& errors);

	// Internal state
	private:
		std::vector<ExpressionAtom*> Atoms;
		VM::EpochTypeID InferredType;
		bool Coalesced;
		bool InferenceDone;
		bool DoingInference;
		bool InferenceRecursed;
	};


	//
	// Helper interface for representing parentheticals
	//
	// A parenthetical can be a complete expression, or a
	// pre-operation statement, or a post-operation statement.
	// These variants require different handling and are thus
	// split among derived classes from this interface.
	//
	class Parenthetical
	{
	// Destruction
	public:
		virtual ~Parenthetical()
		{ }

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const = 0;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const = 0;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors) = 0;
	};


	//
	// Parenthetical wrapper for pre-operator statements
	//
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// State access
	public:
		const PreOpStatement* GetStatement() const
		{ return MyStatement; }

	// Internal state
	private:
		PreOpStatement* MyStatement;
	};


	//
	// Parenthetical wrapper for post-operator statements
	//
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
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// State access
	public:
		const PostOpStatement* GetStatement() const
		{ return MyStatement; }

	// Internal state
	private:
		PostOpStatement* MyStatement;
	};


	//
	// Parenthetical wrapper for arbitrary expressions
	//
	class ParentheticalExpression : public Parenthetical
	{
	// Construction and destruction
	public:
		explicit ParentheticalExpression(Expression* expression);
		virtual ~ParentheticalExpression();

	// Non-copyable
	private:
		ParentheticalExpression(const ParentheticalExpression& other);
		ParentheticalExpression& operator = (const ParentheticalExpression& rhs);

	// Type system
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// State access
	public:
		const Expression& GetExpression() const
		{ return *MyExpression; }

	// Internal state
	private:
		Expression* MyExpression;
	};


	//
	// Expression atom wrapping a parenthetical expression/statement
	//
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// State access
		const Parenthetical* GetParenthetical() const
		{ return MyParenthetical; }

	// Internal state
	private:
		Parenthetical* MyParenthetical;
	};


	//
	// Expression atom wrapping a single identifier
	//
	class ExpressionAtomIdentifier : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomIdentifier(StringHandle identifier, const AST::IdentifierT& originalidentifier)
			: Identifier(identifier),
			  MyType(VM::EpochType_Error),
			  OriginalIdentifier(originalidentifier)
		{ }

	// Non-assignable
	private:
		ExpressionAtomIdentifier& operator = (const ExpressionAtomIdentifier& rhs);

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

		const AST::IdentifierT& GetOriginalIdentifier() const
		{ return OriginalIdentifier; }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// Internal state
	private:
		StringHandle Identifier;
		VM::EpochTypeID MyType;
		const AST::IdentifierT& OriginalIdentifier;
	};


	//
	// Special marker class used for signaling that an
	// identifier should be handled as a reference.
	//
	class ExpressionAtomIdentifierReference : public ExpressionAtomIdentifier
	{
	// Construction
	public:
		ExpressionAtomIdentifierReference(StringHandle identifier, const AST::IdentifierT& originalidentifier)
			: ExpressionAtomIdentifier(identifier, originalidentifier)
		{ }
	};


	//
	// Expression atom wrapper for operators
	//
	class ExpressionAtomOperator : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomOperator(StringHandle identifier, bool ismemberaccess)
			: Identifier(identifier),
			  OriginalIdentifier(identifier),
			  IsMemberAccessFlag(ismemberaccess),
			  OverriddenType(VM::EpochType_Error)
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

	// Additional type inference support
	public:
		bool IsOperatorUnary(const Program& program) const;
		VM::EpochTypeID DetermineOperatorReturnType(Program& program, VM::EpochTypeID lhstype, VM::EpochTypeID rhstype, CompileErrors& errors) const;
		VM::EpochTypeID DetermineUnaryReturnType(Program& program, VM::EpochTypeID operandtype, CompileErrors& errors) const;

		bool IsMemberAccess() const
		{ return IsMemberAccessFlag; }

		void OverrideType(VM::EpochTypeID overridetype)
		{
			OverriddenType = overridetype;
		}

	// Precedence
	public:
		int GetOperatorPrecedence(const Program& program) const;

	// Internal state
	private:
		StringHandle Identifier;
		StringHandle OriginalIdentifier;
		bool IsMemberAccessFlag;
		VM::EpochTypeID OverriddenType;
	};


	//
	// Expression atom wrapper for literal strings
	//
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

	// Internal state
	private:
		StringHandle Handle;
	};


	//
	// Expression atom wrapper for literal booleans
	//
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

	// Internal state
	private:
		bool Value;
	};


	//
	// Expression atom wrapper for literal integers
	//
	class ExpressionAtomLiteralInteger32 : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralInteger32(Integer32 value)
			: Value(value),
			  MyType(VM::EpochType_Integer)
		{ }

	// Accessors
	public:
		Integer32 GetValue() const
		{ return Value; }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

	// Internal state
	private:
		Integer32 Value;
		VM::EpochTypeID MyType;
	};


	//
	// Expression atom wrapper for literal reals
	//
	class ExpressionAtomLiteralReal32 : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomLiteralReal32(Real32 value)
			: Value(value),
			  MyType(VM::EpochType_Real)
		{ }

	// Accessors
	public:
		Real32 GetValue() const
		{ return Value; }

	// Atom interface
	public:
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program& program) const;

	// Internal state
	private:
		Real32 Value;
		VM::EpochTypeID MyType;
	};


	//
	// Expression atom wrapper for embedded statements/function invocations
	//
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
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
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


	//
	// Special marker atom for indicating that data
	// should be copied out of a structure member.
	//
	// This is not generated directly by the IR pass,
	// but rather injected during code generation for
	// certain automatically-created functions.
	//
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

		virtual bool TypeInference(Program&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
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


	//
	// Special marker class for binding a reference to an identifier
	//
	// This is injected automatically by various passes and is not
	// directly converted from the AST. Instead, it arises from
	// semantic checks and manipulations on the IR. It exists primarily
	// as an aid to code generation.
	//
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

		virtual bool TypeInference(Program&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Program&) const
		{
			//
			// This type of atom is a special marker and not a value, therefore
			// it should never be converted to a compile-time parameter.
			//
			// Check the call stack and verify that the expression being converted
			// to compile-time parameters is sane, and that this atom is actually
			// legitimately placed.
			//
			throw InternalException("Invalid atom type for compile time parameter");
		}

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

