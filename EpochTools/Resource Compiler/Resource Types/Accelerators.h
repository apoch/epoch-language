//
// The Epoch Language Project
// Win32 EXE Generator
//
// Accelerator resource compiler
//

#pragma once

// Forward declarations
class LinkWriter;

// Dependencies
#include "Resource Compiler/ResourceEmitter.h"
#include <vector>


namespace ResourceCompiler
{

	struct AcceleratorTable
	{
		struct Entry
		{
			WORD flags;
			WORD ascii;
			WORD id;
			WORD padding;
		};

		DWORD GetTotalSize() const;
		void Emit(LinkWriter& writer) const;

		void LoadFromStream(std::wistream& in);

		std::vector<Entry> Entries;
	};


	class AcceleratorEmitter : public ResourceEmitter
	{
	// Construction
	public:
		explicit AcceleratorEmitter(std::wistream& in);

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer) const;
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		DWORD ResourceSize;
		AcceleratorTable Table;
	};

}
