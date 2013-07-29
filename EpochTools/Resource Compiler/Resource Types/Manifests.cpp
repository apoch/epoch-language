//
// The Epoch Language Project
// Win32 EXE Generator
//
// Manifest resource compiler
//

#include "pch.h"

#include "Resource Compiler/Resource Types/Manifests.h"

#include "Linker/LinkWriter.h"

#include "Utility/Strings.h"
#include "Utility/Files/Files.h"


using namespace ResourceCompiler;


//
// Construct and initialize an manifest resource wrapper
//
ManifestEmitter::ManifestEmitter(std::wistream& in, const std::wstring& relativepath)
{
	in.ignore();
	std::getline(in, ManifestFileName);
	ManifestContents = Files::LoadNarrow(narrow(relativepath + ManifestFileName));
}


//
// Write an accelerator resource to disk
//
void ManifestEmitter::Emit(LinkWriter& writer) const
{
	writer.EmitNarrowString(ManifestContents);
}

//
// Determine how much space the accelerator resource requires
//
DWORD ManifestEmitter::GetSize() const
{
	return ManifestContents.size();
}

