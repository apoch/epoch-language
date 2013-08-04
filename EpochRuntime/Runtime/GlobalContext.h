//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Definitions for accessing global execution contexts
//

#pragma once


namespace Runtime
{

	class ExecutionContext;


	void SetThreadContext(ExecutionContext* context);
	ExecutionContext* GetThreadContext();

}

