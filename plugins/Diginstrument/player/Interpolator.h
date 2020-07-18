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

  void addSpectrum(const S &spectrum, std::vector<T> coordinates);
  void addSpectra(const std::vector<S> &spectra, std::vector<std::vector<T>> coordinates);

  /*static*/ S linear(const S &left,const S &right, const T &target, const T &leftLabel, const T &rightLabel);
  /*static*/ S linearShift( S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel);

  Interpolator() {}

private:
  MultidimensionalNeighbourMap<T, S> data;
  T frequencyStep;
};

template <typename T>
class Interpolator<T, SplineSpectrum<T, 4>>
{
public:
  SplineSpectrum<T, 4> getSpectrum(const std::vector<T> &coordinates);

  void addSpectrum(const SplineSpectrum<T, 4> &spectrum, std::vector<T> coordinates);
  void addSpectra(const std::vector<SplineSpectrum<T, 4>> &spectra, std::vector<std::vector<T>> coordinates);

  static SplineSpectrum<T, 4> linear(SplineSpectrum<T, 4> left, SplineSpectrum<T, 4> right, const T &target, const T &leftLabel, const T &rightLabel, bool shifting = false);

  Interpolator() {}

private:
  MultidimensionalNeighbourMap<T, SplineSpectrum<T, 4>> data;
  //tmp
  constexpr static double maxFrequencyDistance = 0.2;
  T frequencyStep = 0;

  static BSpline<T, 4> matchPieces(BSpline<T, 4> left, const BSpline<T, 4> right, T rightRatio);
  static PiecewiseBSpline<T, 4> consolidatePieces(PiecewiseBSpline<T, 4> & left, PiecewiseBSpline<T, 4> & right, T rightRatio);
};
}; // namespace Diginstrument