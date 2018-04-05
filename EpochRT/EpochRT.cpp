// EpochRT.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>
#include <intrin.h>

#include "StringPool.h"
#include "GC.h"


// TODO - thread safety
ThreadStringPool StringPool;


extern "C" void ERT_assert(bool flag)
{
	// TODO - better handling here
	if(!flag)
		exit(0xffffffff);
}

extern "C" void ERT_passtest()
{
	std::cout << "TEST: pass" << std::endl;
}

extern "C" const char* ERT_string_concat(const char* s1, const char* s2)
{
	return StringPool.AllocConcat(s1, s2);
}

extern "C" bool ERT_string_compare(const char* s1, const char* s2)
{
	return (std::strcmp(s1, s2) == 0);
}

extern "C" const char* ERT_string_from_integer(int i)
{
	std::ostringstream convert;
	convert << i;

	return StringPool.Alloc(convert.str());
}

extern "C" void ERT_gc_init(unsigned segmentoffset)
{
	GC::Init(segmentoffset);
}

extern "C" void ERT_gc_collect_strings()
{
	GC::CollectStrings(&StringPool, _ReturnAddress());
}


extern "C" short ERT_integer16_from_integer(int in)
{
	return static_cast<short>(in);
}


extern "C" void ERT_print(const char* out)
{
	std::cout << out << std::endl;
}


extern "C" void ERT_buffer_alloc(char** outbuffer, unsigned size)
{
	*outbuffer = new char[size];
}


extern "C" char EpochLib_SubstrCharDirect(const char* p, int pos)
{
	return p[pos];
}

extern "C" const char* EpochLib_StrPointer(const char* s)
{
	return s;
}

extern "C" const char* EpochLib_SubstrDirect(const char* p, int pos, int len)
{
	std::string s(p + pos, len);
	return StringPool.Alloc(s);
}

extern "C" bool ERT_cmdlineisvalid()
{
	return true;
}

extern "C" unsigned ERT_cmdlinegetcount()
{
	return 1;
}

extern "C" const char* ERT_cmdlineget(unsigned index)
{
	return "";
}

extern "C" const char* ERT_substring_length(const char* str, unsigned pos, unsigned length)
{
	std::string s(str + pos, length);
	return StringPool.Alloc(s);
}

extern "C" void ERT_write_buffer_real(char* buffer, float value)
{
}

extern "C" char ERT_subchar(const char* str, unsigned pos)
{
	return str[pos];
}

extern "C" const char* ERT_widenfromptr(const char* p)
{
	return p;
}

extern "C" const char* ERT_string_unescape(const char* in)
{
	return in;
}

extern "C" const char* ERT_string_narrow(const char* p)
{
	return p;
}

extern "C" const char* ERT_substring_nolength(const char* str, unsigned pos)
{
	std::string s(str + pos);
	return StringPool.Alloc(s);
}

extern "C" float ERT_string_to_real(const char* p)
{
	return 0.0f;
}

extern "C" void ERT_write_buffer_string(char* buffer, unsigned pos, const char* str)
{
}

extern "C" void ERT_write_buffer(char* buffer, unsigned pos, char value)
{
}

extern "C" void ERT_write_buffer_multiple()
{
}

extern "C" const char* ERT_real_to_string(float value)
{
	return "";
}

extern "C" const char* ERT_buffer_copy(const char* buffer)
{
	return "";
}

extern "C" int ERT_string_compare_notequal(const char* a, const char* b)
{
	return lstrcmpA(a, b);
}

extern "C" int ERT_string_to_integer(const char* str)
{
	return 0;
}


