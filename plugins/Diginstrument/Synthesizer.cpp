#include "Synthesizer.h"

#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_OSCILLATORS 5

//tmp
#include <iostream>

unsigned int Diginstrument::Synthesizer::sampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(const Spectrum<double> & startSpectrum, const Spectrum<double> & endSpectrum, const unsigned int frames, const unsigned int offset)
{
    //TODO: banks
    std::vector<float> res(frames, 0);
    const unsigned int tableSize = sinetable.size();
    auto components = startSpectrum.getComponents(0);
    auto endComponents = endSpectrum.getComponents(0);
    //sort components by amplitude, to mitigate jumps
    std::sort(components.begin(), components.end(), Component<double>::sortByAmplitudeDescending);
    std::sort(endComponents.begin(), endComponents.end(), Component<double>::sortByAmplitudeDescending);
    //this will "reset" the bank: unused oscillators are destroyed or necessary new ones are constructed
    bank.resize(components.size());
    int nextBank = 0;

    for (int k = 0; k<components.size(); k++)
    {
        //tmp
        if (components[k].amplitude < 0.001)
            continue;

        //TODO: starting phase , maybe if bank[i].position = -1?
        const auto component = components[k];
        //TODO: end and start components might differ in amount
        auto endComponent = Component<double>{component.frequency,0,0};
        if (k<endComponents.size())
            endComponent = endComponents[k];

         //tmp: start phase WIP
        if(bank[nextBank].position < 0)
        {
            //TODO: TMP: there was a phase of 90? ill use remainder, but this may be wrong
            bank[nextBank].position = std::round(((std::remainder(component.phase, 2*M_PI) + M_PI) / (M_PI * 2)) * (double)tableSize);
        }
       
        for (int i = 0; i < frames; i++)
        {
            //TODO: benchmark and optimize
            const double intFreq = Interpolation::Linear((double)0, component.frequency, (double)frames, endComponent.frequency, (double)i);
            const double intAmp = Interpolation::Linear((double)0, component.amplitude, (double)frames, endComponent.amplitude, (double)i);
            //calculate step size according to frequency
            const double step = intFreq * ((double)tableSize / (double)sampleRate);
            res[i] += sinetable[bank[nextBank].position] * intAmp;
            //tmp:debug
            //std::cout<<intFreq<< ", amp: "<<intAmp<<" @ "<<nextBank<<std::endl;
            //TODO: what is the best bank indexing schema?
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
        tmp[i] = (float)cos(((double)i / (double)tableSize) * M_PI * 2.0 - M_PI);
    }
    Diginstrument::Synthesizer::sinetable = tmp;
}