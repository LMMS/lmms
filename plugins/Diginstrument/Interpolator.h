#pragma once

#include "MultidimensionalNeighbourMap.hpp"
#include "Spectrum.hpp"
#include "SplineSpectrum.hpp"

namespace Diginstrument
{
template <typename T, class S>
class Interpolator
{
public:
  //TODO: references?/rvalues?
  std::vector<std::pair<T, T>> getSpectrum(const std::vector<T> &coordinates);

  void addSpectrum(const S &spectrum, std::vector<T> coordinates);
  void addSpectra(const std::vector<S> &spectra, std::vector<std::vector<T>> coordinates);

  S linear(S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel);
  S linearShift(S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel);

  Interpolator() {}

private:
  MultidimensionalNeighbourMap<T, S> data;
  T frequencyStep;
};

template <typename T>
class Interpolator<T, SplineSpectrum<T>>
{
public:
  SplineSpectrum<T> getSpectrum(const std::vector<T> &coordinates);

  void addSpectrum(const SplineSpectrum<T> &spectrum, std::vector<T> coordinates);
  void addSpectra(const std::vector<SplineSpectrum<T>> &spectra, std::vector<std::vector<T>> coordinates);

  SplineSpectrum<T> linear(SplineSpectrum<T> &left, SplineSpectrum<T> &right, const T &target, const T &leftLabel, const T &rightLabel);
  SplineSpectrum<T> linearShift(SplineSpectrum<T> &left, SplineSpectrum<T> &right, const T &target, const T &leftLabel, const T &rightLabel);

  Interpolator() {}

private:
  MultidimensionalNeighbourMap<T, SplineSpectrum<T>> data;
  T frequencyStep = 0;
};
}; // namespace Diginstrument