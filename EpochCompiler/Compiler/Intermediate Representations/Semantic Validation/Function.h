//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/CompileTimeParams.h"

#include <vector>


// Forward declarations
class FunctionSignature;
class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Namespace;
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
		virtual Metadata::EpochTypeID GetParamType(const Namespace& curnamespace) const = 0;
		virtual bool IsLocalVariable() const = 0;
		virtual bool IsReference() const = 0;
		virtual bool Validate(const Namespace& curnamespace) const = 0;
		virtual void AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const = 0;
		virtual bool TypeInference(Namespace& curnamespace, CompileErrors& errors) = 0;
		virtual bool PatternMatchValue(const CompileTimeParameter& param, const Namespace& curnamespace) const = 0;
		virtual void AddToScope(StringHandle name, CodeBlock& code, const Namespace& curnamespace) const = 0;
		virtual FunctionParam* Clone() const = 0;
	};

	//
	// Named function parameter
	//
	class FunctionParamNamed : public FunctionParam
	{
	// Construction
	public:
		FunctionParamNamed(StringHandle type, const CompileTimeParameterVector& templateargs, bool isreference)
			: MyTypeName(type),
			  TemplateArgs(templateargs),
			  MyActualType(Metadata::EpochType_Error),
			  IsRef(isreference)
		{ }

	// Function parameter interface
	public:
		virtual Metadata::EpochTypeID GetParamType(const Namespace& curnamespace) const;

		virtual bool IsLocalVariable() const
		{ return true; }

		virtual bool IsReference() const
		{ return IsRef; }

		virtual bool Validate(const Namespace& curnamespace) const;

		virtual void AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const;

		virtual bool TypeInference(Namespace& curnamespace, CompileErrors& errors);

		virtual bool PatternMatchValue(const CompileTimeParameter&, const Namespace&) const
		{ return false; }

		virtual void AddToScope(StringHandle name, CodeBlock& code, const Namespace& curnamespace) const;

		virtual FunctionParam* Clone() const;

	// Additional queries
	public:
		StringHandle GetTypeName() const
		{ return MyTypeName; }

	// Template support
	public:
		void SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace);

	// Internal state
	private:
		StringHandle MyTypeName;
		Metadata::EpochTypeID MyActualType;
		bool IsRef;
		CompileTimeParameterVector TemplateArgs;
	};

	//
	// Function parameter with just a known type but no given name
	//
	// Predominantly used for automatically/internally generated code
	//
	class FunctionParamTyped : public FunctionParam
	{
	// Construction
	public:
		FunctionParamTyped(Metadata::EpochTypeID type, bool isreference)
			: MyType(type),
			  IsRef(isreference)
		{ }

	// Function parameter interface
	public:
		virtual Metadata::EpochTypeID GetParamType(const Namespace&) const
		{ return MyType; }

		virtual bool IsLocalVariable() const
		{ return true; }

		virtual bool IsReference() const
		{ return IsRef; }

		virtual bool Validate(const Namespace&) const
		{ return true; }

		virtual void AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const;

		virtual bool TypeInference(Namespace&, CompileErrors&)
		{ return true; }

		virtual bool PatternMatchValue(const CompileTimeParameter&, const Namespace&) const
		{ return false; }

		virtual void AddToScope(StringHandle, CodeBlock&, const Namespace&) const
		{ /* deliberate no-op */ }

		virtual FunctionParam* Clone() const;

	// Internal state
	private:
		Metadata::EpochTypeID MyType;
		bool IsRef;
	};

	//
	// Higher order function parameter
	//
	class FunctionParamFuncRef : public FunctionParam
	{
	// Construction
	public:
		FunctionParamFuncRef()
			: ReturnType(0)
		{ }

	// Function parameter interface
	public:
		virtual Metadata::EpochTypeID GetParamType(const Namespace&) const
		{ return Metadata::EpochType_Function; }

		virtual bool IsLocalVariable() const
		{ return true; }

		virtual bool IsReference() const
		{ return false; }

		virtual bool Validate(const Namespace& curnamespace) const;

		virtual void AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const;

		virtual bool TypeInference(Namespace&, CompileErrors&)
		{ return true; }

		virtual bool PatternMatchValue(const CompileTimeParameter&, const Namespace&) const
		{ return false; }

		virtual void AddToScope(StringHandle name, CodeBlock& code, const Namespace& curnamespace) const;

		virtual FunctionParam* Clone() const;

	// Mutation
	public:
		void AddParam(StringHandle type)
		{ ParamTypes.push_back(type); }

		void SetReturnType(StringHandle type)
		{ ReturnType = type; }

	// Additional inspection
	public:
		StringHandle GetReturnType() const
		{ return ReturnType; }

		const std::vector<StringHandle>& GetParamTypes() const
		{ return ParamTypes; }

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
		virtual Metadata::EpochTypeID GetParamType(const Namespace& curnamespace) const;

		virtual bool IsLocalVariable() const
		{ return false; }

		virtual bool IsReference() const
		{ return false; }

		virtual bool Validate(const Namespace& curnamespace) const;

		virtual void AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const;

		virtual bool TypeInference(Namespace& curnamespace, CompileErrors& errors);

		virtual bool PatternMatchValue(const CompileTimeParameter& param, const Namespace& curnamespace) const;

		virtual void AddToScope(StringHandle, CodeBlock&, const Namespace&) const
		{ /* deliberate no-op */ }

		virtual FunctionParam* Clone() const;

	// Internal state
	private:
		IRSemantics::Expression* MyExpression;
	};


	//
	// Wrapper for recording function tags
	//
	struct FunctionTag
	{
		StringHandle TagName;
		CompileTimeParameterVector Parameters;
		AST::IdentifierT OriginalTag;
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
			  InferenceDone(false),
			  SuppressReturn(false),
			  Name(0),
			  RawName(0),
			  AnonymousReturn(false),
			  HintReturnType(Metadata::EpochType_Error),
			  DummyNamespace(NULL)
		{ }

		Function(const Function* templatefunc, Namespace& curnamespace, const CompileTimeParameterVector& args);

		~Function();

	// Non-copyable
	private:
		Function(const Function& other);
		Function& operator = (const Function& rhs);

	// Name
	public:
		StringHandle GetName() const
		{ return Name; }

		void SetName(StringHandle name)
		{ Name = name; }
		
		StringHandle GetRawName() const
		{ return RawName; }

		void SetRawName(StringHandle rawname)
		{ RawName = rawname; }

	// Parameters
	public:
		void AddParameter(StringHandle name, FunctionParam* param, CompileErrors& errors);

		std::vector<StringHandle> GetParameterNames() const;
		bool HasParameter(StringHandle paramname) const;

		bool IsParameterLocalVariable(StringHandle name) const;
		Metadata::EpochTypeID GetParameterType(StringHandle name, Namespace& curnamespace, CompileErrors& errors) const;
		bool IsParameterReference(StringHandle name) const;
		StringHandle GetParameterTypeName(StringHandle name) const;

		bool DoesParameterSignatureMatch(size_t index, const FunctionSignature& signature, const Namespace& curnamespace) const;
		Metadata::EpochTypeID GetParameterSignatureType(StringHandle name, const Namespace& curnamespace) const;
		FunctionSignature GetParameterSignature(StringHandle name, const Namespace& curnamespace) const;

		size_t GetNumParameters() const
		{ return Parameters.size(); }

		bool PatternMatchParameter(size_t index, const CompileTimeParameter& param, const Namespace& curnamespace) const;

		void PopulateScope(Namespace& curnamespace, CompileErrors& errors);

	// Returns
	public:
		void SetReturnExpression(Expression* expression);

		const Expression* GetReturnExpression() const
		{ return Return; }

		Metadata::EpochTypeID GetReturnType(const Namespace& curnamespace) const;

		void SuppressReturnRegister()
		{ SuppressReturn = true; }

		bool IsReturnRegisterSuppressed() const
		{ return SuppressReturn; }

		bool HasAnonymousReturn() const
		{ return AnonymousReturn; }

		void SetHintReturnType(Metadata::EpochTypeID rettype)
		{ HintReturnType = rettype; }

	// Signatures
	public:
		FunctionSignature GetFunctionSignature(const Namespace& curnamespace) const;

	// Inner code
	public:
		void SetCode(CodeBlock* code);

		CodeBlock* GetCode()
		{ return Code; }

		const CodeBlock* GetCode() const
		{ return Code; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, InferenceContext& context, CompileErrors& errors);
		void TypeInferenceParamsOnly(Namespace& curnamespace, CompileErrors& errors);

	// Tags
	public:
		void AddTag(const FunctionTag& tag);

		const std::vector<FunctionTag>& GetTags() const
		{ return Tags; }

	// Template support
	public:
		void AddTemplateParameter(Metadata::EpochTypeID type, StringHandle name);

		bool IsTemplate() const
		{ return (!TemplateParams.empty()) && (TemplateArgs.empty()); }

		void SetTemplateArguments(Namespace& curnamespace, const CompileTimeParameterVector& args);

		void FixupScope();

	// Internal state
	private:
		friend class FunctionTable;

		StringHandle Name;
		StringHandle RawName;

		CodeBlock* Code;
		Expression* Return;

		bool AnonymousReturn;

		struct Param
		{
			StringHandle Name;
			FunctionParam* Parameter;

			Param(StringHandle name, FunctionParam* param)
				: Name(name),
				  Parameter(param)
			{ }

			Param Clone() const
			{
				return Param(Name, Parameter->Clone());
			}
		};

		std::vector<Param> Parameters;
		bool InferenceDone;

		bool SuppressReturn;

		std::vector<FunctionTag> Tags;

		std::vector<std::pair<StringHandle, Metadata::EpochTypeID> > TemplateParams;
		CompileTimeParameterVector TemplateArgs;
		Namespace* DummyNamespace;

		Metadata::EpochTypeID HintReturnType;
	};

}

