//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Central interface for retrieving information about types
//

#include "pch.h"

#include "Virtual Machine/TypeInfo.h"

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"

#include <exception>


using namespace VM;


//
// Given a type ID, determine the amount of storage space needed
//
EPOCHVM size_t VM::GetStorageSize(EpochTypeID type)
{
	switch(type)
	{
	case EpochType_Identifier:			return sizeof(StringHandle);
	case EpochType_Function:			return sizeof(StringHandle);

	case EpochType_Integer:				return sizeof(Integer32);
	case EpochType_Integer16:			return sizeof(Integer16);
	case EpochType_String:				return sizeof(StringHandle);
	case EpochType_Boolean:				return sizeof(bool);
	case EpochType_Real:				return sizeof(Real32);

	case EpochType_Buffer:				return sizeof(BufferHandle);

	case EpochType_Nothing:				return 0;

	case EpochType_Error:
	case EpochType_Void:
		throw FatalException("Unable to determine the size of this variable/structure member");

	default:
		if(VM::GetTypeFamily(type) != VM::EpochTypeFamily_Structure && VM::GetTypeFamily(type) != VM::EpochTypeFamily_TemplateInstance)
			throw NotImplementedException("Unsupported data type in VM::GetStorageSize()");

		return sizeof(StructureHandle);
	}
}

//
// Given a type ID, determine the amount of storage space needed when the data is marshaled to C
//
EPOCHVM size_t VM::GetMarshaledSize(EpochTypeID type)
{
	switch(type)
	{
	case EpochType_Identifier:			return sizeof(wchar_t*);
	case EpochType_Function:			return sizeof(void*);

	case EpochType_Integer:				return sizeof(Integer32);
	case EpochType_Integer16:			return sizeof(Integer16);
	case EpochType_String:				return sizeof(wchar_t*);
	case EpochType_Boolean:				return sizeof(Integer32);
	case EpochType_Real:				return sizeof(Real32);

	case EpochType_Buffer:				return sizeof(wchar_t*);

	default:
		throw FatalException("Unable to determine the marshaled size of this variable/structure member");
	}
}
