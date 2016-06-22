#pragma once


class ThreadStringPool
{
public:
	~ThreadStringPool();

public:
	const char* AllocConcat(const char* s1, const char* s2);

private:
	std::vector<std::string *> Pool;
};

