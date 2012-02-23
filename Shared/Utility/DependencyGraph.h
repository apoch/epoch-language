// TODO - document

#pragma once


// Dependencies
#include <vector>
#include <utility>
#include <algorithm>
#include <set>


template <typename ValueT>
class DependencyGraph
{
// Dependency registration
public:
	void Register(const ValueT& value)
	{
		RegisteredValues.push_back(value);
	}

	void AddDependency(const ValueT& owner, const ValueT& dependency)
	{
		Dependencies.push_back(std::make_pair(owner, dependency));
	}

// Dependency resolution
public:
	std::vector<ValueT> Resolve() const
	{
		std::set<ValueT> written;
		std::vector<ValueT> ret;

		for(std::vector<ValueT>::const_iterator iter = RegisteredValues.begin(); iter != RegisteredValues.end(); ++iter)
		{
			std::vector<ValueT> dependencies;
			GetAllDependencies(*iter, dependencies);
			
			for(std::vector<ValueT>::const_iterator diter = dependencies.begin(); diter != dependencies.end(); ++diter)
			{
				if(written.find(*diter) != written.end())
					continue;

				written.insert(*diter);
				ret.push_back(*diter);
			}

			if(written.find(*iter) == written.end())
			{
				written.insert(*iter);
				ret.push_back(*iter);
			}
		}

		return ret;
	}

// Internal helpers
private:
	std::vector<ValueT> GetDirectDependencies(const ValueT& value) const
	{
		std::vector<ValueT> ret;
		for(std::vector<std::pair<ValueT, ValueT> >::const_iterator iter = Dependencies.begin(); iter != Dependencies.end(); ++iter)
		{
			if(iter->first == value)
				ret.push_back(iter->second);
		}

		return ret;
	}

	void GetAllDependencies(const ValueT& value, std::vector<ValueT>& curdeps) const
	{
		if(std::find(curdeps.begin(), curdeps.end(), value) != curdeps.end())
			throw RecoverableException("Circular dependency");

		std::vector<ValueT> directdeps = GetDirectDependencies(value);
		for(std::vector<ValueT>::const_iterator iter = directdeps.begin(); iter != directdeps.end(); ++iter)
		{
			GetAllDependencies(*iter, curdeps);
			curdeps.push_back(*iter);
		}
	}

// Internal storage
private:
	std::vector<ValueT> RegisteredValues;
	std::vector<std::pair<ValueT, ValueT> > Dependencies;
};

