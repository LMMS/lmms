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

using namespace std;

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
    PiecewiseBSpline<T, D> peakFit(const std::vector<std::vector<T>> & spectrum, const std::vector<Extrema::Differential::CriticalPoint> & maxima);

    //TODO
    PiecewiseBSpline<T, D> fitToPartials(const std::vector<std::pair<T, T>> & spectrum, const std::vector<Diginstrument::Component<T>> & partials);

    //TODO
    std::vector<PiecewiseBSpline<T, D>> fit(const std::vector<std::vector<std::vector<T>>> & spectra,
                                            const std::vector<std::pair<std::vector<unsigned int>, std::vector<unsigned int>>> & extrema);

    /*Constructor.
     The parameter given is the base of the exponential expression used to select a subset of points to fit to between two local maxima.
     If the parameter is 1, all points will be selected. A parameter in the range of [1, 1.75] gives acceptable results. 1.25 is recommended for general use.*/
    SpectrumFitter(double indexDistanceBase) : indexDistanceBase(indexDistanceBase) {}
};

/*Fit to one spectrum with pre-calculated extrema*/
//TODO: refactor: why is there two fits? Whats different from peak-valley?
/** DEPRECATED **/
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
PiecewiseBSpline<T, D> SpectrumFitter<T, D>::peakFit(const std::vector<std::vector<T>> & spectrum, const std::vector<Extrema::Differential::CriticalPoint> & maxima)
{
    //TODO: did we hangle overlapping peaks anywhere? (rising-falling-rising or something like that)
    //TODO: should probably select points based on spline length and stuff
    //TODO:TMP:FIXME: we fit with as many CPs as data points
    //wait, maybe i should fit to all points, but not with points.size() cps?

    PiecewiseBSpline<T, D> res;
    //no peaks: return empty spline
    if(maxima.size()==0) return res;
    //accumulate too short segments
    //TODO: FIXME: this approach discards a peak if its segment was too short. However, this will rarely happen, and even rarer near significant peaks.
    //Could be a problem once we can detect overlapping peaks!
    std::vector<std::vector<T>> points;
    auto it = maxima.begin();

    //begin - first peak
    points = std::vector<std::vector<T>>(spectrum.begin(), spectrum.begin()+(maxima.begin()->index)+1);
    //add interpolated first peak
    //TMP: FIXME: if there are not enough points for cubic lagrange, just do linear
    if(it->index==0 || (it)->index+2 > spectrum.size()-1)
    {
        points.push_back({(it)->x, Interpolation::Linear(spectrum[(it)->index][0], spectrum[(it)->index][1], spectrum[(it)->index+1][0], spectrum[(it)->index+1][1], (it)->x)});
    }
    else
    {
        points.push_back({it->x, Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][1], spectrum[it->index][0], spectrum[it->index][1], spectrum[it->index+1][0], spectrum[it->index+1][1], spectrum[it->index+2][0], spectrum[it->index+2][1], it->x)});
    }
    //fit to points
    if(points.size()>D)
    {
        res.add(SplineFitter<T, D>::fit(points, /*TMP: we use all points*/ points.size()));
        points.clear();
    }
    //peak - peak
    while(it!=maxima.end()-1)
    {
        points.reserve(3-it->index+(it+1)->index + points.size());
        //if the previous segment was successfully fitted, add first peak again
        if(points.size()==0)
        {
            //TMP: FIXME: if there are not enough points for cubic lagrange, just do linear
            if((it)->index+2 > spectrum.size()-1)
            {
                points.push_back({(it)->x, Interpolation::Linear(spectrum[(it)->index][0], spectrum[(it)->index][1], spectrum[(it)->index+1][0], spectrum[(it)->index+1][1], (it)->x)});
            }
            else
            {
                points.push_back({it->x, Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][1], spectrum[it->index][0], spectrum[it->index][1], spectrum[it->index+1][0], spectrum[it->index+1][1], spectrum[it->index+2][0], spectrum[it->index+2][1], it->x)});
            }
        }
        //add inbetween points
        for(int i = it->index+1; i<=(it+1)->index; i++)
        {
            points.emplace_back(spectrum[i]);
        }
        //add second peak
        //TMP: FIXME: if there are not enough points for cubic lagrange, just do linear
        if((it+1)->index+2 > spectrum.size()-1)
        {
            points.push_back({(it+1)->x, Interpolation::Linear(spectrum[(it+1)->index][0], spectrum[(it+1)->index][1], spectrum[(it+1)->index+1][0], spectrum[(it+1)->index+1][1], (it+1)->x)});
        }
        else 
        {
            points.push_back({(it+1)->x, Interpolation::CubicLagrange(spectrum[(it+1)->index-1][0], spectrum[(it+1)->index-1][1], spectrum[(it+1)->index][0], spectrum[(it+1)->index][1], spectrum[(it+1)->index+1][0], spectrum[(it+1)->index+1][1], spectrum[(it+1)->index+2][0], spectrum[(it+1)->index+2][1], (it+1)->x)});
        }
        //fit to points
        if(points.size()>D)
        {
            res.add(SplineFitter<T, D>::fit(points, /*TMP: we use all points*/ points.size()));
            points.clear();
        }
        it++;
    }
    //last peak - end
    //if the previous segment was successfully fitted, add second peak again
    if(points.size()==0)
    {
        //TMP: FIXME: if there are not enough points for cubic lagrange, just do linear
        if((it)->index+2 > spectrum.size()-1)
        {
            points.push_back({(it)->x, Interpolation::Linear(spectrum[(it)->index][0], spectrum[(it)->index][1], spectrum[(it)->index+1][0], spectrum[(it)->index+1][1], (it)->x)});
        }
        else
        {
            points.push_back({it->x, Interpolation::CubicLagrange(spectrum[it->index-1][0], spectrum[it->index-1][1], spectrum[it->index][0], spectrum[it->index][1], spectrum[it->index+1][0], spectrum[it->index+1][1], spectrum[it->index+2][0], spectrum[it->index+2][1], it->x)});
        }
    }
    points.reserve(spectrum.size()-it->index+1);
    for(int i = it->index+1; i<spectrum.size(); i++)
    {
        points.emplace_back(spectrum[i]);
    }
    //fit to points
    if(points.size()>D) res.add(SplineFitter<T, D>::fit(points, /*TMP: we use all points*/ points.size()));

    return res;
}

