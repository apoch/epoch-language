#pragma once


// Dependencies
namespace AST { struct Structure; }

#include "Compiler/Session.h"


namespace ASTTraverse
{

	class CompilePassParseWithCallbacks
	{
	// Construction
	public:
		CompilePassParseWithCallbacks(const CompileSession::ParseCallbackTable& callbacks);

	// Non-copyable
	private:
		CompilePassParseWithCallbacks(const CompilePassParseWithCallbacks& rhs);
		CompilePassParseWithCallbacks& operator = (const CompilePassParseWithCallbacks& rhs);

	// Linkages
	public:
		const CompileSession::ParseCallbackTable& Callbacks;

	// AST traversal
	public:
		struct EntryHelper
		{
			template <typename T>
			void operator () (T&)
			{
				// Do nothing for most AST nodes
			}

			void operator () (AST::Structure& structure);

		private:
			CompilePassParseWithCallbacks* self;
			friend class CompilePassParseWithCallbacks;
		} Entry;

		struct ExitHelper
		{
			template <typename T>
			void operator () (T&)
			{ }
		} Exit;
	};

}


namespace CompilerPasses
{

	void ParseWithCallbacks(AST::Program& program, const CompileSession::ParseCallbackTable& callbacks);

}

