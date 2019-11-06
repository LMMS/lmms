#include "MultidimensionalNeighbourMap.hpp"

#include <iostream>

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
    const std::vector<std::pair<float, float>> & getStohastics() const{
      return stohastics;
    }
    const std::vector<std::pair<float, float>> & getComponents() const{
      return /*TODO*/ harmonics;
    }

    float getFundamentalFrequency() const {
      return fundamentalFrequency;
    }

    NoteSpectrum(const float & fundamentalFrequency, const std::vector<std::pair<float, float>> & harmonics, const std::vector<std::pair<float, float>> & stohastics)
              : harmonics(harmonics), stohastics(stohastics), fundamentalFrequency(fundamentalFrequency){}

  private:
    std::vector<std::pair<float, float>> harmonics;
    std::vector<std::pair<float, float>> stohastics;
    float fundamentalFrequency;
};

template <typename S>
class Interpolator
{
  public:
    std::vector<std::pair<float, float>> getSpectrum(std::vector<float> coordinates);

    void addSpectrum(const S & spectrum, std::vector<float> coordinates);
    void addSpectra(const std::vector<S> & spectra,  std::vector<std::vector<float>> coordinates);

    S linear(S & left, S & right, const float & target);

    Interpolator();
  private:
    MultidimensionalNeighbourMap<float, S> data;
    float frequencyStep;
};
};