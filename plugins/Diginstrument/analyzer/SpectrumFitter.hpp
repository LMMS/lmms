#pragma once

#include <vector>

#include "SplineFitter.hpp"
#include "PointSelector.hpp"
#include "Extrema.hpp"
#include "../common/PiecewiseBSpline.hpp"
#include "../common/Spectrum.hpp"
#include "../common/Interpolation.hpp"

//tmp
#include <iostream>

//TODO: maybe: merge pieces for fitting if they are not long enough
/* Fits a PiecewiseBSpline of degree D to a set of points using the least squares method.*/
template <typename T, unsigned int D>
class SpectrumFitter
{
private:
    double indexDistanceBase;

    /*Returns a new vector containing the elements of the parameter vector that have the indices given in the parameter 'indices'.*/
    static std::vector<std::vector<T>> subvectorByIndices(const std::vector<std::vector<T>> &vector, const std::vector<unsigned int> &indices);

    static std::vector<std::vector<T>> addEndpointsToVector(const std::vector<T> start, const std::vector<T> end, std::vector<std::vector<T>> vector);
public:
    /*Fit a PiecewiseBSpline to the given points. Points should be ordered ascending by frequency*/
    PiecewiseBSpline<T, D> fit(const std::vector<std::vector<T>> & spectrum, const std::pair<std::vector<unsigned int>, std::vector<unsigned int>> & extrema);

    //TODO
    PiecewiseBSpline<T, D> peakValleyFit(const std::vector<std::vector<T>> & spectrum, const std::vector<Extrema::Differential::CriticalPoint> & extrema);

    //TODO
    std::vector<PiecewiseBSpline<T, D>> fit(const std::vector<std::vector<std::vector<T>>> & spectra,
                                            const std::vector<std::pair<std::vector<unsigned int>, std::vector<unsigned int>>> & extrema);

    /*Constructor.
     The parameter given is the base of the exponential expression used to select a subset of points to fit to between two local maxima.
     If the parameter is 1, all points will be selected. A parameter in the range of [1, 1.75] gives acceptable results. 1.25 is recommended for general use.*/
    SpectrumFitter(double indexDistanceBase) : indexDistanceBase(indexDistanceBase) {}
};

/*Fit to one spectrum with pre-calculated extrema*/
template <typename T, unsigned int D>
PiecewiseBSpline<T, D> SpectrumFitter<T, D>::fit(const std::vector<std::vector<T>> & spectrum, const std::pair<std::vector<unsigned int>, std::vector<unsigned int>> & extrema)
{    
    PiecewiseBSpline<T, D> res;
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
        }/*TMP: abort if bad*/ else {return res;}
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
        }/*TMP: abort if bad*/ else {return res;}
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
        }/*TMP: abort if bad*/ else {return res;}
        //res.add(SplineFitter<T, D>::fit(subvectorByIndices(spectrum, indices), indices.size()));
    }
    return res;
}

template <typename T, unsigned int D>
std::vector<PiecewiseBSpline<T, D>> SpectrumFitter<T, D>::fit(const std::vector<std::vector<std::vector<T>>> & spectra, const std::vector<std::pair<std::vector<unsigned int>, std::vector<unsigned int>>> & extrema)
{
    std::vector<PiecewiseBSpline<T, D>> res;
    res.reserve(spectra.size());
    for(int i = 0; i<spectra.size(); i++)
    {
        res.push_back(fit(spectra[i], extrema[i]));
    }
    return res;
}

template <typename T, unsigned int D>
std::vector<std::vector<T>> SpectrumFitter<T, D>::subvectorByIndices(const std::vector<std::vector<T>> &vector, const std::vector<unsigned int> &indices)
{
    std::vector<std::vector<T>> res;
    res.reserve(indices.size());
    for (auto &index : indices)
    {
        res.emplace_back(vector[index]);
    }
    return res;
}

template <typename T, unsigned int D>
std::vector<std::vector<T>> SpectrumFitter<T, D>::addEndpointsToVector(const std::vector<T> start, const std::vector<T> end, std::vector<std::vector<T>> vector)
{
    std::vector<std::vector<T>> res;
    res.reserve(vector.size()+2);
    res.push_back(std::move(start));
    std::move(std::begin(vector), std::end(vector), std::back_inserter(vector));
    res.push_back(std::move(end));
    return res;
}


