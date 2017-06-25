#include "stdafx.h"


extern "C" char* GetBufferPtr(char* in, unsigned offset)
{
	return in + offset;
}

extern "C" void WriteBufferPtrByte(char* b, unsigned offset, unsigned value)
{
	*(b + offset) = char(value);
}

extern "C" void WriteBufferPtrDword(char* b, unsigned offset, unsigned value)
{
	*reinterpret_cast<unsigned*>(b + offset) = value;
}

extern "C" int StrLen(const char* s)
{
	return strlen(s);
}

