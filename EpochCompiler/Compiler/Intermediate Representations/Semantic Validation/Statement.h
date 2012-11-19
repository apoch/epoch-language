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

#include "Metadata/CompileTimeParams.h"

#include <vector>


class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Expression;
	class CodeBlock;
	class Namespace;
	struct InferenceContext;


	//
	// IR node representing a statement (function or constructor invocation)
	//
	class Statement
	{
	// Construction and destruction
	public:
		Statement(StringHandle name, const AST::IdentifierT& identifier);
		~Statement();

		Statement* Clone() const;

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

		StringHandle GetRawName() const
		{ return RawName; }

		const std::vector<Expression*>& GetParameters() const
		{ return Parameters; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors);

	// Type system
	public:
		Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

	// Template support
	public:
		void SetTemplateArgs(const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors);
		void SetTemplateArgsDeferred(const CompileTimeParameterVector& args);

	// Internal state
	private:
		StringHandle Name;
		StringHandle RawName;
		std::vector<Expression*> Parameters;
		Metadata::EpochTypeID MyType;
		const AST::IdentifierT& OriginalIdentifier;
		bool CompileTimeCodeExecuted;
		CompileTimeParameterVector TemplateArgs;
		bool NeedsInstantiation;
	};

	//
	// IR node representing a pre-operation statement (such as ++i)
	//
	class PreOpStatement
	{
	// Construction
	public:
		PreOpStatement(StringHandle operatorname, const std::vector<StringHandle>& operand, const AST::IdentifierT& originaloperand)
			: OperatorName(operatorname),
			  Operand(operand),
			  OriginalOperand(originaloperand),
			  MyType(Metadata::EpochType_Error)
		{ }

		PreOpStatement* Clone() const;

	// Non-copyable
	private:
		PreOpStatement(const PreOpStatement&);
		PreOpStatement& operator = (const PreOpStatement&);

	// Property access
	public:
		StringHandle GetOperatorName() const
		{ return OperatorName; }

		const std::vector<StringHandle>& GetOperand() const
		{ return Operand; }

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Type system
	public:
		Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Internal state
	private:
		StringHandle OperatorName;
		std::vector<StringHandle> Operand;
		Metadata::EpochTypeID MyType;
		const AST::IdentifierT& OriginalOperand;
	};

	//
	// IR node representing a post-operation statement (such as i++)
	//
	class PostOpStatement
	{
	// Construction
	public:
		PostOpStatement(const std::vector<StringHandle>& operand, StringHandle operatorname, const AST::IdentifierT& originaloperand)
			: Operand(operand),
			  OperatorName(operatorname),
			  OriginalOperand(originaloperand),
			  MyType(Metadata::EpochType_Error)
		{ }

		PostOpStatement* Clone() const;

	// Non-copyable
	private:
		PostOpStatement(const PostOpStatement&);
		PostOpStatement& operator = (const PostOpStatement&);

	// Property access
	public:
		StringHandle GetOperatorName() const
		{ return OperatorName; }

		const std::vector<StringHandle>& GetOperand() const
		{ return Operand; }

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Type system
	public:
		Metadata::EpochTypeID GetEpochType(const Namespace&) const
		{ return MyType; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Internal state
	private:
		std::vector<StringHandle> Operand;
		StringHandle OperatorName;
		Metadata::EpochTypeID MyType;
		const AST::IdentifierT& OriginalOperand;
	};

}

