#pragma once

#include "SplineSpectrum.hpp"
#include "toJSON.hpp"
#include "Dimension.h"

namespace Diginstrument
{
template <typename T>
class Instrument
{
  public:
    void add(SplineSpectrum<T, 4> && spectrum)
    {
        spectra.push_back(std::move(spectrum));
    }

    const vector<SplineSpectrum<double, 4>> & getSpectra()
    {
        return spectra;
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
        return Diginstrument::JSONConverter::toJSON(name, dimensions, spectra).dump(spaces);
    }

    static Diginstrument::Instrument<double> fromJSON(json object)
    {
        Diginstrument::Instrument<double> res;
        res.name = object["name"];
        res.dimensions.reserve(object["dimensions"].size());
        for(const auto & d : object["dimensions"])
        {
            res.dimensions.push_back(Diginstrument::JSONConverter::dimensionFromJSON(d));
        }
        res.reserve(object["spectra"].size());
        for(const auto & s : object["spectra"])
        {
            res.add(Diginstrument::JSONConverter::spectrumFromJSON(s));
        }
        return res;
    }

    std::string name;
    std::vector<Diginstrument::Dimension> dimensions;

  private:
    std::vector<SplineSpectrum<T, 4>> spectra;
};
};