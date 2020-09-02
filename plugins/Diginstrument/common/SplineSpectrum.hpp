#pragma once

#include "Spectrum.hpp"
#include "PiecewiseBSpline.hpp"

template <typename T, unsigned int D>
class SplineSpectrum : public Diginstrument::Spectrum<T>
{
public:
  SplineSpectrum(T label) : Diginstrument::Spectrum<T>(label), spline() {}
  SplineSpectrum(PiecewiseBSpline<T, D> &&spline) : Diginstrument::Spectrum<T>(0), spline(std::move(spline)) {}
  SplineSpectrum(const PiecewiseBSpline<T, D> &spline) : Diginstrument::Spectrum<T>(0), spline(spline) {}
  SplineSpectrum(PiecewiseBSpline<T, D> &&spline, T label) : Diginstrument::Spectrum<T>(label), spline(std::move(spline)) {}
  SplineSpectrum(const PiecewiseBSpline<T, D> &spline, T label) : Diginstrument::Spectrum<T>(label), spline(spline) {}

  std::vector<Diginstrument::Component<T>> getMatchables() const
  {
    //TMP: phase was present here; in case if phase is removed
    std::vector<Diginstrument::Component<T>> res;
    res.reserve(spline.getPeaks().size());
    for (std::vector<T> peak : spline.getPeaks())
    {
      res.emplace_back(Diginstrument::Component<T>{peak[0], 0, peak[1]});
    }
    return res;
  }

  std::vector<Diginstrument::Component<T>> getUnmatchables() const
  {
    //TODO: UNIMPLEMENTED: unmatchables
    //TMP: unmatchables not yet implemented
    return {};
  }

  std::vector<Diginstrument::Component<T>> getComponents(const T quality) const
  {
    //harmonics+inbetween
    //TODO: use quality
    //TODO: how to synthesize the residual signal?
    //TMP: only matchables; include 'unmatchables' (those that 'cant be shifted and stretched')
    return getMatchables();
  }

  Diginstrument::Component<T> operator[](T x) const
  {
    const auto p = spline[x];
    if (p.size() < 3)
    {
      return Diginstrument::Component<T>(0, 0, 0);
    }
    return Diginstrument::Component<T>(spline[x][0], spline[x][1], spline[x][2]);
  }

  const T getBegin() const
  {
    return spline.getBegin();
  }

  const T getEnd() const
  {
    return spline.getEnd();
  }

  PiecewiseBSpline<T, D> &getSpline()
  {
    return spline;
  }

  bool empty() const
  {
    return spline.getPeaks().size()==0;
  }

private:
  PiecewiseBSpline<T, D> spline;
  T label;
};
