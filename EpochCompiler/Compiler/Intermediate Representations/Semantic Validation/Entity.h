//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for describing entities
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/CompileTimeParams.h"

#include <vector>
#include <utility>


// Forward declarations
class CompileErrors;
class ScopeDescription;


namespace IRSemantics
{

	// Forward declarations
	class CodeBlock;
	class Expression;
	class Namespace;
	struct InferenceContext;


	//
	// Class for representing an entity invocation in the IR
	//
	// This single class is responsible for both prefix and postfix form
	// entity invocations, as well as entity chains.
	//
	class Entity
	{
	// Construction and destruction
	public:
		Entity(StringHandle name, const AST::IdentifierT& originalidentifier);
		~Entity();

		Entity* Clone() const;

	// Non-copyable
	private:
		Entity(const Entity& other);
		Entity& operator = (const Entity& rhs);

	// Parameters
	public:
		void AddParameter(Expression* expression);

		const std::vector<Expression*>& GetParameters() const
		{ return Parameters; }

	// Code attachment
	public:
		void SetCode(CodeBlock* code);

		CodeBlock& GetCode() const
		{ return *Code; }

	// Chaining
	public:
		void AddChain(Entity* chained);

		const std::vector<Entity*>& GetChain() const
		{ return Chain; }

	// Additional accessors
	public:
		StringHandle GetName() const
		{ return Name; }

	// Postfix entity handling
	public:
		StringHandle GetPostfixIdentifier() const
		{ return PostfixName; }

		void SetPostfixIdentifier(StringHandle name)
		{ PostfixName = name; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Optimization
	public:
		void HoistScopes(ScopeDescription* target);

	// Template support
	public:
		void SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace);

	// Internal state
	private:
		StringHandle Name;
		StringHandle PostfixName;
		CodeBlock* Code;
		std::vector<Expression*> Parameters;
		std::vector<Entity*> Chain;

		const AST::IdentifierT& OriginalIdentifier;
	};

}

