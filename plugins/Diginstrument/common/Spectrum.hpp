#pragma once

#include <vector>

namespace Diginstrument
{

template <typename T>
struct  Component{
  T frequency;
  T phase;
  T amplitude;

  Component(const T frequency, const T phase, const T amplitude) : frequency(frequency), phase(phase), amplitude(amplitude) {}
  Component(const Component<T> & other) : frequency(other.frequency), phase(other.phase), amplitude(other.amplitude) {}
  Component() : frequency(0), phase(0), amplitude(0) {}

  //sorting functors
  static constexpr auto sortByAmplitudeDescending = [] (const auto &left, const auto &right) -> bool { return left.amplitude >= right.amplitude; };
  static constexpr auto sortByFrequencyAscending = [] (const auto &left, const auto &right) -> bool { return left.frequency < right.frequency; };

  bool operator<(const Component<T> & other) const
  {
    return frequency<other.frequency;
  }
};

template <typename T>
class Spectrum
{
public:
  virtual std::vector<Component<T>> getComponents(const T quality) const = 0;
  virtual Component<T> operator[](const T frequency) const = 0;
};

template <typename T>
class NoteSpectrum : public Spectrum<T>
{
public:
  const std::vector<Component<T>> &getHarmonics() const
  {
    return harmonics;
  }
  const std::vector<Component<T>> &getStochastics() const
  {
    return stochastics;
  }
  std::vector<Component<T>> getComponents(const T quality) const
  {
    return /*TODO*/ harmonics;
  }

  Component<T> operator[](const T frequency) const
  {
    return /*TODO*/ Component<T>(0,0,0);
  }

  T getLabel() const
  {
    return label;
  }

  NoteSpectrum(const T &label, const std::vector<Component<T>> &harmonics, const std::vector<Component<T>> &stohastics)
      : harmonics(harmonics), stochastics(stohastics), label(label) {}

private:
  std::vector<Component<T>> harmonics;
  std::vector<Component<T>> stochastics;
  T label;
};
} // namespace Diginstrument