template <typename T, unsigned int D>
PiecewiseBSpline<T, D> SpectrumFitter<T, D>::peakValleyFit(const std::vector<std::vector<T>> & spectrum, const std::vector<Extrema::Differential::CriticalPoint> & extrema)
{
    //TODO: re-think possible patterns and their continuity
    //TODO: too few or mismatched extrema (e.g. one minimum only)
    using PointType = Extrema::Differential::CriticalPoint::PointType;
    PiecewiseBSpline<T, D> res;
    //tmp
    if(extrema.size()==0) return res;
    auto it = extrema.begin();
    //accumulate segments where points.size()<D
    std::vector<std::vector<T>> points;
    //is first minimum or maximum?
    if(it->pointType == PointType::maximum)
    {
        //fit to [first, max]
        points = std::vector<std::vector<T>>(spectrum.begin(), spectrum.begin()+(it->index)+1);
        const auto Y = Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][2], spectrum[it->index][0], spectrum[it->index][2], spectrum[it->index+1][0], spectrum[it->index+1][2], spectrum[it->index+2][0], spectrum[it->index+2][2], it->x);
        points.push_back({it->x, 0, Y});
        if(points.size()>D){
            res.add(SplineFitter<T, D>::fit(points, points.size()));
            points.clear();
        }
    }
    else
    {
        //fit to [first, min, max]
        //tmp: disregard min
        //tmp: check
        if(extrema.size()>1)
            {
            points = std::vector<std::vector<T>>(spectrum.begin(), spectrum.begin()+((it+1)->index)+1);
            const auto Y = Interpolation::CubicLagrange(spectrum[(it+1)->index-1][0], spectrum[(it+1)->index-1][2], spectrum[(it+1)->index][0], spectrum[(it+1)->index][2], spectrum[(it+1)->index+1][0], spectrum[(it+1)->index+1][2], spectrum[(it+1)->index+2][0], spectrum[(it+1)->index+2][2], (it+1)->x);
            points.push_back({(it+1)->x, 0, Y});
            if(points.size()>D){
                res.add(SplineFitter<T, D>::fit(points, points.size()));
                points.clear();
            }
            it++;
            }
    }
    //TODO
    //TMP: no point selection, all points and set CPs
    while(it<extrema.end()-1)
    {
        //fit to [max, max]
        if(it->pointType == PointType::maximum && (it+1)->pointType == PointType::maximum)
        {
            //TODO
            //TODO: rethink accumulation. big problems can be caused here, like if i put in a point twice, the whole spline was useless
            const auto Y1 = Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][2], spectrum[it->index][0], spectrum[it->index][2], spectrum[it->index+1][0], spectrum[it->index+1][2], spectrum[it->index+2][0], spectrum[it->index+2][2], it->x);
            const auto Y2 = Interpolation::CubicLagrange(spectrum[(it+1)->index-1][0], spectrum[(it+1)->index-1][2], spectrum[(it+1)->index][0], spectrum[(it+1)->index][2], spectrum[(it+1)->index+1][0], spectrum[(it+1)->index+1][2], spectrum[(it+1)->index+2][0], spectrum[(it+1)->index+2][2], (it+1)->x);
            points.reserve(3-it->index+(it+1)->index);
            if(points.size()==0) points.push_back({it->x, 0, Y1});
            for(int i = it->index+1; i<=(it+1)->index; i++)
            {
                points.emplace_back(spectrum[i]);
            }
            points.push_back({(it+1)->x, 0, Y2});
            if(points.size()>D){
                res.add(SplineFitter<T, D>::fit(points, points.size()));
                points.clear();
            }
            it++;
            continue;
        }
        //fit to [max, min, max]
        if(it->pointType == PointType::maximum && (it+2)->pointType == PointType::maximum && (it+1)->pointType == PointType::minimum)
        {
            //TODO
            //tmp: disregard min
            const auto Y1 = Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][2], spectrum[it->index][0], spectrum[it->index][2], spectrum[it->index+1][0], spectrum[it->index+1][2], spectrum[it->index+2][0], spectrum[it->index+2][2], it->x);
            const auto Y2 = Interpolation::CubicLagrange(spectrum[(it+2)->index-1][0], spectrum[(it+2)->index-1][2], spectrum[(it+2)->index][0], spectrum[(it+2)->index][2], spectrum[(it+2)->index+1][0], spectrum[(it+2)->index+1][2], spectrum[(it+2)->index+2][0], spectrum[(it+2)->index+2][2], (it+2)->x);
            points.reserve(3-it->index+(it+2)->index);
            if(points.size()==0) points.push_back({it->x, 0, Y1});
            for(int i = it->index+1; i<=(it+2)->index; i++)
            {
                points.emplace_back(spectrum[i]);
            }
            points.push_back({(it+2)->x, 0, Y2});
            if(points.size()>D){
                res.add(SplineFitter<T, D>::fit(points, points.size()));
                points.clear();
            }
            it+=2;
            continue;
        }
        it++;
    }
    //is last minimum or maximum?
    if(extrema.back().pointType == PointType::maximum)
    {
        //fit to [max, end]
        const auto Y = Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][2], spectrum[it->index][0], spectrum[it->index][2], spectrum[it->index+1][0], spectrum[it->index+1][2], spectrum[it->index+2][0], spectrum[it->index+2][2], it->x);
        points.reserve(spectrum.size()-it->index+1);
        if(points.size()==0) points.push_back({it->x, 0, Y});
        for(int i = it->index+1; i<spectrum.size(); i++)
        {
            points.emplace_back(spectrum[i]);
        }
        if(points.size()>D){ res.add(SplineFitter<T, D>::fit(points, points.size())); }
    }
    else
    {
        //fit to [max, min, end]
        //TODO: is this correct?
        std::cout<<"UNINPLEMENTED: max,min,end - "<<(it-1)->x<<" "<<(it)->x<<" "<<spectrum.back().front()<<std::endl;
    }
    
    return res;
}