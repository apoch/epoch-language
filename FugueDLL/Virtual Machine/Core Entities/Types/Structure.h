//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a structure (record) type. This class primarily
// is used to ensure that members of a structure are set up correctly
// in memory, and read from/written to easily.
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
	// Encapsulation for a structure type definition
	//
	class StructureType : public CompositeType
	{
	// Helpers for reading/writing structure members on the stack/heap
	public:
		virtual void ComputeOffsets(const ScopeDescription& scope);

		virtual size_t GetMemberStorageSize() const
		{ return StorageSize; }

		virtual size_t GetTotalSize() const
		{ return GetMemberStorageSize() + sizeof(size_t); }
	};


	//
	// Helper class for tracking all structure types used in a program
	//
	// Each time a structure type is defined it is registered in this
	// helper. Each unique structure type is assigned a numeric ID,
	// and this ID is used to look up the structure definition itself.
	//
	// Note that two structure types are considered equivalent if and
	// only if their members are the all of the same types, in the same
	// order, and using the same identifiers.
	//
	class StructureTrackerClass
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
		~StructureTrackerClass();
		void Detach();

	// Registration and lookup interface
	public:
		IDType RegisterStructureType(const StructureType& type);
		static IDType LookForMatchingStructureType(const StructureType& type);
		static IDType LookForMatchingStructureType(const ScopeDescription& scope);
		const StructureType& GetStructureType(IDType id) const;

		static StructureTrackerClass* GetOwnerOfStructureType(IDType id);

	// Cleanup of shared data (should be done each time a program is loaded)
	public:
		static void ResetSharedData();

	// Internal tracking
	private:
		static IDType CurrentID;
		static std::map<IDType, StructureTrackerClass*> OwnerMap;

		std::map<IDType, StructureType*> StructureTypeMap;
	};

}

