#ifndef arrayOperations_hpp
#define arrayOperations_hpp

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include "../../common/source/MyBool.hpp"

template<class T>
std::vector<T> logicalIndex(const std::vector<T>& in, const std::vector<bool>& index)
{
	assert(in.size() == index.size());

	std::vector<T> ret;
	for (size_t i = 0; i != in.size(); ++i) {
		if (index[i]) {
			ret.push_back(in[i]);
		}
	}
	return ret;
}

template<class T>
std::vector<T> logicalIndex(const std::vector<T>& in, const std::vector<MyBool>& index)
{
	assert(in.size() == index.size());

	std::vector<T> ret;
	for (size_t i = 0; i != in.size(); ++i) {
		if (index[i]) {
			ret.push_back(in[i]);
		}
	}
	return ret;
}

// create a vector the size of index, where the elements are taken from in or set to init, depending on the value of index[i]
template<class T>
std::vector<T> spread(const std::vector<T>& in, const std::vector<bool>& index, const T& init = T())
{
	std::vector<T> ret(index.size(), init);
	typename std::vector<T>::const_iterator inIter = in.begin();
	for (size_t i = 0; i != index.size(); ++i) {
		if (index[i]) {
			assert(inIter != in.end());
			ret[i] = *inIter;
			++inIter;
		}
	}
	assert(inIter == in.end());
	return ret;
}

// create a vector the size of index, where the elements are taken from in or set to init, depending on the value of index[i]
template<class T>
std::vector<T> spread(const std::vector<T>& in, const std::vector<MyBool>& index, const T& init = T())
{
	std::vector<T> ret(index.size(), init);
	typename std::vector<T>::const_iterator inIter = in.begin();
	for (size_t i = 0; i != index.size(); ++i) {
		if (index[i]) {
			assert(inIter != in.end());
			ret[i] = *inIter;
			++inIter;
		}
	}
	assert(inIter == in.end());
	return ret;
}

template<class T>
std::vector<T> operator!(const std::vector<T>& in)
{
	std::vector<T> ret(in.size());
	for (size_t i = 0; i != in.size(); ++i) {
		ret[i] = !in[i];
	}
	return ret;
}

template<class T>
std::vector<std::pair<size_t, size_t> > runs(T beginIter, T endIter)
{
	std::vector<std::pair<size_t, size_t> > beginEnd;

	bool inRun = false;
	size_t begin = 0;
	size_t count = 0;
	for (T iter = beginIter; iter != endIter; ++iter) {
		if (*iter && !inRun) {
			inRun = true;
			begin = count;
		} else if (!*iter && inRun) {
			inRun = false;
			beginEnd.push_back(std::make_pair(begin, count));
		}
		++count;
	}
	if (inRun) {
		inRun = false;
		beginEnd.push_back(std::make_pair(begin, count));
	}

	return beginEnd;
}

template<class T>
bool all(T begin, T end)
{
	for (T iter = begin; iter != end; ++iter) {
		if (!(*iter)) {
			return false;
		}
	}
	return true;
}

template<class T>
bool any(T begin, T end)
{
	for (T iter = begin; iter != end; ++iter) {
		if (*iter) {
			return true;
		}
	}
	return false;
}

#endif
