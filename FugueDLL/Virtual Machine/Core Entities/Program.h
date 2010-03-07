//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class encapsulating a complete Epoch program.
//

#pragma once


// Dependencies
#include "Utility/Memory/Stack.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Thread Pooling/PoolTracker.h"
#include "Language Extensions/ExtensionCatalog.h"

// Forward declarations
class HeapStorage;


namespace VM
{

	//
	// Wrapper class for entire Epoch programs
	//
	class Program
	{
	// Construction
	public:
		Program();
		~Program();

	// Program flags
	public:
		void SetUsesConsole()								{ FlagsUsesConsole = true; }
		bool GetUsesConsole() const							{ return FlagsUsesConsole; }

	// Global scope access
	public:
		ScopeDescription& GetGlobalScope()					{ return GlobalScope; }
		const ScopeDescription& GetGlobalScope() const		{ return GlobalScope; }

		ActivatedScope* GetActivatedGlobalScope()			{ return ActivatedGlobalScope; }

		VM::Block& CreateGlobalInitBlock();
		void ReplaceGlobalInitBlock(VM::Block* block);
		const VM::Block* GetGlobalInitBlock() const
		{ return GlobalInitBlock; }

	// Stack space access
	public:
		StackSpace& GetStack()					{ return Stack; }

	// Execution interface
	public:
		RValuePtr Execute();

	// Thread pool management interface
	public:
		void CreateThreadPool(const std::wstring& poolname, unsigned numthreads);
		void AddPoolWorkItem(const std::wstring& poolname, const std::wstring& threadname, std::auto_ptr<Threads::PoolWorkItem> workitem);
		bool HasThreadPool(const std::wstring& poolname) const;

	// Traversal interface
	public:
		template <class TraverserT>
		void Traverse(TraverserT& traverser)
		{
			traverser.SetProgram(*this);
			GlobalScope.Traverse(traverser);
			traverser.TraverseGlobalInitBlock(GlobalInitBlock);
			if(GlobalInitBlock)
				GlobalInitBlock->Traverse(traverser);
			
			Extensions::TraverseExtensions(traverser);
		}

	// Static string pool management
	public:
		const std::wstring& PoolStaticString(const std::wstring& str);

	// Internal tracking
	private:
		std::set<std::wstring> StaticStringPool;
		ScopeDescription GlobalScope;
		ActivatedScope* ActivatedGlobalScope;
		StackSpace Stack;
		VM::Block* GlobalInitBlock;
		HeapStorage* GlobalStorageSpace;
		ThreadPoolTracker ThreadPools;

		bool FlagsUsesConsole;

	// Shared internal tracking
	private:
		static unsigned ProgramInstances;
	};
}

