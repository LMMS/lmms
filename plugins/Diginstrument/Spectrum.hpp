#pragma once

#include <vector>

namespace Diginstrument
{
template <typename T>
class Spectrum
{
public:
  virtual std::vector<std::pair<T, T>> getComponents(const T quality) const = 0;
};

template <typename T>
class NoteSpectrum : public Spectrum<T>
{
public:
  const std::vector<std::pair<T, T>> &getHarmonics() const
  {
    return harmonics;
  }
  const std::vector<std::pair<T, T>> &getStochastics() const
  {
    return stochastics;
  }
  std::vector<std::pair<T, T>> getComponents(const T quality) const
  {
    return /*TODO*/ harmonics;
  }

  T getLabel() const
  {
    return label;
  }

  NoteSpectrum(const T &label, const std::vector<std::pair<T, T>> &harmonics, const std::vector<std::pair<T, T>> &stohastics)
      : harmonics(harmonics), stochastics(stohastics), label(label) {}

private:
  std::vector<std::pair<T, T>> harmonics;
  std::vector<std::pair<T, T>> stochastics;
  T label;
};
} // namespace Diginstrument