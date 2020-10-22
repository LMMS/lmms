#pragma once

#include <vector>
#include <iterator>
#include <algorithm>

#include "../common/Intersection.hpp"

namespace Extrema
{
/**DEPRECATED**/
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
    //step while it sustains
    while(curr!=end && *curr==*prev)
    {
        curr++;
        prev++;
    }
    //entirely constant
    if(curr==end)
    {
        return std::make_pair(std::vector<unsigned int>{0},std::vector<unsigned int>{(unsigned int)std::distance(begin, end)-1});
    }
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
        //step next if sustains
        if(*curr-minimumDifference == *next )
        {
            next++;
            continue;
        }
        //maxima if prev<curr>next or prev<curr=curr+1...>next
        if (*curr - minimumDifference > *next && *curr - minimumDifference > *prev)
        {
            maxima.push_back(std::distance(begin, curr));
        }
        //minima if prev>curr<next or prev>curr=curr+1...<next
        if (*curr + minimumDifference < *next && *curr + minimumDifference < *prev)
        {
            minima.push_back(std::distance(begin, curr));
        }
        prev = curr;
        curr = next;
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

/* Extrema locator based on numerical differentiation approximation*/
class Differential
{
public:

    class CriticalPoint
    {
      public:
        enum PointType
        {
            minimum = 0,
            maximum = 1,
            falling = 2,
            rising = 3
        };

        PointType pointType;
        double x;
        unsigned int index;

        bool operator<(const CriticalPoint & other) const
        {
            return this->x<other.x;
        }

        CriticalPoint(PointType pointType, double x, unsigned int index) : pointType(pointType), x(x), index(index) {}
        CriticalPoint(const CriticalPoint & other) : pointType(other.pointType), x(other.x), index(other.index) {}
    };

    template <class Iterator>
    static std::vector<std::pair<double, double>> makeFirstCentralDerivative(Iterator begin, Iterator end)
    {
        std::vector<std::pair<double, double>> res;
        res.reserve(std::distance(begin,end)-2);
        Iterator fn = begin+1;
        while(fn != end-1)
        {
            const auto & nL = *(fn-1);
            const auto & nR = *(fn+1);
            res.emplace_back( std::make_pair((*fn).first, (nR.second-nL.second) / ((nR.first-nL.first))) );
            fn++;
        }
        return res;
    }

    //TODO: changed from central; might have messed up peak detection
    template <class Iterator>
    static std::vector<std::pair<double, double>> makeFirstDerivative(Iterator begin, Iterator end)
    {
        std::vector<std::pair<double, double>> res;
        res.reserve(std::distance(begin,end)-1);
        Iterator fn = begin;
        while(fn != end-1)
        {
            const auto & nL = *(fn);
            const auto & nR = *(fn+1);
            res.emplace_back( std::make_pair(nL.first, nR.second-nL.second));
            fn++;
        }
        return res;
    }

    template <class Iterator>
    static std::vector<std::pair<double, double>> makeSecondDerivative(Iterator begin, Iterator end)
    {
        std::vector<std::pair<double, double>> res;
        res.reserve(std::distance(begin,end)-2);
        Iterator fn = begin+1;
        while(fn != end-1)
        {
            const auto & nL = *(fn-1);
            const auto & nR = *(fn+1);
            //TODO: deltaT is non-uniform!
            const auto deltaTSquare = (((nR.first-nL.first)) * ((nR.first-nL.first)))/4.0;
            res.emplace_back( std::make_pair((*fn).first, (nR.second-2*((*fn).second)+nL.second) / deltaTSquare ) );
            fn++;
        }
        return res;
    }

    template <class Iterator>
    static std::vector<CriticalPoint> maxima(Iterator begin, Iterator end, double minProminence = 0, double minDistance = 0)
    {
        std::vector<std::pair<double, double>> d1 = makeFirstDerivative(begin, end);
        std::vector<CriticalPoint> res;
        for(int i = 0; i<d1.size()-1; i++)
        {
            if(d1[i].second > 0 && d1[i+1].second < 0 && abs(d1[i].second-d1[i+1].second) > minProminence)
            {
                //local maximum
                const double x = Intersection::X<double>(d1[i].first, d1[i].second, d1[i+1].first, d1[i+1].second);
                //if too close to last peak
                if(!res.empty() && abs(res.back().x-d1[i+1].first) < minDistance)
                {
                    //if new is higher, replace last
                    if((begin+i+1)->second > (begin+res.back().index)->second)
                    {
                        res.back().index = i+1;
                        res.back().x = x;
                    }
                //if far enough
                }
                else
                {
                    //insert new maximum
                    res.emplace_back(CriticalPoint::PointType::maximum, x, i+1);
                }              
            }
        }
        return res;
    }

    /* Find approximations of first and second order critical points (extrema and inflexion points)
        The result contains both orders, ordered by x */
    template <class Iterator>
    static std::vector<CriticalPoint> intermixed(Iterator begin, Iterator end, int order = 1)
    {
        std::vector<std::pair<double, double>> d1 = makeFirstDerivative(begin, end);
        std::vector<std::pair<double, double>> d2 = makeSecondDerivative(begin, end);
        std::vector<CriticalPoint> res;
        //search for local extrema
        for(int i = 0; i<d1.size()-1; i++)
        {
            if(d1[i].second > 0 && d1[i+1].second < 0)
            {
                //local maximum
                const double x = Intersection::X<double>(d1[i].first, d1[i].second, d1[i+1].first, d1[i+1].second);
                res.emplace_back(CriticalPoint::PointType::maximum, x, i+1);
                continue;
            }
            if(d1[i].second < 0 && d1[i+1].second > 0)
            {
                //local minimum
                const double x = Intersection::X<double>(d1[i].first, d1[i].second, d1[i+1].first, d1[i+1].second);
                res.emplace_back(CriticalPoint::PointType::minimum, x, i+1);
                continue;
            }
        }
        //search for inflexion points
        if(order>1)
        {
            for(int i = 0; i<d2.size()-1; i++)
            {
                if(d2[i].second > 0 && d2[i+1].second < 0)
                {
                    //rising point
                    const double x = Intersection::X<double>(d2[i].first, d2[i].second, d2[i+1].first, d2[i+1].second);
                    res.emplace_back(CriticalPoint::PointType::rising, x, i+1);
                }
                if(d2[i].second < 0 && d2[i+1].second > 0)
                {
                    //falling point
                    const double x = Intersection::X<double>(d2[i].first, d2[i].second, d2[i+1].first, d2[i+1].second);
                    res.emplace_back(CriticalPoint::PointType::falling, x, i+1);
                }
            }
        }
        //sort by x
        std::sort(res.begin(), res.end());

        return res;
    }
};
}; // namespace Extrema
