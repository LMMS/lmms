//tmp
#include "Interpolator.h"

#include "PiecewiseBSpline.hpp"

template <typename T>
class SplineSpectrum : public Diginstrument::Spectrum
{
private:
    PiecewiseBSpline<T, 4> spline;
    float label;
public:
    SplineSpectrum(PiecewiseBSpline<T, 4> && spline) : spline(std::move(spline)){}

    std::vector<std::pair<T, T>> getHarmonics() const{
      return spline.getPeaks();
    }

    std::pair<T, T> operator[](T x)
    {
        return spline[x];
    }
};