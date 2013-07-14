//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration for the compilation pass which
// feeds the AST's contents into a compiler plugin. This is
// pretty much entirely for getting the self-hosted compiler
// bootstrapped.
//

#include "pch.h"

#include "Compiler/Passes/ASTPlugin.h"

#include "Compiler/Self Hosting Plugins/Plugin.h"


using namespace ASTTraverse;


void CompilerPasses::HandASTToPlugin(AST::Program& program)
{
	ASTTraverse::ASTPlugin pass;
	ASTTraverse::DoTraversal(pass, program);

	if(!Plugins.InvokeBooleanPluginFunction(L"PluginCompileTimeCodeExecution"))
		throw FatalException("Compile time code execution failed");

	if(!Plugins.InvokeBooleanPluginFunction(L"PluginTypeInference"))
		throw FatalException("Type inference failed");

	if(!Plugins.InvokeBooleanPluginFunction(L"PluginValidate"))
		throw FatalException("Validation failed");
}


void ASTPlugin::EntryHelper::operator()(AST::Program&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIREnterProgram");
}

void ASTPlugin::ExitHelper::operator()(AST::Program&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIRExitProgram");
}


void ASTPlugin::EntryHelper::operator()(AST::Undefined& undefined)
{
	((void)(undefined));
}


void ASTPlugin::EntryHelper::operator()(AST::Structure& structure)
{
	((void)(structure));
}

void ASTPlugin::ExitHelper::operator()(AST::Structure& structure)
{
	((void)(structure));
}



void ASTPlugin::EntryHelper::operator()(AST::Function& function)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIREnterFunction", std::wstring(function.Name.begin(), function.Name.end()).c_str());
}

void ASTPlugin::ExitHelper::operator()(AST::Function&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIRExitFunction");
}


void ASTPlugin::EntryHelper::operator()(AST::SumType& sumtype)
{
	((void)(sumtype));
}

void ASTPlugin::ExitHelper::operator()(AST::SumType& sumtype)
{
	((void)(sumtype));
}


void ASTPlugin::EntryHelper::operator()(AST::TypeAlias& alias)
{
	((void)(alias));
}

void ASTPlugin::EntryHelper::operator()(AST::StrongTypeAlias& alias)
{
	((void)(alias));
}


void ASTPlugin::EntryHelper::operator()(AST::FunctionParameter& parameter)
{
	((void)(parameter));
}

void ASTPlugin::ExitHelper::operator()(AST::FunctionParameter& parameter)
{
	((void)(parameter));
}


void ASTPlugin::EntryHelper::operator()(AST::Assignment& assignment)
{
	((void)(assignment));
}

void ASTPlugin::ExitHelper::operator()(AST::Assignment& assignment)
{
	((void)(assignment));
}


void ASTPlugin::EntryHelper::operator()(AST::CodeBlock&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIREnterCodeBlock");
}

void ASTPlugin::ExitHelper::operator()(AST::CodeBlock&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIRExitCodeBlock");
}


void ASTPlugin::EntryHelper::operator()(AST::Initialization& initialization)
{
	((void)(initialization));
}

void ASTPlugin::ExitHelper::operator()(AST::Initialization& initialization)
{
	((void)(initialization));
}


void ASTPlugin::EntryHelper::operator()(AST::Expression& expression)
{
	((void)(expression));
}

void ASTPlugin::ExitHelper::operator()(AST::Expression& expression)
{
	((void)(expression));
}


void ASTPlugin::EntryHelper::operator()(AST::ExpressionComponent& component)
{
	((void)(component));
}

void ASTPlugin::ExitHelper::operator()(AST::ExpressionComponent& component)
{
	((void)(component));
}


void ASTPlugin::EntryHelper::operator()(AST::ExpressionFragment& fragment)
{
	((void)(fragment));
}

void ASTPlugin::ExitHelper::operator()(AST::ExpressionFragment& fragment)
{
	((void)(fragment));
}


