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
	class Namespace;
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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const = 0;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors) = 0;
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors) = 0;
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace& curnamespace) const = 0;
		virtual ExpressionAtom* Clone() const = 0;

		virtual bool Demote(Metadata::EpochTypeID targettype, const Namespace& curnamespace)
		{ return (targettype == GetEpochType(curnamespace)); }

		virtual bool MakeReference(size_t, std::vector<ExpressionAtom*>&)
		{ return false; }

		virtual bool MakeAnnotatedReference(size_t, std::vector<ExpressionAtom*>&)
		{ return false; }
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

		Expression* Clone() const;

	// Type system
	public:
		EPOCHCOMPILER Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;

	// Reference binding helpers
	public:
		bool MakeReference();
		bool MakeAnnotatedReference();

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		bool IsInferenceDone() const
		{ return InferenceDone; }

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
		void Coalesce(Namespace& curnamespace, CodeBlock& activescope, CompileErrors& errors);

	// Internal state
	private:
		std::vector<ExpressionAtom*> Atoms;
		Metadata::EpochTypeID InferredType;
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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const = 0;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const = 0;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors) = 0;

	// Deep copies
	public:
		virtual Parenthetical* Clone() const = 0;
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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Deep copies
	public:
		Parenthetical* Clone() const;

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
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type system
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// Deep copies
	public:
		Parenthetical* Clone() const;

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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Deep copies
	public:
		Parenthetical* Clone() const;

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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

		virtual ExpressionAtom* Clone() const;

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
	class ExpressionAtomIdentifierBase : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomIdentifierBase(StringHandle identifier, const AST::IdentifierT& originalidentifier)
			: Identifier(identifier),
			  MyType(Metadata::EpochType_Error),
			  OriginalIdentifier(originalidentifier)
		{ }

	// Non-assignable
	private:
		ExpressionAtomIdentifierBase& operator = (const ExpressionAtomIdentifierBase& rhs);

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

		const AST::IdentifierT& GetOriginalIdentifier() const
		{ return OriginalIdentifier; }

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
		{
			CompileTimeParameter ret(L"@@identifier", Metadata::EpochType_Identifier);
			ret.Payload.LiteralStringHandleValue = Identifier;

			return ret;
		}

		virtual ExpressionAtom* Clone() const;

	// Internal state
	protected:
		StringHandle Identifier;
		Metadata::EpochTypeID MyType;
		const AST::IdentifierT& OriginalIdentifier;
	};

	//
	// Expression atom which is a plain identifier
	//
	class ExpressionAtomIdentifier : public ExpressionAtomIdentifierBase
	{
	// Construction
	public:
		ExpressionAtomIdentifier(StringHandle identifier, const AST::IdentifierT& originalidentifier)
			: ExpressionAtomIdentifierBase(identifier, originalidentifier)
		{ }

		virtual ExpressionAtom* Clone() const;

		virtual bool MakeReference(size_t index, std::vector<ExpressionAtom*>& atoms);
		virtual bool MakeAnnotatedReference(size_t index, std::vector<ExpressionAtom*>& atoms);
	};


	//
	// Special marker class used for signaling that an
	// identifier should be handled as a reference.
	//
	class ExpressionAtomIdentifierReference : public ExpressionAtomIdentifierBase
	{
	// Construction
	public:
		ExpressionAtomIdentifierReference(StringHandle identifier, const AST::IdentifierT& originalidentifier)
			: ExpressionAtomIdentifierBase(identifier, originalidentifier)
		{ }

		virtual ExpressionAtom* Clone() const;

		virtual bool MakeReference(size_t index, std::vector<ExpressionAtom*>& atoms);
		virtual bool MakeAnnotatedReference(size_t index, std::vector<ExpressionAtom*>& atoms);
	};


	//
	// Expression atom wrapper for operators
	//
	class ExpressionAtomOperator : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomOperator(StringHandle identifier, bool ismemberaccess, StringHandle secondaryidentifier)
			: Identifier(identifier),
			  OriginalIdentifier(identifier),
			  IsMemberAccessFlag(ismemberaccess),
			  OverriddenType(Metadata::EpochType_Error),
			  SecondaryIdentifier(secondaryidentifier)
		{ }

	// Accessors
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }

		void SetIdentifier(StringHandle identifier)
		{ Identifier = identifier; }

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
		{ throw std::runtime_error("Invalid atom for compile time param"); }

		virtual ExpressionAtom* Clone() const;

		virtual bool MakeReference(size_t index, std::vector<ExpressionAtom*>& atoms);
		virtual bool MakeAnnotatedReference(size_t index, std::vector<ExpressionAtom*>& atoms);

	// Additional type inference support
	public:
		bool IsOperatorUnary(const Namespace& curnamespace) const;
		Metadata::EpochTypeID DetermineOperatorReturnType(Namespace& curnamespace, Metadata::EpochTypeID lhstype, Metadata::EpochTypeID rhstype, CompileErrors& errors) const;
		Metadata::EpochTypeID DetermineUnaryReturnType(Namespace& curnamespace, Metadata::EpochTypeID operandtype, CompileErrors& errors) const;

		bool IsMemberAccess() const
		{ return IsMemberAccessFlag; }

		void OverrideType(Metadata::EpochTypeID overridetype)
		{
			OverriddenType = overridetype;
		}

	// Precedence
	public:
		int GetOperatorPrecedence(const Namespace& curnamespace) const;

	// Internal state
	private:
		StringHandle Identifier;
		StringHandle OriginalIdentifier;
		StringHandle SecondaryIdentifier;
		bool IsMemberAccessFlag;
		Metadata::EpochTypeID OverriddenType;
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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace& curnamespace) const;
		virtual ExpressionAtom* Clone() const;

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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace& curnamespace) const;
		
		virtual ExpressionAtom* Clone() const
		{ return new ExpressionAtomLiteralBoolean(Value); }

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
			  MyType(Metadata::EpochType_Integer)
		{ }

	// Accessors
	public:
		Integer32 GetValue() const
		{ return Value; }

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace& curnamespace) const;
		virtual ExpressionAtom* Clone() const;
		virtual bool Demote(Metadata::EpochTypeID targettype, const Namespace& curnamespace);

	// Internal state
	private:
		Integer32 Value;
		Metadata::EpochTypeID MyType;
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
			  MyType(Metadata::EpochType_Real)
		{ }

	// Accessors
	public:
		Real32 GetValue() const
		{ return Value; }

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace& curnamespace) const;
		virtual ExpressionAtom* Clone() const;

	// Internal state
	private:
		Real32 Value;
		Metadata::EpochTypeID MyType;
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
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors);
		virtual bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
		{ throw std::runtime_error("Invalid atom type for compile time parameter"); }
		
		virtual ExpressionAtom* Clone() const;

		virtual bool MakeReference(size_t index, std::vector<ExpressionAtom*>& atoms);
		virtual bool MakeAnnotatedReference(size_t index, std::vector<ExpressionAtom*>& atoms);

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
		ExpressionAtomCopyFromStructure(Metadata::EpochTypeID type, StringHandle membername)
			: MyType(type),
			  MemberName(membername)
		{ }

	// Non-copyable
	private:
		ExpressionAtomCopyFromStructure(const ExpressionAtomCopyFromStructure& other);
		ExpressionAtomCopyFromStructure& operator = (const ExpressionAtomCopyFromStructure& rhs);

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

		virtual bool TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
		{ throw std::runtime_error("Invalid atom type for compile time parameter"); }

		virtual ExpressionAtom* Clone() const;

	// Inspection
	public:
		StringHandle GetMemberName() const
		{ return MemberName; }

	// Internal state
	private:
		Metadata::EpochTypeID MyType;
		StringHandle MemberName;
	};


	//
	// Special marker class for binding a reference to a structure or member
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
		ExpressionAtomBindReference(StringHandle membername, Metadata::EpochTypeID membertype, bool isreference)
			: Identifier(membername),
			  MyType(membertype),
			  IsRef(isreference)
		{ }

	// Non-copyable
	private:
		ExpressionAtomBindReference(const ExpressionAtomBindReference& other);
		ExpressionAtomBindReference& operator = (const ExpressionAtomBindReference& rhs);

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

		virtual bool TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
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

		virtual ExpressionAtom* Clone() const;

		virtual bool MakeReference(size_t index, std::vector<ExpressionAtom*>& atoms);
		virtual bool MakeAnnotatedReference(size_t index, std::vector<ExpressionAtom*>& atoms);

	// Inspection
	public:
		StringHandle GetIdentifier() const
		{ return Identifier; }
		
		bool IsReference() const
		{ return IsRef; }

	// Internal state
	private:
		StringHandle Identifier;
		Metadata::EpochTypeID MyType;
		bool IsRef;
	};


	//
	// Special atom helper used to emit type annotations
	// onto the stack during execution; helps the VM perform
	// type-based dispatching in various ways.
	//
	class ExpressionAtomTypeAnnotation : public ExpressionAtom
	{
	// Construction
	public:
		explicit ExpressionAtomTypeAnnotation(Metadata::EpochTypeID type)
			: MyType(type)
		{ }

	// Non-copyable
	private:
		ExpressionAtomTypeAnnotation(const ExpressionAtomTypeAnnotation& other);
		ExpressionAtomTypeAnnotation& operator = (const ExpressionAtomTypeAnnotation& rhs);

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

		virtual bool TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
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

		virtual ExpressionAtom* Clone() const;

	// Internal state
	private:
		Metadata::EpochTypeID MyType;
	};

	//
	// Special helper atom used to write a type annotation
	// from the return value register onto the stack. Used
	// when type dispatch is needed on a return value from
	// a function which is not first stored in a variable.
	//
	class ExpressionAtomTempReferenceFromRegister : public ExpressionAtom
	{
	// Construction
	public:
		ExpressionAtomTempReferenceFromRegister()
		{ }

	// Non-copyable
	private:
		ExpressionAtomTempReferenceFromRegister(const ExpressionAtomTempReferenceFromRegister& other);
		ExpressionAtomTempReferenceFromRegister& operator = (const ExpressionAtomTempReferenceFromRegister& rhs);

	// Atom interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return Metadata::EpochType_Error; }

		virtual bool TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
		{ return true; }

		virtual bool CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
		{ return true; }
		
		virtual CompileTimeParameter ConvertToCompileTimeParam(const Namespace&) const
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

		virtual ExpressionAtom* Clone() const
		{ return new ExpressionAtomTempReferenceFromRegister; }
	};

}

