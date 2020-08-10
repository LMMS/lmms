#pragma once

#include "MultidimensionalNeighbourMap.hpp"
#include "../common/Spectrum.hpp"
#include "../common/SplineSpectrum.hpp"
#include "../common/Interpolation.hpp"
#include "../common/PeakMatcher.h"

#include <deque>
#include <algorithm>

namespace Diginstrument
{
template <typename T, class S>
class Interpolator
{
public:
  //TODO: references?/rvalues?
  std::vector<Component<T>> getSpectrum(const std::vector<T> &coordinates);

  void clear();

  void setDimensions(const std::vector<std::pair<std::string, bool>> & dimensions);

  void addSpectrum(const S &spectrum, std::vector<T> coordinates);
  void addSpectra(const std::vector<S> &spectra, std::vector<std::vector<T>> coordinates);

  static S interpolateSpectra(const S & left, const S & right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting = false);

  Interpolator() {}

private:
  MultidimensionalNeighbourMap<T, S> data;
  static constexpr T frequencyStep = 0.001;
  std::vector<std::pair<std::string, bool>> dimensions;
};

template <typename T>
class Interpolator<T, SplineSpectrum<T, 4>>
{
public:
  static SplineSpectrum<T, 4> interpolateSpectra(SplineSpectrum<T, 4> left, SplineSpectrum<T, 4> right, const T &target, const T &leftLabel, const T &rightLabel, bool shifting = false);

  Interpolator() {}

private:
  //tmp
  constexpr static double maxFrequencyDistance = 0.2;

  static BSpline<T, 4> matchPieces(BSpline<T, 4> left, const BSpline<T, 4> right, T rightRatio);
  static PiecewiseBSpline<T, 4> consolidatePieces(PiecewiseBSpline<T, 4> & left, PiecewiseBSpline<T, 4> & right, T rightRatio);
};
}; // namespace Diginstrument