template <typename T, unsigned int D>
PiecewiseBSpline<T, D> SpectrumFitter<T, D>::fitToPartials(const std::vector<std::pair<T, T>> & spectrum, const std::vector<Diginstrument::Component<T>> & partials)
{
    PiecewiseBSpline<T, D> res;
    if(partials.size()==0) return res; //TODO: fit to whole

    auto startPoint = spectrum.begin();
    for(const auto & partial : partials)
    {
        auto it = std::upper_bound(startPoint, spectrum.end(), partial,
            [](const Diginstrument::Component<T> & component, const std::pair<T, T> & pair)->bool
            {return component.frequency<pair.first;});
        //TODO: what if end?
        auto endPoint = it-1;
        cout<<partial.frequency<<" - "<<startPoint->first<<" "<<endPoint->first<<": "<<distance(startPoint, endPoint)+1<<" points"<<endl;
        //TODO: very rough algorithm, for now we just fit to whole
        if(distance(startPoint, endPoint)>D) startPoint = it;
    }
    //tmp
    vector<vector<double>> convertedRawSpectrum;
    convertedRawSpectrum.reserve(spectrum.size());
    for(auto & p : spectrum)
    {
        convertedRawSpectrum.push_back({p.first, p.second});
    }
    res.add(SplineFitter<T, D>::fit(convertedRawSpectrum, /*TMP: we use half of all points*/ convertedRawSpectrum.size()/2));
    //TODO: data reduction; areas of zeros or idk
    return res;
}
