#pragma once

#include <vector>
#include <cmath>

#include "Extrema.hpp"

namespace Diginstrument
{
class Phase
{
  public:
    static void unwrapInPlace(std::vector<double> & phase)
    {
        constexpr double cutoff = M_PI;
        //calculate derivative
        std::vector<double> diff(phase.size()-1);
        for(int i = 1; i<phase.size(); i++)
        {
            diff[i-1]=phase[i]-phase[i-1];
            //prevent correction if distance would be less than the cutoff
            if(abs(diff[i-1])<cutoff)
            {
                diff[i-1] = 0;
            }
        }
        //normalize distance to integers
        for(auto & e : diff)
        {
            e/=2*M_PI;
            //rounding down at half-way point
            if(abs(fmod(e,1))<=0.5) 
            {
                e = floor(e);
            }
            else 
            {
                e = round(e);
            }
        }
        //accumulate the correction
        for(int i = 1; i<diff.size(); i++)
        {
            diff[i]+=diff[i-1];
        }
        //apply correction
        for(int i = 1; i<phase.size(); i++)
        {
            phase[i]-= (2*M_PI)*diff[i-1];
        }
    }
};
}