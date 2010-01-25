#pragma once


// Forward declarations
namespace VM
{
	class ScopeDescription;
}


namespace Validator
{

	void TraceScopeEntry(const VM::ScopeDescription& scope);
	void TraceScopeExit(const VM::ScopeDescription& scope);

}

