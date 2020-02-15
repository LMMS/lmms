#pragma once

#include <vector>

#include "SplineFitter.hpp"
#include "PointSelector.hpp"
#include "PiecewiseBSpline.hpp"
#include "Extrema.hpp"

//TODO: maybe: merge pieces for fitting if they are not long enough
/* Fits a PiecewiseBSpline of degree D to a set of points using the least squares method.*/
template <typename T, unsigned int D>
class SpectrumFitter
{
private:
    double indexDistanceBase;

    /*Returns a new vector containing the elements of the parameter vector that have the indices given in the parameter 'indices'.*/
    static std::vector<std::pair<T, T>> subvectorByIndices(const std::vector<std::pair<T, T>> &vector, const std::vector<unsigned int> &indices);

public:
    /*Fit a PiecewiseBSpline to the given points.*/
    PiecewiseBSpline<T, D> fit(std::vector<std::pair<T, T>> spectrum);

    /*Constructor.
     The parameter given is the base of the exponential expression used to select a subset of points to fit to between two local maxima.
     If the parameter is 1, all points will be selected. A parameter in the range of [1, 1.75] gives acceptable results. 1.25 is recommended for general use.*/
    SpectrumFitter(double indexDistanceBase) : indexDistanceBase(indexDistanceBase) {}
};

template <typename T, unsigned int D>
PiecewiseBSpline<T, D> SpectrumFitter<T, D>::fit(std::vector<std::pair<T, T>> spectrum)
{
    PiecewiseBSpline<T, D> res;
    //determine the indices of local extrema in the spectrum
    std::vector<T> values;
    values.reserve(spectrum.size());
    for (const std::pair<T, T> &point : spectrum)
    {
        values.emplace_back(point.second);
    }
    const auto extrema = Extrema::Both(values.begin(), values.end(), 0);
    //Approximate true maxima
    for (auto &index : extrema.second)
    {
        if (index == 0 || index == spectrum.size() - 1)
            continue;
        spectrum[index] = Approximation::Parabolic(spectrum[index - 1].first, spectrum[index - 1].second, spectrum[index].first, spectrum[index].second, spectrum[index + 1].first, spectrum[index + 1].second);
    }

    auto minima = extrema.first.begin();
    auto maxima = extrema.second.begin();
    //if the first extremity is a minimum, do a "half-fit"
    if (*minima < *maxima)
    { //TODO: maybe: vary control point amount based on distance and inbetween elements
        auto indices = PointSelector::selectIndices(*minima, *maxima, indexDistanceBase, true);
        //tmp
        if (indices.size() > D)
        {
            res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
        }
        //res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
        minima++;
    }

    //while there are extrema left to fit to
    while (minima != extrema.first.end() && maxima != extrema.second.end() - 1)
    {
        //fit to [maxima,maxima+1] with minima as "focus-point"
        auto indices = PointSelector::selectIndices(*maxima, *(maxima + 1), indexDistanceBase, *minima);
        //tmp
        if (indices.size() > D)
        {
            res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
        }
        //res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
        maxima++;
        minima++;
    }

    //if we didn't reach the last minimum, do a "half-fit"
    if (minima != extrema.first.end())
    {
        auto indices = PointSelector::selectIndices(*maxima, *minima, indexDistanceBase);
        //tmp
        if (indices.size() > D)
        {
            res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
        }
        //res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
    }
    return res;
}

template <typename T, unsigned int D>
std::vector<std::pair<T, T>> SpectrumFitter<T, D>::subvectorByIndices(const std::vector<std::pair<T, T>> &vector, const std::vector<unsigned int> &indices)
{
    std::vector<std::pair<T, T>> res;
    res.reserve(indices.size());
    for (auto &index : indices)
    {
        res.emplace_back(vector[index]);
    }
    return res;
}
