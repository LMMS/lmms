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

  std::vector<Diginstrument::Component<T>> getHarmonics() const
  {
    //TMP
    std::vector<Diginstrument::Component<T>> res;
    for (std::vector<T> peak : spline.getPeaks())
    {
      res.emplace_back(Diginstrument::Component<T>{peak[0], peak[1], peak[2]});
    }
    return res;
  }

  std::vector<Diginstrument::Component<T>> getComponents(const T quality) const
  {
    //harmonics+inbetween
    //TODO: how to synthesize the residual signal?
    //TMP
    return getHarmonics();
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
