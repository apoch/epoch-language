//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class for representing statements
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include <vector>


class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Expression;
	class CodeBlock;
	class Program;
	struct InferenceContext;


	class Statement
	{
	// Construction and destruction
	public:
		Statement(StringHandle name, const AST::IdentifierT& identifier);
		~Statement();

	// Non-assignable
	private:
		Statement& operator= (const Statement& rhs);

	// Parameter control
	public:
		void AddParameter(Expression* expression);

	// Detail access
	public:
		StringHandle GetName() const
		{ return Name; }

		const std::vector<Expression*>& GetParameters() const
		{ return Parameters; }

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(IRSemantics::Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors);

	// Type system
	public:
		VM::EpochTypeID GetEpochType(const Program&) const
		{ return MyType; }

	// Internal state
	private:
		StringHandle Name;
		std::vector<Expression*> Parameters;
		VM::EpochTypeID MyType;
		const AST::IdentifierT& OriginalIdentifier;
	};


	class PreOpStatement
	{
	// Construction
	public:
		PreOpStatement(StringHandle operatorname, const std::vector<StringHandle> operand)
			: OperatorName(operatorname),
			  Operand(operand),
			  MyType(VM::EpochType_Error)
		{ }

	// Property access
	public:
		StringHandle GetOperatorName() const
		{ return OperatorName; }

		const std::vector<StringHandle>& GetOperand() const
		{ return Operand; }

	// Type inference
	public:
		bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Type system
	public:
		VM::EpochTypeID GetEpochType(const Program&) const
		{ return MyType; }

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Internal state
	private:
		StringHandle OperatorName;
		std::vector<StringHandle> Operand;
		VM::EpochTypeID MyType;
	};

	class PostOpStatement
	{
	// Construction
	public:
		PostOpStatement(const std::vector<StringHandle> operand, StringHandle operatorname)
			: Operand(operand),
			  OperatorName(operatorname),
			  MyType(VM::EpochType_Error)
		{ }

	// Property access
	public:
		StringHandle GetOperatorName() const
		{ return OperatorName; }

		const std::vector<StringHandle>& GetOperand() const
		{ return Operand; }

	// Type inference
	public:
		bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Type system
	public:
		VM::EpochTypeID GetEpochType(const Program&) const
		{ return MyType; }

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Internal state
	private:
		std::vector<StringHandle> Operand;
		StringHandle OperatorName;
		VM::EpochTypeID MyType;
	};

}

