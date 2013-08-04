#include "pch.h"
#include "Utility/Threading/MachineInfo.h"


unsigned Threads::GetCPUCount()
{
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

