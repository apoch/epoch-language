//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Various work item wrapper classes for use with worker thread pools
//

#pragma once


// Dependencies
#include "Utility/Threading/ThreadPool.h"


namespace VM
{

	// Forward declarations
	class Block;
	struct ExecutionContext;
	class Future;
	class Program;

	namespace Operations
	{ class ParallelFor; }


	struct ForkThreadWorkItem : public Threads::PoolWorkItem
	{
	// Construction
	public:
		ForkThreadWorkItem(Block& codeblock, ExecutionContext& context);

	// Work item interface
	public:
		virtual void PerformWork();

	// Internal tracking
	protected:
		Block& TheBlock;
		Program& RunningProgram;
	};


	struct FutureWorkItem : public Threads::PoolWorkItem
	{
	// Construction
	public:
		FutureWorkItem(Future& thefuture, ExecutionContext& context);

	// Work item interface
	public:
		virtual void PerformWork();

	// Internal tracking
	protected:
		Future& TheFuture;
	};


	struct ParallelForWorkItem : public Threads::PoolWorkItem
	{
	// Construction
	public:
		ParallelForWorkItem(VM::Operations::ParallelFor& pforop, VM::ActivatedScope* parentscope, Block& codeblock, Program& runningprogram, size_t lowerbound, size_t upperbound, const std::wstring& countervarname, unsigned skipinstructions);

	// Work item interface
	public:
		virtual void PerformWork();

	// Internal tracking
	protected:
		VM::Operations::ParallelFor& ParallelForOp;
		VM::ActivatedScope* ParentScope;

		Block& TheBlock;
		Program& RunningProgram;

		size_t LowerBound;
		size_t UpperBound;

		const std::wstring& CounterVarName;

		unsigned SkipInstructions;
	};

}


