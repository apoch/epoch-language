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

	//
	// Wrapper for describing a context in which
	// type inference is to be performed. Since
	// Epoch type deduction is highly contextual,
	// this wrapper sees extensive use in the
	// type inference engine.
	//
	struct InferenceContext
	{
		//
		// Just a set of possible types
		//
		typedef std::vector<Metadata::EpochTypeID> TypePossibilities;

		//
		// Maps a parameter index onto a set of
		// possible types for each parameter
		//
		typedef std::vector<TypePossibilities> PossibleParameterTypes;

		//
		// Tracks the stack of expected types for
		// use in deducing complex situations
		//
		typedef std::list<PossibleParameterTypes> TypeListStack;

		
		//
		// Basically the same as the above typedefs, but
		// using function signatures instead of types.
		//
		typedef std::vector<FunctionSignature> SignaturePossibilities;
		typedef std::vector<SignaturePossibilities> PossibleSignatureSet;
		typedef std::list<PossibleSignatureSet> SignatureStack;

		//
		// Different states the inference engine might be in
		//
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

		// Actual context state
		StringHandle ContextName;
		StringHandle FunctionName;
		TypeListStack ExpectedTypes;
		SignatureStack ExpectedSignatures;
		ContextStates State;

		// Construction
		InferenceContext(StringHandle name, ContextStates state)
			: ContextName(name),
			  State(state)
		{ }
	};

}

