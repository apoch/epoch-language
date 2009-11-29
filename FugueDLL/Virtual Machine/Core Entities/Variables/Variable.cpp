//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper classes for Epoch variables
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"


// Shared pool of string data; see class definition for details
VM::StringVariable::PoolType VM::StringVariable::Pool;

// Shared pool of buffers; compare with string pooling system
VM::BufferVariable::PoolType VM::BufferVariable::Pool;

