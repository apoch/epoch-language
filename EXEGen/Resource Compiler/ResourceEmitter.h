//
// The Epoch Language Project
// Win32 EXE Generator
//
// Base interface for objects that emit compiled resources to the output file
//

#pragma once

// Forward declarations
class LinkWriter;


namespace ResourceCompiler
{

	//
	// Interface for resource emitter logic
	//
	class ResourceEmitter
	{
	public:
		virtual ~ResourceEmitter()
		{ }

	public:
		virtual void Emit(LinkWriter& writer) = 0;
		virtual DWORD GetSize() const = 0;

		virtual void SetOffset(DWORD offset)
		{ Offset = offset; }

	protected:
		DWORD Offset;
	};

}

