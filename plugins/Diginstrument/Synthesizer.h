#pragma once

#include <vector>
#include <math.h>
#include <algorithm>

namespace Diginstrument
{

struct Oscillator
{
  unsigned int position = 0;
};

class Synthesizer
{
public:
  std::vector<float> playNote(std::vector<std::pair<double, double>>, const unsigned int frames, const unsigned int offset);

  void static setSampleRate(const unsigned int sampleRate);

  Synthesizer();

private:
  static unsigned int sampleRate;
  static std::vector<float> sinetable;
  std::vector<Oscillator> bank;

  void static buildSinetable();
};
}; // namespace Diginstrument