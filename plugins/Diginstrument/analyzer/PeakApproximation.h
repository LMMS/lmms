#pragma once

#include <vector>

#include "Extrema.hpp"

namespace Diginstrument
{
    std::vector<Extrema::Differential::CriticalPoint> PeakAndValleyApproximation(const std::vector<Extrema::Differential::CriticalPoint> & cps);
};