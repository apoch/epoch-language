//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for operators that can act on lists of parameters
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/CompoundOperator.h"
#include "Virtual Machine/Operations/StackOps.h"


using namespace VM;
using namespace VM::Operations;


namespace
{

	Operation* GetRealOperation(Operation* op)
	{
		PushOperation* pushop = dynamic_cast<PushOperation*>(op);
		if(pushop)
			return pushop->GetNestedOperation();

		return op;
	}
}


void CompoundOperator::AddOperation(VM::Operation* op)
{
	SubOps.push_back(GetRealOperation(op));
}

void CompoundOperator::AddOperationToFront(VM::Operation* op)
{
	SubOps.push_front(GetRealOperation(op));
}

