//
// The Epoch Language Project
// Win32 EXE Generator
//
// Manifest resource compiler
//

#pragma once

// Forward declarations
class LinkWriter;

// Dependencies
#include "Resource Compiler/ResourceEmitter.h"
#include <vector>


namespace ResourceCompiler
{

	class ManifestEmitter : public ResourceEmitter
	{
	// Construction
	public:
		ManifestEmitter(std::wistream& in, const std::wstring& relativepath);

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer) const;
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		DWORD ResourceSize;
		std::wstring ManifestFileName;
		std::string ManifestContents;
	};

}
