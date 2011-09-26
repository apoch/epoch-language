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


bool Entity::Validate(const Program& program) const
{
	bool valid = true;

	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->Validate(program))
			valid = false;
	}

	for(std::vector<Entity*>::const_iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
	{
		if(!(*iter)->Validate(program))
			valid = false;
	}

	if(!Code)
		valid = false;
	else
	{
		if(!Code->Validate(program))
			valid = false;
	}

	return valid;
}

bool Entity::CompileTimeCodeExecution(Program& program)
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program))
			return false;
	}

	if(!Code)
		return false;

	if(!Code->CompileTimeCodeExecution(program))
		return false;

	for(std::vector<Entity*>::iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program))
			return false;
	}

	return true;
}
