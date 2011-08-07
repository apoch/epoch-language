//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for code blocks and their contents
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/AnyStatement.h"


namespace AST
{

	//
	// Variant holding any type of entity invocation
	//
	typedef boost::variant
		<
			Undefined,
			DeferredEntity,
			DeferredPostfixEntity
		> AnyEntity;

	//
	// Variant holding any type of entry in a code block
	//
	// Entries can consist of entity invocations, assignments,
	// statements, or nested anonymous scopes (which are just
	// standalone code blocks).
	//
	typedef boost::variant
		<
			Undefined,
			AnyEntity,
			DeferredAssignment,
			AnyStatement,
			DeferredCodeBlock
		> CodeBlockEntry;


	//
	// Deferred wrapper for code block entries
	//
	// Since a code block entry is a variant, we can't forward declare
	// the deferred wrapper conveniently, so the typedef lives here.
	//
	typedef Deferred<CodeBlockEntry> DeferredCodeBlockEntry;

	//
	// Deferred wrapper for a container of code block entries
	//
	typedef std::vector<DeferredCodeBlockEntry, Memory::OneWayAlloc<DeferredCodeBlockEntry> > DeferredCodeBlockEntryVector;


	//
	// AST node representing a block of code
	//
	// Consists of a container of entries; see above
	//
	struct CodeBlock
	{
		DeferredCodeBlockEntryVector Entries;

		long RefCount;

		CodeBlock()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		CodeBlock(const CodeBlock&);
		CodeBlock& operator = (const CodeBlock&);
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredCodeBlock,
	(AST::DeferredCodeBlockEntryVector, Content->Entries)
)

