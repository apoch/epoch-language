//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Traverser class for serializing code to Epoch Assembly format
//

#pragma once


// Forward declarations
namespace VM
{
	class Operation;
	class Program;
	class ScopeDescription;
	class Block;
	class FunctionSignature;
	class StructureType;
	class TupleType;
	class ResponseMap;
	class ResponseMapEntry;
}

namespace Marshalling { class CallDLL; }


// Dependencies
#include "Traverser/TraversalInterface.h"
#include <fstream>


namespace Serialization
{

	class SerializationTraverser;


	//
	// Explicit specializations of this function are used to perform
	// the actual serialization. Using a template makes it easier to
	// expose the code to the traversal wrapper class, i.e. we don't
	// have to define lots of overloads by hand for every operation.
	//
	template <class OperationClass>
	void SerializeNode(const OperationClass& op, SerializationTraverser& traverser);

	template <class OperationClass>
	const std::wstring& GetToken();


	//
	// Helper class used with the program traversal interface
	//
	// The traversal interface takes care of invoking this helper class
	// for each element of a loaded program, such as lexical scopes,
	// individual operations, and so on.
	//
	class SerializationTraverser
	{
	// Construction
	public:
		SerializationTraverser(const std::string& filename);

	// Traversal interface
	public:
		void SetProgram(VM::Program& program);

		void TraverseGlobalInitBlock(VM::Block* block);

		template <class OperationClass>
		void TraverseNode(const OperationClass& op)
		{
			if(TraversedObjects.find(&op) != TraversedObjects.end())
				return;

			TraversedObjects.insert(&op);
			SerializeNode(op, *this);
		}

		bool EnterBlock(const VM::Block& block);
		void ExitBlock(const VM::Block& block);
		void NullBlock();

		void RegisterScope(VM::ScopeDescription& scope);

		void EnterTask();
		void ExitTask();

		void EnterThread();
		void ExitThread();

	// State query interface
	public:
		VM::ScopeDescription* GetCurrentScope()		{ return CurrentScope; }
		void SetCurrentScope(VM::ScopeDescription* scope)
		{
			CurrentScope = scope;
		}

	// Serialization interface
	public:
		void WriteOp(const void* opptr, const std::wstring& token, bool newline);
		void WriteOp(const void* opptr, const std::wstring& token, VM::EpochVariableTypeID type);
		void WriteOp(const std::wstring& token);
		void WriteOp(const void* opptr, const std::wstring& token, const std::wstring& param);
		void WriteOp(const void* opptr, const std::wstring& token, const std::wstring& param1, const std::wstring& param2);
		void WriteOp(const void* opptr, const std::wstring& token, const std::wstring& param1, const std::wstring& param2, VM::EpochVariableTypeID param3);
		void WriteChainedOp(const void* opptr, const std::wstring& token, bool ischained, const std::wstring& param1, const std::wstring& param2);
		void WriteOpWithPayload(const VM::Operation* opptr, const std::wstring& token);
		void WritePayload(const Traverser::Payload& payload);

		void WriteCastOp(const void* opptr, const std::wstring& token, VM::EpochVariableTypeID originaltype, VM::EpochVariableTypeID destinationtype);
		void WriteCastOp(const void* opptr, const std::wstring& token, VM::EpochVariableTypeID originaltype);
		void WriteArithmeticOp(const void* opptr, const std::wstring& token, bool isfirstlist, bool issecondlist, size_t numparams);
		void WriteForkFuture(const void* opptr, const std::wstring& token, const std::wstring& varname, VM::EpochVariableTypeID type, bool usesthreadpool);
		void WriteSendMessage(const void* opptr, const std::wstring& token, bool usestaskid, const std::wstring& messagename, const std::list<VM::EpochVariableTypeID>& payloadtypes);
		void WriteAcceptMessage(const void* opptr, const std::wstring& token, const std::wstring& messagename, const std::list<VM::EpochVariableTypeID>& payloadtypes);
		void WriteConsList(const void* opptr, const std::wstring& token, VM::EpochVariableTypeID elementtype, size_t numelements);
		void WriteCompoundOp(const void* opptr, const std::wstring& token, size_t numops);
		void WriteCompoundOp(const void* opptr, const std::wstring& token, VM::EpochVariableTypeID type, size_t numops);

		void WriteHandoffOp(const void* opptr, const std::wstring& token, const std::wstring& libraryname);

	// Internal helpers
	private:
		void PadTabs();

		void WriteFunctionSignature(const VM::FunctionSignature& signature);
		void WriteStructureType(const VM::StructureType& type);
		void WriteTupleType(const VM::TupleType& type);
		void WriteResponseMap(const VM::ResponseMap& themap);
		void WriteResponseMapEntry(const VM::ResponseMapEntry& entry);

		void TraverseScope(VM::ScopeDescription& scope);

	// Internal tracking
	private:
		std::wofstream OutputStream;

		VM::Program* CurrentProgram;
		VM::ScopeDescription* CurrentScope;

		unsigned TabDepth;
		bool IgnoreTabPads;

		std::set<const void*> TraversedObjects;
		std::set<const void*> SkippedObjects;
	};

}

