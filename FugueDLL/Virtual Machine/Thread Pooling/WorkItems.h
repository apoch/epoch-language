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

}


