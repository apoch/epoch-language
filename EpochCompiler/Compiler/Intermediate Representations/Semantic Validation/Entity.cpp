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
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"


using namespace IRSemantics;


//
// Construct and initialize an entity invocation IR node
//
Entity::Entity(StringHandle name)
	: Name(name), Code(NULL), PostfixName(0)
{
}

//
// Destruct and clean up an entity invocation IR node
//
Entity::~Entity()
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete *iter;

	for(std::vector<Entity*>::iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
		delete *iter;

	delete Code;
}


//
// Append a parameter to the list of expressions passed to the entity
//
void Entity::AddParameter(Expression* expression)
{
	Parameters.push_back(expression);
}

//
// Set the block of code attached to the entity invocation
//
// This function guarantees that it is safe to replace existing code blocks.
// Note that the entity IR node takes ownership of the passed code block.
//
void Entity::SetCode(CodeBlock* code)
{
	delete Code;
	Code = code;
}

//
// Append an entity invocation to the end of the entity chain
//
// Note that the entity IR node takes ownership of all entities in the chain.
//
void Entity::AddChain(Entity* entity)
{
	Chain.push_back(entity);
}


//
// Validate the contents of an entity invocation node
//
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

//
// Perform compile-time code execution on an entity invocation
//
// Note that the code block is not requested to perform compile-time code
// execution at this point; we wait to do so until the type inference pass
// is performed in order to prevent variables from being used prior to
// their point of definition in the program. Since variable definitions are
// compile-time code that modifies the local lexical scope, if we perform
// this process too soon, variables can be referenced anywhere in the scope
// in which they are defined, rather than only after the point of definition.
//
bool Entity::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, CompileErrors& errors)
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, false, errors))
			return false;
	}

	if(!Code)
		return false;

	for(std::vector<Entity*>::iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, errors))
			return false;
	}

	return true;
}

//
// Perform type inference on an entity invocation's contents
//
bool Entity::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	CompileTimeCodeExecution(program, activescope, errors);

	InferenceContext newcontext(0, InferenceContext::CONTEXT_ENTITY_PARAM);
	newcontext.FunctionName = context.FunctionName;

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, newcontext, i, Parameters.size(), errors))
			return false;

		++i;
	}

	if(!Code)
		return false;

	if(!Code->TypeInference(program, context, errors))
		return false;

	for(std::vector<Entity*>::iterator iter = Chain.begin(); iter != Chain.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, context, errors))
			return false;
	}

	program.AddScope(Code->GetScope());
	return true;
}

