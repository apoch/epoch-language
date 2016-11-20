// ShellSection.cpp
// Implementation file for functions used to ensure Isolated Shell executables
// define a section used to mark them as such.

#include "stdafx.h"
#include "ShellSection.h"

// "vsshell" section is used to mark the executable as being based on the Visual Studio Shell.
#pragma section("vsshell", read, write)
__declspec(allocate("vsshell"))
int iVSShellSectionField = 0;

// The objective of this function is to ensure the field above is used in code.
// Otherwise, the compiler could optimize the section away in release builds.
void InitializeVSShellSection()
{
    iVSShellSectionField = 1;
}
