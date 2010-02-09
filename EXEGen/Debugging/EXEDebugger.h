//
// The Epoch Language Project
// Win32 EXE Generator
//
// Facilities for debugging compiled Epoch .EXEs
//

#pragma once


namespace Debugger
{

	//
	// Wrapper class for an .EXE file debug session
	//
	class EXEDebugger
	{
	// Construction and destruction
	public:
		EXEDebugger(const std::wstring& filename);
		~EXEDebugger();

	// Access to loaded data
	public:
		const void* GetBinaryCodeBuffer() const
		{ return BinaryCodeBuffer; }

	// Internal helpers
	private:
		void Cleanup();
		void FindInterestingLocations();

	// Internal tracking
	private:
		const void* EntireFileBuffer;
		const void* BinaryCodeBuffer;

		HANDLE FileHandle;
		HANDLE Mapping;
	};

}

