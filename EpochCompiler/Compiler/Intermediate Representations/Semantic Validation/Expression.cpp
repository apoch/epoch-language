//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing expressions
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"


using namespace IRSemantics;


bool Expression::Validate(const Program& program) const
{
	// TODO - type-check expressions
	return true;
}


ExpressionComponent::ExpressionComponent(const std::vector<StringHandle>& prefixes)
	: UnaryPrefixes(prefixes),
	  Atom(NULL)
{
}

ExpressionComponent::~ExpressionComponent()
{
	delete Atom;
}

void ExpressionComponent::SetAtom(ExpressionAtom* atom)
{
	delete Atom;
	Atom = atom;
}


ExpressionFragment::ExpressionFragment(StringHandle operatorname, ExpressionComponent* component)
	: OperatorName(operatorname),
	  Component(component)
{
}

ExpressionFragment::~ExpressionFragment()
{
	delete Component;
}


Expression::Expression(ExpressionComponent* first)
	: First(first)
{
}

Expression::~Expression()
{
	delete First;

	for(std::vector<ExpressionFragment*>::iterator iter = Remaining.begin(); iter != Remaining.end(); ++iter)
		delete *iter;
}

void Expression::AddFragment(ExpressionFragment* fragment)
{
	Remaining.push_back(fragment);
}


ExpressionAtomStatement::ExpressionAtomStatement(Statement* statement)
	: MyStatement(statement)
{
}

ExpressionAtomStatement::~ExpressionAtomStatement()
{
	delete MyStatement;
}


ExpressionAtomParenthetical::ExpressionAtomParenthetical(Parenthetical* parenthetical)
	: MyParenthetical(parenthetical)
{
}

ExpressionAtomParenthetical::~ExpressionAtomParenthetical()
{
	delete MyParenthetical;
}



ParentheticalPreOp::ParentheticalPreOp(PreOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPreOp::~ParentheticalPreOp()
{
	delete MyStatement;
}


ParentheticalPostOp::ParentheticalPostOp(PostOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPostOp::~ParentheticalPostOp()
{
	delete MyStatement;
}

