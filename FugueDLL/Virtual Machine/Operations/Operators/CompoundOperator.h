//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for operators that can act on lists of parameters
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{
	namespace Operations
	{

		//
		// Operation for operators that can act on lists of parameters
		//
		class CompoundOperator
		{
		// Destruction
		public:
			virtual ~CompoundOperator()
			{
				for(std::list<VM::Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
					delete *iter;
			}

		// List support
		public:
			void AddOperation(VM::Operation* op);
			void AddOperationToFront(VM::Operation* op);

		// Additional queries
		public:
			const std::list<VM::Operation*>& GetSubOperations() const
			{ return SubOps; }

		// Internal tracking
		protected:
			std::list<VM::Operation*> SubOps;
		};

	}
}
