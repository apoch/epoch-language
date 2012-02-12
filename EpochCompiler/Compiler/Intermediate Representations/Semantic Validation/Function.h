//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>



namespace IRSemantics
{

	// Forward declarations
	class Program;
	class CodeBlock;
	class Expression;
	struct InferenceContext;


	//
	// Abstract base for function parameters
	//
	class FunctionParam
	{
	// Destruction
	public:
		virtual ~FunctionParam()
		{ }

	// Function parameter interface
	public:
		virtual VM::EpochTypeID GetParamType(const IRSemantics::Program& program) const = 0;
		virtual bool IsLocalVariable() const = 0;
		virtual bool IsReference() const = 0;
		virtual bool Validate(const IRSemantics::Program& program) const = 0;
	};

	//
	// Named function parameter
	//
	class FunctionParamNamed : public FunctionParam
	{
	// Construction
	public:
		FunctionParamNamed(StringHandle type, bool isreference)
			: MyType(type),
			  IsRef(isreference)
		{ }

	// Function parameter interface
	public:
		virtual VM::EpochTypeID GetParamType(const IRSemantics::Program& program) const;

		virtual bool IsLocalVariable() const
		{ return true; }

		virtual bool IsReference() const
		{ return IsRef; }

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		StringHandle MyType;
		bool IsRef;
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
		virtual VM::EpochTypeID GetParamType(const IRSemantics::Program& program) const
		{ return VM::EpochType_Function; }

		virtual bool IsLocalVariable() const
		{ return true; }

		virtual bool IsReference() const
		{ return false; }

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
		virtual VM::EpochTypeID GetParamType(const IRSemantics::Program& program) const;

		virtual bool IsLocalVariable() const
		{ return false; }

		virtual bool IsReference() const
		{ return false; }

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
			  Return(NULL),
			  InferenceDone(false)
		{ }

		~Function();

	// Non-copyable
	private:
		Function(const Function& other);
		Function& operator = (const Function& rhs);

	// Name
	public:
		void SetName(StringHandle name)
		{ Name = name; }

	// Parameters
	public:
		void AddParameter(StringHandle name, FunctionParam* param);

		std::vector<StringHandle> GetParameterNames() const;

		bool IsParameterLocalVariable(StringHandle name) const;
		VM::EpochTypeID GetParameterType(StringHandle name, const IRSemantics::Program& program) const;
		bool IsParameterReference(StringHandle name) const;

		size_t GetNumParameters() const
		{ return Parameters.size(); }

	// Returns
	public:
		void SetReturnExpression(Expression* expression);

		const Expression* GetReturnExpression() const
		{ return Return; }

		VM::EpochTypeID GetReturnType(const Program& program) const;

	// Inner code
	public:
		void SetCode(CodeBlock* code);

		CodeBlock* GetCode()
		{ return Code; }

		const CodeBlock* GetCode() const
		{ return Code; }

	// Validation
	public:
		bool Validate(const IRSemantics::Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(IRSemantics::Program& program);

	// Type inference
	public:
		bool TypeInference(IRSemantics::Program& program, InferenceContext& context);

	// Internal state
	private:
		StringHandle Name;

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
		bool InferenceDone;
	};

}

