//
// The Epoch Language Project
// CUDA Interoperability Library
//
// EpochASM-to-CUDA compiler logic
//
// Compilation is performed in stages known as "sessions." Each session
// consists of compiling one EpochASM code block to the equivalent CUDA
// code. Multiple sessions may be performed before finally compiling to
// PTX code via NVCC.
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"
#include "Traverser/TraversalInterface.h"
#include "Utility/Files/TempFile.h"


namespace Compiler
{

	class CompilationSession
	{
	// Internal helper structures and type shortcuts
	private:
		struct Leaf
		{
			std::wstring Token;
			Traverser::Payload Payload;

			Leaf(const wchar_t* token, const Traverser::Payload* payload)
				: Token(token),
				  Payload(*payload)
			{ }
		};

		typedef std::vector<Leaf> LeafList;

		struct LeafBlock
		{
			LeafList Leaves;
			bool IsFunction;

			LeafBlock(bool isfunction)
				: IsFunction(isfunction)
			{ }
		};

		typedef std::stack<LeafBlock> LeafListStack;

	// Construction
	public:
		CompilationSession(TemporaryFileWriter& codefile, std::list<Traverser::ScopeContents>& registeredvariables, Extensions::CompileSessionHandle sessionhandle);

	// Preparation and data loading
	public:
		void RegisterScope(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents);
		void RegisterLeaf(const wchar_t* token, const Traverser::Payload* payload);

		void EnterNode();
		void ExitNode();

	// Additional output helpers
	public:
		void FunctionPreamble(Extensions::OriginalCodeHandle handle);
		void MarshalOut();

		void ExpectFunctionTraversal(const std::wstring& functionname);

	// Internal helpers
	private:
		void PadTabs();
		void OutputPayload(const Traverser::Payload& payload, std::wostream& stream);

		void WriteScopeContents(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents);
		void WriteLeaves(LeafList& leaves);

		std::wstring GenerateLeafCode(const LeafList& leaves, LeafList::const_iterator& iter);

		size_t GetArraySize(const std::wstring& arrayname) const;

		void AdvanceLeafIterator(LeafList::const_iterator& iter) const;

	// Internal tracking
	private:
		Extensions::CompileSessionHandle SessionHandle;

		unsigned TabDepth;

		std::stack<Traverser::Payload> Parameters;
		std::wostringstream ExpressionConstruction;

		std::list<Traverser::ScopeContents>& RegisteredVariables;

		TemporaryFileWriter& TemporaryCodeFile;

		LeafListStack LeafStack;

		std::map<std::wstring, size_t> ArraySizeCache;

		std::wstring PendingFunctionName;
		bool ExpectingFunctionReturns;
		bool ExpectingFunctionParams;

		bool ExpectingFunctionBlock;
		std::wstring PendingFunctionReturnValueName;
		VM::EpochVariableTypeID PendingFunctionReturnValueType;


		bool MarshalFloats;
		bool MarshalInts;

		bool MarshalFloatArrays;
		bool MarshalIntArrays;
	};

}

