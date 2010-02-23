//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - language extensions
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"

#include "Language Extensions/ExtensionCatalog.h"



//
// Helper functor for registering extensions
//
template <typename DefinitionT>
struct RegisterEnumeratedExtension
{
	RegisterEnumeratedExtension(DefinitionT& grammardefinition)
		: GrammarDefinition(grammardefinition)
	{ }

	void operator () (const std::wstring& blockkeyword) const
	{
		GrammarDefinition.AddLanguageExtension(blockkeyword);
	}

private:
	DefinitionT& GrammarDefinition;
};

//
// Inform the parse analyzer that a language extension should be loaded
//
template <typename DefinitionT>
struct RegisterExtension : public ParseFunctorBase
{
	RegisterExtension(Parser::ParserState& state, DefinitionT& grammardefinition)
		: ParseFunctorBase(state),
		  GrammarDefinition(grammardefinition)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring extensionname(begin + 1, end - 1);
		Trace(L"RegisterExtension", extensionname);
		Extensions::ExtensionLibraryHandle handle = Extensions::RegisterExtensionLibrary(extensionname, State.GetParsedProgram());
		Extensions::EnumerateExtensionKeywords(RegisterEnumeratedExtension<DefinitionT>(GrammarDefinition), handle);
	}

private:
	DefinitionT& GrammarDefinition;
};


//
// Inform the parse analyzer that a language-extension block is beginning
//
struct PushExtensionBlockKeyword : public ParseFunctorBase
{
	PushExtensionBlockKeyword(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring keyword(begin, end);
		Trace(L"PushExtensionBlockKeyword", keyword);
		State.PushExtensionBlockKeyword(keyword);
	}
};


//
// Inform the parse analyzer that a language-extension block has been closed
//
struct RegisterExtensionBlock : public ParseFunctorBase
{
	RegisterExtensionBlock(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExtensionBlock");
		State.RegisterExtensionBlock();
	}
};
