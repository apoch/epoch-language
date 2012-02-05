//
// The Epoch Language Project
// Shared Library Code
//
// Template routine for clearing a map of handles which are no longer used
//

#pragma once


// Dependencies
#include <map>
#include <set>
#include <boost/unordered_map.hpp>


// TODO - roll this into a flat single implementation

template <typename HandleType, typename ResourceType>
void EraseDeadHandles(std::map<HandleType, ResourceType>& data, const std::set<HandleType>& livehandles)
{
	for(std::map<HandleType, ResourceType>::iterator iter = data.begin(); iter != data.end(); )
	{
		if(livehandles.find(iter->first) == livehandles.end())
			iter = data.erase(iter);
		else
			++iter;
	}
}

template <typename HandleType, typename ResourceType>
void EraseDeadHandles(boost::unordered_map<HandleType, ResourceType>& data, const std::set<HandleType>& livehandles)
{
	for(boost::unordered_map<HandleType, ResourceType>::iterator iter = data.begin(); iter != data.end(); )
	{
		if(livehandles.find(iter->first) == livehandles.end())
			iter = data.erase(iter);
		else
			++iter;
	}
}
