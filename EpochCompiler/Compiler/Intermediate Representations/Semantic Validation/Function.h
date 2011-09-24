//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"

#include <vector>



namespace IRSemantics
{

	// Forward declarations
	class Program;
	class CodeBlock;
	class Expression;


	//
	// Abstract base for function parameters
	//
	class FunctionParam
	{
	// Constants
	public:
		enum Type
		{
			Named,
			FunctionRef,
			Expression,
		};

	// Destruction
	public:
		virtual ~FunctionParam()
		{ }

	// Function parameter interface
	public:
		virtual Type GetParamType() const = 0;
		virtual bool Validate(const IRSemantics::Program& program) const = 0;
	};

	//
	// Named function parameter
	//
	class FunctionParamNamed : public FunctionParam
	{
	// Construction
	public:
		explicit FunctionParamNamed(StringHandle type)
			: MyType(type)
		{ }

	// Function parameter interface
	public:
		virtual Type GetParamType() const
		{ return FunctionParam::Named; }

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		StringHandle MyType;
	};

	//
	// Higher order function parameter
	//
	class FunctionParamFuncRef : public FunctionParam
	{
	// Construction
	public:
		FunctionParamFuncRef(const std::vector<StringHandle>& paramtypes, StringHandle returntype)
			: ParamTypes(paramtypes),
			  ReturnType(returntype)
		{ }

	// Function parameter interface
	public:
		virtual Type GetParamType() const
		{ return FunctionParam::FunctionRef; }

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		std::vector<StringHandle> ParamTypes;
		StringHandle ReturnType;
	};

	//
	// Expression function parameter
	//
	class FunctionParamExpression : public FunctionParam
	{
	// Construction and destruction
	public:
		explicit FunctionParamExpression(IRSemantics::Expression* expression)
			: MyExpression(expression)
		{ }

		virtual ~FunctionParamExpression();

	// Non-copyable
	private:
		FunctionParamExpression(const FunctionParamExpression& other);
		FunctionParamExpression& operator = (const FunctionParamExpression& rhs);

	// Function parameter interface
	public:
		virtual Type GetParamType() const
		{ return FunctionParam::Expression; }

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		IRSemantics::Expression* MyExpression;
	};



	//
	// Wrapper for functions
	//
	class Function
	{
	// Construction and destruction
	public:
		Function()
			: Code(NULL),
			  Return(NULL)
		{ }

		~Function();

	// Non-copyable
	private:
		Function(const Function& other);
		Function& operator = (const Function& rhs);

	// Parameters
	public:
		void AddParameter(StringHandle name, FunctionParam* param);

	// Returns
	public:
		void SetReturnExpression(Expression* expression);

	// Inner code
	public:
		void SetCode(CodeBlock* code);

		const CodeBlock* GetCode() const
		{ return Code; }

	// Validation
	public:
		bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		CodeBlock* Code;
		Expression* Return;

		struct Param
		{
			StringHandle Name;
			FunctionParam* Parameter;

			Param(StringHandle name, FunctionParam* param)
				: Name(name),
				  Parameter(param)
			{ }
		};

		std::vector<Param> Parameters;
	};

}