void ASTPlugin::EntryHelper::operator()(AST::FunctionTag& tag)
{
	((void)(tag));
}

void ASTPlugin::ExitHelper::operator()(AST::FunctionTag& tag)
{
	((void)(tag));
}


void ASTPlugin::EntryHelper::operator()(AST::TemplateParameter& parameter)
{
	((void)(parameter));
}

void ASTPlugin::EntryHelper::operator()(AST::StructureMemberVariable& member)
{
	((void)(member));
}


void ASTPlugin::EntryHelper::operator()(AST::StructureMemberFunctionRef& funcref)
{
	((void)(funcref));
}

void ASTPlugin::ExitHelper::operator()(AST::StructureMemberFunctionRef& funcref)
{
	((void)(funcref));
}


void ASTPlugin::EntryHelper::operator()(AST::NamedFunctionParameter& parameter)
{
	((void)(parameter));
}

void ASTPlugin::ExitHelper::operator()(AST::NamedFunctionParameter& parameter)
{
	((void)(parameter));
}


void ASTPlugin::EntryHelper::operator()(AST::FunctionReferenceSignature& signature)
{
	((void)(signature));
}

void ASTPlugin::ExitHelper::operator()(AST::FunctionReferenceSignature& signature)
{
	((void)(signature));
}


void ASTPlugin::EntryHelper::operator()(AST::Nothing& nothing)
{
	((void)(nothing));
}


void ASTPlugin::EntryHelper::operator()(AST::Entity& entity)
{
	((void)(entity));
}

void ASTPlugin::ExitHelper::operator()(AST::Entity& entity)
{
	((void)(entity));
}

void ASTPlugin::EntryHelper::operator()(AST::PostfixEntity& entity)
{
	((void)(entity));
}

void ASTPlugin::ExitHelper::operator()(AST::PostfixEntity& entity)
{
	((void)(entity));
}

void ASTPlugin::EntryHelper::operator()(AST::ChainedEntity& entity)
{
	((void)(entity));
}

void ASTPlugin::ExitHelper::operator()(AST::ChainedEntity& entity)
{
	((void)(entity));
}



void ASTPlugin::EntryHelper::operator()(AST::Statement& statement)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIREnterStatement", std::wstring(statement.Identifier.begin(), statement.Identifier.end()).c_str());
}

void ASTPlugin::ExitHelper::operator()(AST::Statement&)
{
	Plugins.InvokeVoidPluginFunction(L"PluginIRExitStatement");
}

void ASTPlugin::EntryHelper::operator()(AST::PreOperatorStatement& statement)
{
	((void)(statement));
}

void ASTPlugin::ExitHelper::operator()(AST::PreOperatorStatement& statement)
{
	((void)(statement));
}

void ASTPlugin::EntryHelper::operator()(AST::PostOperatorStatement& statement)
{
	((void)(statement));
}

void ASTPlugin::ExitHelper::operator()(AST::PostOperatorStatement& statement)
{
	((void)(statement));
}



void ASTPlugin::EntryHelper::operator()(AST::RefTag& reftag)
{
	((void)(reftag));
}


void ASTPlugin::EntryHelper::operator()(AST::IdentifierT& identifier)
{
	((void)(identifier));
}



void ASTPlugin::EntryHelper::operator () (Markers::FunctionReturnExpression& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::FunctionReturnExpression& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::ExpressionComponentPrefixes& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::ExpressionComponentPrefixes& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::FunctionSignatureParams& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::FunctionSignatureParams& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::FunctionSignatureReturn& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::FunctionSignatureReturn& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::StructureFunctionParams& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::StructureFunctionParams& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::StructureFunctionReturn& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::StructureFunctionReturn& marker)
{
	((void)(marker));
}


void ASTPlugin::EntryHelper::operator () (Markers::TemplateArgs& marker)
{
	((void)(marker));
}

void ASTPlugin::ExitHelper::operator () (Markers::TemplateArgs& marker)
{
	((void)(marker));
}


