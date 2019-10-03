#include <vector>

namespace Diginstrument{
class Instrument
{
  public:
    std::vector<std::pair<float, float>> getSpectrum(std::vector<float> coordinates);
};
};