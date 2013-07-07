//
// Stub - shell out to self-hosting code generator
//

#pragma once


namespace IRSemantics { class Program; }


namespace CompilerPasses
{

	bool GenerateCodeSelfHosted(IRSemantics::Program& program);

}

