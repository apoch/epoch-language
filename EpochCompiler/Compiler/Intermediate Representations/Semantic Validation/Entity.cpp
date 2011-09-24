//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for describing entities
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"


using namespace IRSemantics;


Entity::Entity(StringHandle name)
	: Code(NULL),
	  Name(name)
{
}

Entity::~Entity()
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete *iter;

	for(std::vector<Entity*>::iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
		delete *iter;

	delete Code;
}


void Entity::AddParameter(Expression* expression)
{
	Parameters.push_back(expression);
}

void Entity::SetCode(CodeBlock* code)
{
	delete Code;
	Code = code;
}

void Entity::AddChain(Entity* entity)
{
	Chain.push_back(entity);
}

