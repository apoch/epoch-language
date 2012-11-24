//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Pass for optimizing semantic IR contents
//

#include "pch.h"

#include "Compiler/Passes/Optimization.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"


void CompilerPasses::Optimize(IRSemantics::Program& program)
{
	const boost::unordered_map<StringHandle, IRSemantics::Function*>& irmap = program.GlobalNamespace.Functions.GetDefinitions();
	for(boost::unordered_map<StringHandle, IRSemantics::Function*>::const_iterator iter = irmap.begin(); iter != irmap.end(); ++iter)
		iter->second->HoistScopes();
}

