//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for all composite data types in Epoch.
// A composite data type is any construct which contains
// multiple members that are bound together in memory.
// Tuples and structures (records) are the primary kinds
// of composite type in Epoch.
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"


// Forward declarations
namespace Serialization { class SerializationTraverser; }


namespace VM
{

	// Forward declarations
	class ScopeDescription;
	class StructureType;
	class TupleType;
	class FunctionSignature;

	//
	// Base class for a composite type definition
	//
	class CompositeType
	{
	// Construction and destruction
	public:
		CompositeType()
			: StorageSize(0)
		{ }

		virtual ~CompositeType()
		{ }

	// Member setup interface
	public:
		void AddMember(const std::wstring& name, EpochVariableTypeID type);
		void AddMember(const std::wstring& name, const StructureType& structtype, IDType typehint);
		void AddMember(const std::wstring& name, const TupleType& type, IDType typehint);
		void AddFunctionMember(const std::wstring& name, const std::wstring& hint);

	// Member query interface
	public:
		EpochVariableTypeID GetMemberType(const std::wstring& name) const;
		IDType GetMemberTypeHint(const std::wstring& name) const;
		const std::wstring& GetMemberTypeHintString(const std::wstring& name) const;

		const std::vector<std::wstring>& GetMemberOrder() const
		{ return MemberOrder; }

		bool HasMember(const std::wstring& member) const
		{ return (MemberInfoMap.find(member) != MemberInfoMap.end()); }

	// Helpers for reading/writing members on the stack/heap
	public:
		virtual void ComputeOffsets(const ScopeDescription& scope) = 0;
		size_t GetMemberOffset(const std::wstring& name) const;

		virtual size_t GetMemberStorageSize() const = 0;
		virtual size_t GetTotalSize() const = 0;

	// Internal tracking
	protected:
		struct MemberInfo
		{
			EpochVariableTypeID Type;
			size_t Offset;
			IDType TypeHint;
			std::wstring StringHint;
		};

		std::map<std::wstring, MemberInfo> MemberInfoMap;
		std::vector<std::wstring> MemberOrder;
		size_t StorageSize;
	
	// Helper access to serializer
	public:
		friend class Serialization::SerializationTraverser;
	};


}

