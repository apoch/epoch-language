#pragma once

#include "Compiler/Abstract Syntax Tree/RefCounting.h"
#include "Utility/Memory/OneWayAllocator.h"
#include "Lexer/Lexer.h"

namespace AST
{

	typedef boost::iterator_range<std::wstring::const_iterator> IdentifierT;

	struct IdentifierListRaw
	{
		typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > ContainedT;

		ContainedT Container;

		long RefCount;

		IdentifierListRaw()
			: RefCount(0)
		{ }

		IdentifierListRaw(const IdentifierT& identifier)
			: RefCount(0),
			  Container(1, identifier)
		{ }

	// Non-copyable
	private:
		IdentifierListRaw(const IdentifierListRaw&);
		IdentifierListRaw& operator = (const IdentifierListRaw&);
	};

	typedef DeferredContainer<IdentifierListRaw, boost::intrusive_ptr<IdentifierListRaw> > IdentifierList;

}

typedef std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> > IdentifierVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::IdentifierList,
	(IdentifierVec, Content->Container)
)
