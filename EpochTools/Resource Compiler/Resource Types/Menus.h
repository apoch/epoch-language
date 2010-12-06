//
// The Epoch Language Project
// Win32 EXE Generator
//
// Menu resource compiler
//

#pragma once

// Forward declarations
class LinkWriter;

// Dependencies
#include "Resource Compiler/ResourceEmitter.h"
#include <list>


namespace ResourceCompiler
{

	struct MenuEntry
	{
		bool IsPopup;
		bool IsLast;
		WORD ID;
		std::wstring Text;
		std::list<MenuEntry> SubEntries;

		DWORD GetTotalSize() const;
		void LoadFromStream(std::wistream& in, std::list<MenuEntry>& entrylist);
		void Emit(LinkWriter& writer);
	};


	class MenuEmitter : public ResourceEmitter
	{
	// Construction
	public:
		explicit MenuEmitter(std::wistream& in);

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer);
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		DWORD ResourceSize;
		MenuEntry Root;
	};

}
