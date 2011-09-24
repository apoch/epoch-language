//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for describing entities
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"


namespace IRSemantics
{

	// Forward declarations
	class CodeBlock;
	class Expression;


	class Entity
	{
	// Construction and destruction
	public:
		explicit Entity(StringHandle name);
		~Entity();

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

	// Internal state
	private:
		StringHandle Name;
		CodeBlock* Code;
		std::vector<Expression*> Parameters;
		std::vector<Entity*> Chain;
	};

}

