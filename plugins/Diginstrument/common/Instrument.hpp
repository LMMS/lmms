#pragma once

#include "SplineSpectrum.hpp"
#include "PartialSet.hpp"
#include "toJSON.hpp"
#include "Dimension.h"

namespace Diginstrument
{
template <typename S, typename T>
class Instrument
{
  public:
    void add(S && spectrum)
    {
        spectra.push_back(std::move(spectrum));
    }

    void add(PartialSet<double> && partialSet)
    {
        partialSets.push_back(std::move(partialSet));
    }

    const vector<S> & getSpectra() const
    {
        return spectra;
    }

    const vector<PartialSet<T>> & getPartialSets() const
    {
        return partialSets;
    }

    void reserve(unsigned int n)
    {
        spectra.reserve(n);
    }

    void clear()
    {
        spectra.clear();
    }

    std::string toString(unsigned int spaces)
    {
        return Diginstrument::JSONConverter::toJSON(name, dimensions, spectra, partialSets).dump(spaces);
    }

    static Diginstrument::Instrument<S, T> fromJSON(json object)
    {
        Diginstrument::Instrument<S, T> res;
        res.name = object["name"];
        res.dimensions.reserve(object["dimensions"].size());
        for(const auto & d : object["dimensions"])
        {
            res.dimensions.push_back(Diginstrument::JSONConverter::dimensionFromJSON(d));
        }
        
        res.reserve(object["spectra"].size());
        for(const auto & s : object["spectra"])
        {
            res.add(Diginstrument::JSONConverter::splineFromJSON(s));
        }
        res.reserve(object["partial_sets"].size());
        for(const auto & p : object["partial_sets"])
        {
            res.add(Diginstrument::JSONConverter::partialSetFromJSON(p));
        }
        return res;
    }

    std::string name;
    std::vector<Diginstrument::Dimension> dimensions;

  private:
    std::vector<S> spectra;
    std::vector<PartialSet<T>> partialSets;
};
};