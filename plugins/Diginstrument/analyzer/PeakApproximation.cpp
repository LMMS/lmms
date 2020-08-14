#include "PeakApproximation.h"

std::vector<Extrema::Differential::CriticalPoint> Diginstrument::PeakAndValleyApproximation(const std::vector<Extrema::Differential::CriticalPoint> & cps)
{
    std::vector<Extrema::Differential::CriticalPoint> res;
    //tmp
    if(cps.size()<3) return res;
    //TMP: TODO: does it always start with an inflexion point? or can we miss an extremity?
    auto it = cps.begin()+1;
    while(it<cps.end()-1)
    {
        if(it->pointType == Extrema::Differential::CriticalPoint::PointType::maximum || it->pointType == Extrema::Differential::CriticalPoint::PointType::minimum)
        {
            res.emplace_back(*it);
        }
        //tmp: hidden peak approximation
        if(
            (it-1)->pointType == Extrema::Differential::CriticalPoint::PointType::falling
            && (it)->pointType == Extrema::Differential::CriticalPoint::PointType::rising
            && (it+1)->pointType == Extrema::Differential::CriticalPoint::PointType::falling
        )
        {
            res.emplace_back(Extrema::Differential::CriticalPoint::PointType::maximum, it->x, it->index);
        }
        it++;
    }
    return res;
}