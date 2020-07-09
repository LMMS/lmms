#pragma once

#include <vector>
#include <math.h>
#include <algorithm>
#include <set>

#include "Spectrum.hpp"
#include "Interpolation.hpp"
#include "PeakMatcher.h"

namespace Diginstrument
{
/*TMP
class Oscillator : public Component<double>
{
  int position = -1;
  int updateCounter = 0;

public:
  Oscillator(double frequency, double amplitude, int position) :  Component(frequency, -1, amplitude), position(position){}
  Oscillator() : Component(0, -1, 0) {}
};*/

class Synthesizer
{
public:
  //std::vector<float> playNote(std::vector<std::pair<double, double>>, const unsigned int frames, const unsigned int offset);
  std::vector<float> playNote(const Spectrum<double> & spectrum, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate);

  void static setSampleRate(const unsigned int sampleRate);

  Synthesizer();

private:
  static unsigned int outSampleRate;
  static std::vector<float> sinetable;
  std::vector<Component<double>> bank;
  std::vector<int> updateCounters;

  void static buildSinetable();
};
}; // namespace Diginstrument