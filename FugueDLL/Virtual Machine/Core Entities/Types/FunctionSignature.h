//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Representation of a function's signature. Used for higher-order
// functions and polymorphism.
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"


// Forward declarations
namespace Serialization { class SerializationTraverser; }


namespace VM
{
	// Forward declarations
	class FunctionBase;
	class ScopeDescription;


	//
	// Wrapper for a function signature. A function's signature is uniquely
	// defined by its parameters and returns; two functions with identical
	// parameters and returns have identical signatures, and therefore can
	// be used interchangeably as parameters to a higher-order function.
	//
	class FunctionSignature
	{
	// Internal constants
	public:
		enum ParamTypeFlags
		{
			PARAMTYPEFLAG_NONE = 0,
			PARAMTYPEFLAG_ISREFERENCE = 0x01,
			PARAMTYPEFLAG_ISARRAY = 0x02,
		};

	// Construction and destruction
	public:
		FunctionSignature() { }
		FunctionSignature(const FunctionSignature& rhs);
		~FunctionSignature();

	// Parameter and return management
	public:
		void AddParam(EpochVariableTypeID type, IDType typehint, FunctionSignature* signaturehint);
		const std::vector<EpochVariableTypeID>& GetParamTypes() const
		{ return Params; }

		void AddReturn(EpochVariableTypeID type, IDType typehint);

		void SetLastParamToReference();

		size_t GetNumMembers() const
		{ return Params.size(); }

	// Comparison to a function wrapper object
	public:
		bool DoesFunctionMatchSignature(const FunctionBase* function, const ScopeDescription& scope) const;
		bool DoesSignatureMatch(const FunctionSignature& signature) const;

	// Type hints for structures, tuples, and higher-order functions
	public:
		IDType GetVariableTupleTypeID(unsigned index) const;
		IDType GetVariableStructureTypeID(unsigned index) const;

		const FunctionSignature& GetFunctionSignature(unsigned index) const;

		EpochVariableTypeID GetVariableType(unsigned index) const;

		bool IsReference(unsigned index) const;
		bool IsFunctionSignature(unsigned index) const;

		bool IsArray(unsigned index) const;
		EpochVariableTypeID GetArrayType(unsigned index) const;

		EpochVariableTypeID GetReturnType() const;
		const std::vector<EpochVariableTypeID>& GetReturnTypes() const
		{ return Returns; }

	// Copy operations
	public:
		FunctionSignature* Clone() const;
		FunctionSignature& operator = (const FunctionSignature& rhs);

	// Internal helpers
	protected:
		void Clean();
		void CopyFrom(const FunctionSignature& rhs);

	// Internal tracking
	private:
		std::vector<EpochVariableTypeID> Params;
		std::vector<EpochVariableTypeID> Returns;

		std::vector<IDType> ParamTypeHints;
		std::vector<unsigned> ParamFlags;
		std::vector<FunctionSignature*> FunctionSignatures;

		std::vector<IDType> ReturnTypeHints;

	// Access for serialization
	public:
		friend class Serialization::SerializationTraverser;
	};

}

