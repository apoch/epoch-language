//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Pass for optimizing semantic IR contents
//

#pragma once


// Forward declarations
namespace IRSemantics { class Program; }


namespace CompilerPasses
{

	void Optimize(IRSemantics::Program& program);

}


