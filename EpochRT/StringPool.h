#pragma once


class ThreadStringPool
{
public:
	ThreadStringPool();
	~ThreadStringPool();

public:
	const char* AllocConcat(const char* s1, const char* s2);

	void MarkInUse(const char* s);
	void ToggleTraceBit();
	void FreeUnusedEntries();

private:
	struct PoolEntry
	{
		PoolEntry(uint32_t tf, std::string* s)
			: TraceFlag(tf),
			  String(s)
		{ }

		uint32_t TraceFlag;
		const std::string* String;
	};

	uint32_t TraceFlag;
	std::vector<PoolEntry> Pool;
};

