//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for relaying messages between Epoch tasks
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	// Forward declarations
	class Block;
	class ResponseMapEntry;


	namespace Operations
	{

		//
		// Operation for sending a message to another task
		//
		class SendTaskMessage : public Operation, public SelfAware<SendTaskMessage>
		{
		// Construction
		public:
			SendTaskMessage(bool usestaskid, const std::wstring& messagename, const std::list<EpochVariableTypeID>& payloadtypes)
				: MessageName(messagename),
				  PayloadTypes(payloadtypes),
				  UsesTaskID(usestaskid)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

		// Additional queries
		public:
			const std::wstring& GetMessageName() const							{ return MessageName; }
			const std::list<EpochVariableTypeID>& GetPayloadTypes() const		{ return PayloadTypes; }
			bool DoesUseTaskID() const											{ return UsesTaskID; }

		// Internal tracking
		private:
			const std::wstring& MessageName;
			std::list<EpochVariableTypeID> PayloadTypes;
			bool UsesTaskID;
		};


		//
		// Operation for accepting a single incoming message
		//
		class AcceptMessage : public Operation, public SelfAware<AcceptMessage>
		{
		// Construction and destruction
		public:
			AcceptMessage(const std::wstring& messagename, Block* theblock, ScopeDescription* helperscope);
			virtual ~AcceptMessage();

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional queries
		public:
			const std::wstring& GetMessageName() const;
			const std::list<VM::EpochVariableTypeID>& GetPayloadTypes() const;

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			VM::ResponseMapEntry* ResponseEntry;
		};


		//
		// Operation for accepting a message given one or more possible patterns
		//
		class AcceptMessageFromResponseMap : public Operation, public SelfAware<AcceptMessageFromResponseMap>
		{
		// Construction
		public:
			AcceptMessageFromResponseMap(const std::wstring& mapname)
				: MapName(mapname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		
		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(MapName.c_str());
				payload.IsIdentifier = true;
				payload.ParameterCount = GetNumParameters(*scope);
				return payload;
			}
			
		// Internal tracking
		private:
			const std::wstring& MapName;
		};


		//
		// Operation for looking up which task forked the current task
		//
		class GetTaskCaller : public Operation, public SelfAware<GetTaskCaller>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_TaskHandle; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};

		//
		// Operation for determining who sent the most recently accepted message
		//
		class GetMessageSender : public Operation, public SelfAware<GetMessageSender>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_TaskHandle; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};

	}

}
