//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a tuple type. This class primarily
// is used to ensure that members of a tuple are set up
// correctly in memory, and read from/written to easily.
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Types/CompositeType.h"


// Forward declarations
namespace Serialization { class SerializationTraverser; }


namespace VM
{

	// Forward declarations
	class ScopeDescription;

	//
	// Encapsulation for a tuple type definition
	//
	class TupleType : public CompositeType
	{
	// Helpers for reading/writing tuple members on the stack/heap
	public:
		virtual void ComputeOffsets(const ScopeDescription& scope);

		virtual size_t GetMemberStorageSize() const
		{ return StorageSize; }

		virtual size_t GetTotalSize() const
		{ return GetMemberStorageSize() + sizeof(size_t); }
	};


	//
	// Helper class for tracking all tuple types used in a program
	//
	// Each time a tuple type is defined (either explicitly by the user
	// or implicitly by a function's return value) it is registered in
	// this helper. Each unique tuple type is assigned a numeric ID,
	// and this ID is used to look up the tuple definition itself.
	//
	// Note that two tuple types are considered equivalent if and only
	// if their members are the all of the same types, in the same
	// order, and using the same identifiers.
	//
	class TupleTrackerClass
	{
	// Allow access to the file loading and serialization code, for simplicity
	public:
		friend class FileLoader;
		friend class Serialization::SerializationTraverser;

	// Constants
	public:
		static const IDType InvalidID;

	// Destruction and cleanup
	public:
		~TupleTrackerClass();
		void Detach();

	// Registration and lookup interface
	public:
		IDType RegisterTupleType(const TupleType& type);
		static IDType LookForMatchingTupleType(const TupleType& type);
		static IDType LookForMatchingTupleType(const ScopeDescription& scope);
		static IDType LookForMatchingTupleType(const std::vector<EpochVariableTypeID>& typelist);
		const TupleType& GetTupleType(IDType id) const;

		static TupleTrackerClass* GetOwnerOfTupleType(IDType id);

	// Cleanup of shared data (should be done each time a program is loaded)
	public:
		static void ResetSharedData();

	// Internal tracking
	private:
		static IDType CurrentID;
		static std::map<IDType, TupleTrackerClass*> OwnerMap;

		std::map<IDType, TupleType*> TupleTypeMap;
	};

}

