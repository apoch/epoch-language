#pragma once


#include "Compiler/Abstract Syntax Tree/AnyStatement.h"


namespace AST
{

	typedef boost::variant
		<
			Undefined,
			DeferredEntity,
			DeferredPostfixEntity
		> AnyEntity;

	typedef boost::variant
		<
			Undefined,
			AnyEntity,
			DeferredAssignment,
			AnyStatement,
			DeferredCodeBlock
		> CodeBlockEntry;


	typedef Deferred<CodeBlockEntry> DeferredCodeBlockEntry;

	typedef std::vector<DeferredCodeBlockEntry, Memory::OneWayAlloc<DeferredCodeBlockEntry> > DeferredCodeBlockEntryVector;


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


BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredCodeBlock,
	(AST::DeferredCodeBlockEntryVector, Content->Entries)
)

