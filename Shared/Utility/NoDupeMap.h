//
// The Epoch Language Project
// Shared Library Code
//
// Helper routine for adding entries to a map and ensuring they are not duplicates
//

#pragma once


template<typename MapT>
void AddToMapNoDupe(MapT& themap, const typename MapT::value_type& value)
{
	if(themap.find(value.first) != themap.end())
		throw RecoverableException("Tried to add a duplicate entry to a map");

	themap.insert(value);
}

template<typename SetT>
void AddToSetNoDupe(SetT& theset, const typename SetT::value_type& value)
{
	if(theset.find(value) != theset.end())
		throw RecoverableException("Tried to add a duplicate entry to a set");

	theset.insert(value);
}

