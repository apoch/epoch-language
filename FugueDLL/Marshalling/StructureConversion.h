//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for converting Epoch structures into C-compatible structures
//
// WARNING - all marshalling code is platform-specific!
//

#pragma once

// Forward declarations
namespace VM
{
	class ScopeDescription;
	class ActivatedScope;
	class StructureVariable;
}

namespace Marshalling
{
	void CStructToEpoch(const std::vector<Byte*>& buffers, const std::vector<VM::StructureVariable*> variables);
	void* EpochToCStruct(const VM::StructureVariable& structvar, VM::ActivatedScope& params);
}

