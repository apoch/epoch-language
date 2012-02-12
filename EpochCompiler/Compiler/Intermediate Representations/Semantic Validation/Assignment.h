//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>


namespace IRSemantics
{

	// Forward declarations
	class Expression;
	class CodeBlock;
	class Program;
	struct InferenceContext;


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

		virtual void SetRHSRecursive(AssignmentChain* rhs)
		{ }

		virtual VM::EpochTypeID GetEpochType(const Program& program) const = 0;

		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) = 0;

		virtual bool Validate(const Program& program) const = 0;
	};

	class Assignment
	{
	// Construction and destruction
	public:
		Assignment(const std::vector<StringHandle>& lhs, StringHandle operatorname);
		~Assignment();

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

		const AssignmentChain* GetRHS() const
		{ return RHS; }

		AssignmentChain* GetRHS()
		{ return RHS; }

		StringHandle GetOperatorName() const
		{ return OperatorName; }

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Type inference
	public:
		bool TypeInference(IRSemantics::Program& program, CodeBlock& activescope, InferenceContext& context);

	// Internal state
	private:
		std::vector<StringHandle> LHS;
		StringHandle OperatorName;
		AssignmentChain* RHS;
		VM::EpochTypeID LHSType;
	};


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
		virtual VM::EpochTypeID GetEpochType(const Program& program) const;
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context);
		virtual bool Validate(const Program& program) const;

	// Internal state
	private:
		Expression* MyExpression;
	};

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

		virtual VM::EpochTypeID GetEpochType(const Program& program) const;

		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context);
		
		virtual bool Validate(const Program& program) const;

	// Access to assignment
	public:
		const Assignment& GetAssignment() const
		{ return *MyAssignment; }

	// Internal state
	private:
		Assignment* MyAssignment;
	};

}

