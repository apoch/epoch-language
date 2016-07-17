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

}

