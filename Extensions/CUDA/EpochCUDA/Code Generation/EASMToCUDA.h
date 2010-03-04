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


extern bool CUDAAvailableForExecution;
extern bool CUDALibraryLoaded;


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
			bool IsDoWhile;

			LeafBlock(bool isfunction, bool isdowhile)
				: IsFunction(isfunction),
				  IsDoWhile(isdowhile)
			{ }
		};

		typedef std::stack<LeafBlock> LeafListStack;

	// Construction
	public:
		CompilationSession(TemporaryFileWriter& codefile, std::list<Traverser::ScopeContents>& registeredvariables, Extensions::CompileSessionHandle sessionhandle);
		CompilationSession(TemporaryFileWriter& codefile, TemporaryFileWriter& prototypesheaderfile, Extensions::CompileSessionHandle sessionhandle);

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
		void WriteLeaves(LeafList& leaves, bool leavesarestatements);

		std::wstring GenerateLeafCode(LeafList::const_iterator& iter, LeafList::const_iterator& enditer);

		void AdvanceLeafIterator(LeafList::const_iterator& iter) const;

		LeafList PopTrailingLeaves(LeafList& leaves);

		void SetNamedArrayMarshalIndex(const std::wstring& arrayname, unsigned index);
		unsigned GetNamedArrayMarshalIndex(const std::wstring& arrayname) const;

	// Internal tracking
	private:
		Extensions::CompileSessionHandle SessionHandle;

		unsigned TabDepth;

		std::stack<Traverser::Payload> Parameters;
		std::wostringstream ExpressionConstruction;

		std::list<Traverser::ScopeContents>* RegisteredVariables;

		TemporaryFileWriter& TemporaryCodeFile;
		TemporaryFileWriter* PrototypeHeaderFile;

		LeafListStack LeafStack;

		std::wstring PendingFunctionName;
		bool ExpectingFunctionReturns;
		bool ExpectingFunctionParams;

		bool ExpectingFunctionBlock;
		std::wstring PendingFunctionReturnValueName;
		VM::EpochVariableTypeID PendingFunctionReturnValueType;

		
		struct ArrayInfo
		{
			unsigned MarshalIndex;
			VM::EpochVariableTypeID ContainedType;
		};
		std::map<std::wstring, ArrayInfo> ArrayMarshalInfo;


		bool ExpectingDoWhileBlock;

		bool MarshalFloats;
		bool MarshalInts;

		bool MarshalFloatArrays;
		bool MarshalIntArrays;

		bool IgnoreIndentation;
	};

}

