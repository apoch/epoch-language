//
// The Epoch Language Project
// Shared Library Code
//
// Central interface for retrieving information about types
//

#include "pch.h"

#include "Metadata/TypeInfo.h"

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"

#include <exception>


using namespace Metadata;


//
// Given a type ID, determine the amount of storage space needed
//
size_t Metadata::GetStorageSize(EpochTypeID type)
{
	switch(MakeNonReferenceType(type))
	{
	case EpochType_Identifier:			return sizeof(StringHandle);

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
		if(Metadata::GetTypeFamily(type) == Metadata::EpochTypeFamily_Function)
			return sizeof(StringHandle);

		if(Metadata::IsStructureType(type))
			return sizeof(StructureHandle);
	
		throw NotImplementedException("Unsupported data type in Metadata::GetStorageSize()");
	}
}

//
// Given a type ID, determine the amount of storage space needed when the data is marshaled to C
//
size_t Metadata::GetMarshaledSize(EpochTypeID type)
{
	switch(type)
	{
	case EpochType_Identifier:			return sizeof(wchar_t*);

	case EpochType_Integer:				return sizeof(Integer32);
	case EpochType_Integer16:			return sizeof(Integer16);
	case EpochType_String:				return sizeof(wchar_t*);
	case EpochType_Boolean:				return sizeof(Integer32);
	case EpochType_Real:				return sizeof(Real32);

	case EpochType_Buffer:				return sizeof(wchar_t*);

	default:
		if(GetTypeFamily(type) == EpochTypeFamily_Function)
			return sizeof(void*);

		throw FatalException("Unable to determine the marshaled size of this variable/structure member");
	}
}
