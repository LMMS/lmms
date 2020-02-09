#pragma once

#include "MultidimensionalNeighbourMap.hpp"

namespace Diginstrument{
class Spectrum
{
  public:
    virtual const std::vector<std::pair<float, float>> & getComponents() const{
      return components;
    }

    Spectrum(const std::vector<std::pair<float, float>> & components) : components(components){};
    Spectrum(const Spectrum & other) : components(other.getComponents()){}
    Spectrum(Spectrum && other): components(std::move(other.components)){}
    Spectrum(): components(){}

  private:
    std::vector<std::pair<float, float>> components;
};

class NoteSpectrum : public Spectrum
{ 
  public:
    const std::vector<std::pair<float, float>> & getHarmonics() const{
      return harmonics;
    }
    const std::vector<std::pair<float, float>> & getStochastics() const{
      return stochastics;
    }
    const std::vector<std::pair<float, float>> & getComponents() const{
      return /*TODO*/ harmonics;
    }

    float getLabel() const {
      return label;
    }

    NoteSpectrum(const float & label, const std::vector<std::pair<float, float>> & harmonics, const std::vector<std::pair<float, float>> & stohastics)
              : harmonics(harmonics), stochastics(stohastics), label(label){}

  private:
    std::vector<std::pair<float, float>> harmonics;
    std::vector<std::pair<float, float>> stochastics;
    float label;
};

template <typename S>
class Interpolator
{
  public:
    std::vector<std::pair<float, float>> getSpectrum(const std::vector<float> coordinates);

    void addSpectrum(const S & spectrum, std::vector<float> coordinates);
    void addSpectra(const std::vector<S> & spectra,  std::vector<std::vector<float>> coordinates);

    S linear(S & left, S & right, const float & target, const float & leftLabel, const float & rightLabel);
    S linearShift(S & left, S & right, const float & target, const float & leftLabel, const float & rightLabel);

    Interpolator();
  private:
    MultidimensionalNeighbourMap<float, S> data;
    float frequencyStep;
};
};