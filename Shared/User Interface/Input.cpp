//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of user interface input routines.
//

#include "pch.h"
#include "User Interface/Input.h"

#include <iostream>

using namespace UI;


//
// Block execution while the user inputs a value
//
std::wstring Input::BlockingRead()
{
	std::wstring result;
	std::getline(std::wcin, result);
	return result;
}
