#include "MultidimensionalNeighbourMap.hpp"

#include <iostream>


namespace Diginstrument{
class Spectrum
{
  public:
    const std::vector<std::pair<float, float>> & getComponents() const{
      return components;
    }

    Spectrum(std::vector<std::pair<float, float>> components) : components(components){};
    Spectrum(const Spectrum & other) : components(other.getComponents()){}

  private:
    std::vector<std::pair<float, float>> components;
};


class Interpolator
{
  public:
    std::vector<std::pair<float, float>> getSpectrum(std::vector<float> coordinates);

    void addSpectrum(const Spectrum & spectrum, std::vector<float> coordinates);
    void addSpectra(const std::vector<Spectrum> & spectra,  std::vector<std::vector<float>> coordinates);

    Interpolator();
  private:
    MultidimensionalNeighbourMap<float, Spectrum> data;
};
};