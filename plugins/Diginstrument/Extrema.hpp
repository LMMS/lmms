#pragma once

#include <vector>
#include <iterator>

namespace Extrema
{

/*Returns the indices of both local minima and maxima in an array*/
/*The minimumDifference is the minimum distance from both neighbours to be considered an extrema*/
/*O(n)*/
template <class Iterator>
std::pair<std::vector<unsigned int>, std::vector<unsigned int>> Both(Iterator begin, Iterator end, typename std::iterator_traits<Iterator>::value_type minimumDifference)
{
    std::vector<unsigned int> minima, maxima;
    Iterator prev = begin;
    Iterator curr = begin + 1;
    //is the first one minima or maxima?
    bool ascending = *prev < *curr;
    if (ascending)
    {
        minima.push_back(0);
    }
    else
    {
        maxima.push_back(0);
    }

    Iterator next = curr + 1;
    while (next != end)
    {
        if (*curr - minimumDifference > *next && *curr - minimumDifference > *prev)
        {
            maxima.push_back(std::distance(begin, curr));
        }
        if (*curr + minimumDifference < *next && *curr + minimumDifference < *prev)
        {
            minima.push_back(std::distance(begin, curr));
        }
        prev = curr++;
        next++;
    }
    //is the last one minima or maxima?
    ascending = *prev < *curr;
    if (ascending)
    {
        maxima.push_back(std::distance(begin, curr));
    }
    else
    {
        minima.push_back(std::distance(begin, curr));
    }

    return std::make_pair(minima, maxima);
}
}; // namespace Extrema
