#pragma once


namespace llvm
{
	class ExecutionEngine;
}


namespace GCCompilation
{

	void PrepareGCData(llvm::ExecutionEngine& ee, std::vector<char>* sectiondata);

}

