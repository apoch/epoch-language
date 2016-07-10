#pragma once


namespace Utility
{

	template<typename T>
	void AppendToBuffer(std::vector<char>* buffer, const T& data)
	{
		const char* pdata = reinterpret_cast<const char*>(&data);
		for(size_t i = 0; i < sizeof(T); ++i)
		{
			buffer->push_back(*pdata);
			++pdata;
		}
	}

}