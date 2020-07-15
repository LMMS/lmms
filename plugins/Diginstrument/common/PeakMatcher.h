#pragma once

#include <cmath>
#include <vector>
#include <unordered_set>
#include <algorithm>

#include "Spectrum.hpp"

namespace Diginstrument
{

struct Match
{
    unsigned int left;
    unsigned int right;
    double distance;

    Match(unsigned int left, unsigned int right, double distance): left(left), right(right), distance(distance){}
};

class PeakMatcher
{
private:
    static double calculateDistance(const Diginstrument::Component<double> &left, const Diginstrument::Component<double> &right);
public:
    static std::vector<Match> matchPeaks(const std::vector<Diginstrument::Component<double>> & leftComponents, const std::vector<Diginstrument::Component<double>> & rightComponents);
};
} // namespace Diginstrument