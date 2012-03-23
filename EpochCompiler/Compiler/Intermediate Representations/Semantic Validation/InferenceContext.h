//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Contextual data wrapper for type inference
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/FunctionSignature.h"

#include <list>
#include <vector>


namespace IRSemantics
{

	struct InferenceContext
	{
		typedef std::vector<VM::EpochTypeID> TypePossibilities;
		typedef std::vector<TypePossibilities> PossibleParameterTypes;
		typedef std::list<PossibleParameterTypes> TypeListStack;

		typedef std::vector<FunctionSignature> SignaturePossibilities;
		typedef std::vector<SignaturePossibilities> PossibleSignatureSet;
		typedef std::list<PossibleSignatureSet> SignatureStack;

		enum ContextStates
		{
			CONTEXT_GLOBAL,
			CONTEXT_FUNCTION,
			CONTEXT_FUNCTION_RETURN,
			CONTEXT_CODE_BLOCK,
			CONTEXT_STATEMENT,
			CONTEXT_EXPRESSION,
			CONTEXT_ENTITY_PARAM,
			CONTEXT_ASSIGNMENT,
		};

		StringHandle ContextName;
		TypeListStack ExpectedTypes;
		SignatureStack ExpectedSignatures;
		ContextStates State;

		StringHandle FunctionName;

		// Construction
		InferenceContext(StringHandle name, ContextStates state)
			: ContextName(name),
			  State(state)
		{ }
	};

}

