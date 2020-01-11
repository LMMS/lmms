#include <vector>
#include <math.h>

#include "Interpolator.h"

namespace Diginstrument{
class Synthesizer {
  public:
    std::vector<float> playNote(const std::vector<float> coordinates, 
                                const unsigned int frames,
                                const unsigned int offset/*TODO: should be frames or seconds?*/);

    void static setSampleRate(const unsigned int sampleRate);

    Synthesizer();

  private:
    static unsigned int sampleRate;
    static std::vector<float> sinetable;

    void static buildSinetable();

    Diginstrument::Interpolator<NoteSpectrum> interpolator;
    /*TODO:
        time step
        */
};
};