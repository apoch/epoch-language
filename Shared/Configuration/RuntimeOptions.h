//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Configurable runtime options for the virtual machine.
// These options can usually be set either dynamically while the engine
// runs, or via an external config file, or via command line switches.
// See RuntimeOptions.cpp for a breakdown of each option's purpose and
// acceptable values.
//

#pragma once


namespace Config
{

	void LoadFromConfigFile();


	extern bool TraceParserExecution;

	extern size_t StackSize;

	extern unsigned NumMessageSlots;

	extern unsigned TabWidth;

}

