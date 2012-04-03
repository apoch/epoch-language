//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for identifiers and lists of identifiers
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/RefCounting.h"
#include "Utility/Memory/OneWayAllocator.h"
#include "Lexer/Lexer.h"


namespace AST
{

	//
	// An identifier is simply a pair of iterators pointing
	// into the original code text buffer, thus eliminating
	// the overhead of making a string copy of every single
	// identifier in the program.
	//
	typedef boost::iterator_range<positertype> IdentifierT;

	//
	// An identifier list is simply a container of identifiers
	//
	// Since identifier lists are very common (every expression
	// supports a set of unary prefixes, which is an identifier
	// list; structure member accesses are identifier lists) it
	// makes sense to defer the construction of the vector that
	// holds the identifiers for as long as possible. Using the
	// below structure has proven to be a net performance win.
	//
	struct IdentifierListRaw
	{
		typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > ContainedT;

		ContainedT Container;

		long RefCount;

		IdentifierListRaw()
			: RefCount(0)
		{ }

		IdentifierListRaw(const IdentifierT& identifier)
			: Container(1, identifier), RefCount(0)
		{ }

	// Non-copyable
	private:
		IdentifierListRaw(const IdentifierListRaw&);
		IdentifierListRaw& operator = (const IdentifierListRaw&);
	};

	//
	// Deferred wrapper for an identifier list
	//
	typedef DeferredContainer<IdentifierListRaw, boost::intrusive_ptr<IdentifierListRaw> > IdentifierList;

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > IdentifierVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::IdentifierList,
	(IdentifierVec, Content->Container)
)
