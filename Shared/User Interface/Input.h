//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// User interface input routines. This structure is designed to decouple
// the bulk of the application from the actual type of user interface.
// This should make it trivial to port the code from one UI framework to
// another, between console and GUI formats, etc.
//

#pragma once

namespace UI
{

	//
	// Wrapper class for acquiring input from the user
	//
	class Input
	{
	// Read operations
	public:
		std::wstring BlockingRead();
	};

}

