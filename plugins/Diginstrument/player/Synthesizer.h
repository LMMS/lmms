#pragma once

#include <vector>
#include <math.h>
#include <algorithm>
#include <set>

#include "../common/Spectrum.hpp"
#include "../common/PartialSet.hpp"
#include "../common/Interpolation.hpp"
#include "../common/PeakMatcher.h"

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
//TODO: is sample rate supposed to be provided here?
  std::vector<float> playNote(std::vector<Diginstrument::Component<double>> components, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate);
  std::vector<float> playNote(const Spectrum<double> & spectrum, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate);
  //tmp: new synthesis from scratch
  std::vector<float> playNote(const PartialSet<double> & slice, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate);

  void static setSampleRate(const unsigned int sampleRate);

  Synthesizer();

private:
  static unsigned int outSampleRate;
  static std::vector<float> sinetable;
  //std::vector<Component<double>> bank;
  std::vector<int> updateCounters;

  //tmp
  std::vector<unsigned int> bank;

  void static buildSinetable();
};
}; // namespace Diginstrument