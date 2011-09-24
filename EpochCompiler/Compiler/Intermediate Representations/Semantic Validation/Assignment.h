//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"

#include <vector>


namespace IRSemantics
{

	// Forward declarations
	class Expression;


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
		void SetRHS(AssignmentChain* rhs);
		void SetRHSRecursive(AssignmentChain* rhs);

	// Access to properties
	public:
		const std::vector<StringHandle>& GetLHS() const
		{ return LHS; }

		const AssignmentChain* GetRHS() const
		{ return RHS; }

		StringHandle GetOperatorName() const
		{ return OperatorName; }

	// Internal state
	private:
		std::vector<StringHandle> LHS;
		StringHandle OperatorName;
		AssignmentChain* RHS;
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

	// Access to assignment
	public:
		const Assignment& GetAssignment() const
		{ return *MyAssignment; }

	// Internal state
	private:
		Assignment* MyAssignment;
	};

}

