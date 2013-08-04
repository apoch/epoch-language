//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

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
	// Base class for representing the RHS of an assignment
	//
	// This can be terminal (i.e. the RHS is an expression) or
	// non-terminal (i.e. the RHS is another assignment).
	//
	class AssignmentChain
	{
	// Destruction
	public:
		virtual ~AssignmentChain()
		{ }

	// Chain interface
	public:
		virtual bool CanChainToAssignment() const
		{ return false; }

		virtual void SetRHSRecursive(AssignmentChain*)
		{ }

		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const = 0;

		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) = 0;

		virtual bool Validate(const Namespace& curnamespace) const = 0;

		virtual AssignmentChain* Clone() const = 0;
	};


	//
	// IR node class for representing assignments
	//
	class Assignment
	{
	// Construction and destruction
	public:
		Assignment(const std::vector<StringHandle>& lhs, StringHandle operatorname, const AST::IdentifierT& originallhs);
		~Assignment();

		Assignment* Clone() const;

	// Non-copyable
	private:
		Assignment(const Assignment& other);
		Assignment& operator = (const Assignment& rhs);

	// Control of right-hand-side of assignment
	public:
		void SetRHSRecursive(AssignmentChain* rhs);

	// Access to properties
	public:
		const std::vector<StringHandle>& GetLHS() const
		{ return LHS; }

		const Metadata::EpochTypeID GetLHSType() const
		{ return LHSType; }

		const AssignmentChain* GetRHS() const
		{ return RHS; }

		AssignmentChain* GetRHS()
		{ return RHS; }

		StringHandle GetOperatorName() const
		{ return OperatorName; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Accessible state
	public:
		bool WantsTypeAnnotation;
		bool HasAdditionalEffects;

	// Internal state
	private:
		std::vector<StringHandle> LHS;
		StringHandle OperatorName;
		AssignmentChain* RHS;
		Metadata::EpochTypeID LHSType;
		const AST::IdentifierT& OriginalLHS;
	};


	//
	// Special form of assignment RHS which is terminal
	//
	class AssignmentChainExpression : public AssignmentChain
	{
	// Construction and destruction
	public:
		explicit AssignmentChainExpression(Expression* expression);
		virtual ~AssignmentChainExpression();

	// Non-copyable
	private:
		AssignmentChainExpression(const AssignmentChainExpression& other);
		AssignmentChainExpression& operator = (const AssignmentChainExpression& rhs);

	// Accessors
	public:
		const Expression& GetExpression() const
		{ return *MyExpression; }

	// Chain interface
	public:
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);
		virtual bool Validate(const Namespace& curnamespace) const;
		virtual AssignmentChain* Clone() const;

	// Internal state
	private:
		Expression* MyExpression;
	};


	//
	// Special form of assignment RHS which is non-terminal
	//
	class AssignmentChainAssignment : public AssignmentChain
	{
	// Construction and destruction
	public:
		explicit AssignmentChainAssignment(Assignment* assignment);
		virtual ~AssignmentChainAssignment();

	// Non-copyable
	private:
		AssignmentChainAssignment(const AssignmentChainAssignment& other);
		AssignmentChainAssignment& operator = (const AssignmentChainAssignment& rhs);

	// Chain interface
	public:
		virtual bool CanChainToAssignment() const
		{ return true; }

		virtual void SetRHSRecursive(AssignmentChain* rhs);

		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;

		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);
		
		virtual bool Validate(const Namespace& curnamespace) const;

		virtual AssignmentChain* Clone() const;

	// Access to assignment
	public:
		const Assignment& GetAssignment() const
		{ return *MyAssignment; }

	// Internal state
	private:
		Assignment* MyAssignment;
	};

}

