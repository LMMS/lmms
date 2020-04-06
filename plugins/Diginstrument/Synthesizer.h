#pragma once

#include <vector>
#include <math.h>
#include <algorithm>

#include "Spectrum.hpp"

namespace Diginstrument
{

struct Oscillator
{
  //tmp
  //unsigned int position = 0;
  double position = 0;
};

class Synthesizer
{
public:
  //std::vector<float> playNote(std::vector<std::pair<double, double>>, const unsigned int frames, const unsigned int offset);
  std::vector<float> playNote(const Spectrum<double> & startSpectrum, const Spectrum<double> & endSpectrum, const unsigned int frames, const unsigned int offset);

  void static setSampleRate(const unsigned int sampleRate);

  Synthesizer();

private:
  static unsigned int sampleRate;
  static std::vector<float> sinetable;
  std::vector<Oscillator> bank;

  void static buildSinetable();
};
}; // namespace Diginstrument