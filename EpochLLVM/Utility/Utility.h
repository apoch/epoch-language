#pragma once


namespace Utility
{

	template<typename T, size_t TSize = sizeof(T)>
	void AppendToBuffer(std::vector<char>* buffer, const T& data)
	{
		const char* pdata = reinterpret_cast<const char*>(&data);
		for(size_t i = 0; i < TSize; ++i)
		{
			buffer->push_back(*pdata);
			++pdata;
		}
	}

	template<typename T, size_t TSize = sizeof(T)>
	void AppendToBuffer(std::vector<unsigned char>* buffer, const T& data)
	{
		const unsigned char* pdata = reinterpret_cast<const unsigned char*>(&data);
		for(size_t i = 0; i < TSize; ++i)
		{
			buffer->push_back(*pdata);
			++pdata;
		}
	}

	inline void AppendToBuffer(std::vector<char>* buffer, const char* data)
	{
		while(*data)
		{
			buffer->push_back(*data);
			++data;
		}
	}

	inline void AppendToBuffer(std::vector<unsigned char>* buffer, const char* data)
	{
		while(*data)
		{
			buffer->push_back(static_cast<unsigned char>(*data));
			++data;
		}
	}

}

