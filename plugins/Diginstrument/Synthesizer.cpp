#include "Synthesizer.h"

#define DEFAULT_SAMPLE_RATE 48000

unsigned int Diginstrument::Synthesizer::sampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(const std::vector<std::pair<float, float>> components, const unsigned int frames, const unsigned int offset){
    std::vector<float> res(frames);
    const unsigned int tableSize = sinetable.size();
    for(auto component : components){
        const unsigned int step = component.first * (tableSize / (float)sampleRate);
        unsigned int pos = (offset * step) % tableSize;
        for(int i = 0; i<frames; i++){
            res[i] += sinetable[pos] * component.second;
            pos = (int)round((pos + step)) % tableSize;
        }
    }
    return res;
}

Diginstrument::Synthesizer::Synthesizer()
{
    if(Diginstrument::Synthesizer::sinetable.size()<=0){
        if( Diginstrument::Synthesizer::sampleRate <= 0 ){setSampleRate( DEFAULT_SAMPLE_RATE );}
        buildSinetable();
    }
}

void Diginstrument::Synthesizer::setSampleRate(const unsigned int sampleRate)
{
    if(Diginstrument::Synthesizer::sampleRate == sampleRate) return;
    Diginstrument::Synthesizer::sampleRate = sampleRate;
    Diginstrument::Synthesizer::buildSinetable();
}

void Diginstrument::Synthesizer::buildSinetable()
{
    unsigned int tableSize = /*TODO*/ 480000;
    std::vector<float> tmp(tableSize);
    for(int i = 0; i<tableSize; i++){
        tmp[i] = (float)sin(((double)i / (double)tableSize) * M_PI * 2.0);
    }
    Diginstrument::Synthesizer::sinetable = tmp;
}