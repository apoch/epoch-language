//
// The Epoch Language Project
// Shared Library Code
//
// Definitions for bytecode tag values
//
// These values are generally metadata that provide information such as
// what type of code block is associated with a subsequent code sequence,
// and so on. They are not instructions for the VM; therefore they may
// overlap instruction values, and the VM is not expected to correctly
// execute any of the metadata (that's the job of the parser/loader).
//

#pragma once


namespace Bytecode
{

	typedef unsigned EntityTag;

	namespace EntityTags
	{

		static const EntityTag Function = 0x00;

	}

}

