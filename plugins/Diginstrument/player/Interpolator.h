#pragma once

#include "MultidimensionalNeighbourMap.hpp"
#include "../common/Spectrum.hpp"
#include "../common/SplineSpectrum.hpp"
#include "../common/PartialSet.hpp"
#include "../common/Interpolation.hpp"
#include "../common/PeakMatcher.h"
#include "../common/Dimension.h"

#include <deque>
#include <algorithm>

namespace Diginstrument
{
template <typename T, class S>
class Interpolator
{
public:
  S getSpectrum(const std::vector<T> &coordinates);
  PartialSet<T> getPartials(const std::vector<T> &coordinates, unsigned int startFrame, unsigned int frames);

  void clear();

  void setDimensions(const std::vector<Dimension> & dimensions);
  const std::vector<Dimension> & getDimensions() const;

  void addSpectrum(const S &spectrum, std::vector<T> coordinates);
  //TODO: the order of coordinates might not be consistent!
  void addSpectra(const std::vector<S> &spectra);
  void addSpectra(const std::vector<S> &spectra, std::vector<std::vector<T>> coordinates);

  void addPartialSets(const std::vector<PartialSet<T>> & partialSets);

  Interpolator() {}

private:

  static BSpline<T, 4> mergePieces(BSpline<T, 4> left, const BSpline<T, 4> right, T rightRatio);
  static PiecewiseBSpline<T, 4> consolidatePieces(PiecewiseBSpline<T, 4> & left, PiecewiseBSpline<T, 4> & right, T rightRatio);
  
  MultidimensionalNeighbourMap<T, S> residual;
  MultidimensionalNeighbourMap<T, PartialSet<T>> partials;
  std::vector<Dimension> dimensions;

  S interpolateSpectra(const S & left, const S & right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting);
  PartialSet<T> interpolatePartialSet(const PartialSet<T> &left, const PartialSet<T> &right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting);
  
  NoteSpectrum<T> constructSpectrum(
    const NoteSpectrum<T> & left,
    const NoteSpectrum<T> & right,
    const T &target, const T &leftLabel, const T &rightLabel,
    const std::vector<Match> & matches,
    const std::vector<unsigned int> & unmatchedLeft,
    const std::vector<unsigned int> & unmatchedRight
    );

  SplineSpectrum<T, 4> constructSpectrum(
    const SplineSpectrum<T, 4> & left,
    const SplineSpectrum<T, 4> & right,
    const T &target, const T &leftLabel, const T &rightLabel,
    const std::vector<Match> & matches,
    const std::vector<unsigned int> & unmatchedLeft,
    const std::vector<unsigned int> & unmatchedRight
    );
};
}; // namespace Diginstrument