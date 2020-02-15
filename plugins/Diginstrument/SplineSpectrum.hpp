#pragma once

#include "Spectrum.hpp"
#include "PiecewiseBSpline.hpp"

template <typename T>
class SplineSpectrum : public Diginstrument::Spectrum<T>
{
private:
  PiecewiseBSpline<T, 4> spline;
  T label;

public:
  SplineSpectrum(T label) : spline(), label(label) {}
  SplineSpectrum(PiecewiseBSpline<T, 4> &&spline) : spline(std::move(spline)), label(0) {}
  SplineSpectrum(const PiecewiseBSpline<T, 4> &spline) : spline(spline), label(0) {}
  SplineSpectrum(PiecewiseBSpline<T, 4> &&spline, T label) : spline(std::move(spline)), label(label) {}
  SplineSpectrum(const PiecewiseBSpline<T, 4> &spline, T label) : spline(spline), label(label) {}

  std::vector<std::pair<T, T>> getHarmonics() const
  {
    //TMP
    return spline.getPeaks();
  }

  std::vector<std::pair<T, T>> getComponents(const T quality) const
  {
    //harmonics+inbetween
    //TODO: how to synthesize the residual signal?
    //TMP
    return getHarmonics();
  }

  std::pair<T, T> operator[](T x) const
  {
    return spline[x];
  }

  const T getBegin() const
  {
    return spline.getBegin();
  }

  const T getEnd() const
  {
    return spline.getEnd();
  }
};
