#include "Synthesizer.h"

#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_OSCILLATORS 5

unsigned int Diginstrument::Synthesizer::sampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(std::vector<std::pair<double, double>> components, const unsigned int frames, const unsigned int offset)
{
    /*TODO:
            Time steps!
            */
    //float time = (frames + offset) / (float)sampleRate;
    //tmp
    //std::vector<float> tmp = coordinates;
    //tmp: constant "force"
    //tmp.push_back(0.8f);
    //tmp.push_back(time);

    std::vector<float> res(frames, 0);
    const unsigned int tableSize = sinetable.size();
    //sort components by amplitude, to mitigate jumps
    //TODO: after removing std::pair, add various comparators
    std::sort(components.begin(), components.end(), [](const auto &left, const auto &right) -> bool { return left.second >= right.second; });
    //this will "reset" the bank: unused oscillators are destroyed or necessary new ones are constructed
    bank.resize(components.size());
    int nextBank = 0;

    for (auto component : components)
    {
        //tmp
        if (component.second < 0.0001)
            continue;
        //calculate step size according to frequency
        const unsigned int step = component.first * (tableSize / (float)sampleRate);
        unsigned int pos = bank[nextBank].position;
        for (int i = 0; i < frames; i++)
        {
            res[i] += sinetable[bank[nextBank].position] * component.second;
            bank[nextBank].position = (int)round((bank[nextBank].position + step)) % tableSize;
        }
        nextBank++;
    }

    //TODO: do i actually need this? why does the waveform look the same?
    /*for(auto e : res){
        e=e/(float)spectrum.getComponents(10).size();
    }*/
    return res;
}

Diginstrument::Synthesizer::Synthesizer() : bank(DEFAULT_OSCILLATORS)
{
    if (Diginstrument::Synthesizer::sinetable.size() <= 0)
    {
        if (Diginstrument::Synthesizer::sampleRate <= 0)
        {
            setSampleRate(DEFAULT_SAMPLE_RATE);
        }
        buildSinetable();
    }
}

void Diginstrument::Synthesizer::setSampleRate(const unsigned int sampleRate)
{
    if (Diginstrument::Synthesizer::sampleRate == sampleRate)
        return;
    Diginstrument::Synthesizer::sampleRate = sampleRate;
    Diginstrument::Synthesizer::buildSinetable();
}

void Diginstrument::Synthesizer::buildSinetable()
{
    unsigned int tableSize = /*TODO*/ 480000;
    std::vector<float> tmp(tableSize);
    for (int i = 0; i < tableSize; i++)
    {
        tmp[i] = (float)sin(((double)i / (double)tableSize) * M_PI * 2.0);
    }
    Diginstrument::Synthesizer::sinetable = tmp;
}