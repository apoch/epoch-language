//
// The Epoch Language Project
// Shared Library Code
//
// Template routine for clearing a map of handles which are no longer used
//

#pragma once


template <typename MapType, typename SetType>
void EraseDeadHandles(MapType& data, const SetType& livehandles)
{
	for(typename MapType::iterator iter = data.begin(); iter != data.end(); )
	{
		if(livehandles.find(iter->first) == livehandles.end())
			iter = data.erase(iter);
		else
			++iter;
	}
}

template <typename MapType, typename SetType>
void EraseDeadHandles(MapType& data, const SetType& livehandles, const SetType& statichandles)
{
	for(typename MapType::iterator iter = data.begin(); iter != data.end(); )
	{
		if(livehandles.find(iter->first) == livehandles.end())
		{
			if(statichandles.find(iter->first) == statichandles.end())
				iter = data.erase(iter);
			else
				++iter;
		}
		else
			++iter;
	}
}

template <typename MapType, typename SetType>
void EraseAndDeleteDeadHandles(MapType& data, const SetType& livehandles)
{
	for(typename MapType::iterator iter = data.begin(); iter != data.end(); )
	{
		if(livehandles.find(iter->first) == livehandles.end())
		{
			delete iter->second;
			iter = data.erase(iter);
		}
		else
			++iter;
	}
}

