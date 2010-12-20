//
// The Epoch Language Project
// Shared Library Code
//
// Configurable runtime options for the virtual machine.
// This module defines the variables used to store the options
// declared in RuntimeOptions.h.
//

#include "pch.h"

#include "Configuration/RuntimeOptions.h"
#include "Configuration/ConfigFile.h"


// Space reserved for the execution stack, in bytes (default is 1MB)
// Each forked task will get this amount of stack space as well
size_t Config::StackSize = (1024 * 1024);


// Number of pre-allocated message slots in the inter-thread mailboxes
unsigned Config::NumMessageSlots = 64;


// Default width of a tab (in spaces)
unsigned Config::TabWidth = 4;



//
// Load the configuration options from an external config file
// If any particular option is not present in the config file,
// the default value set above in code is preserved.
//
void Config::LoadFromConfigFile()
{
	ConfigReader config;

	config.ReadConfig(L"stacksize", Config::StackSize);

	config.ReadConfig(L"messageslots", Config::NumMessageSlots);

	config.ReadConfig(L"tabwidth", Config::TabWidth);
}

