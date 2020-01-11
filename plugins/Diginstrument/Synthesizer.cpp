#include "Synthesizer.h"

//tmp
#include <iostream>

#define DEFAULT_SAMPLE_RATE 48000

unsigned int Diginstrument::Synthesizer::sampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(const std::vector<float> coordinates, const unsigned int frames, const unsigned int offset){
    /*TODO:
            Time steps!
            */
    float time = (frames + offset) / (float)sampleRate;
    //tmp
    std::vector<float> tmp = coordinates;
    //tmp: constant "force"
    tmp.push_back(0.8f);
    tmp.push_back(time);
    std::vector<std::pair<float, float>> components = interpolator.getSpectrum(tmp);

    std::vector<float> res(frames, 0);
    const unsigned int tableSize = sinetable.size();
    for(auto component : components){
        const unsigned int step = component.first * (tableSize / (float)sampleRate);
        unsigned int pos = (offset * step) % tableSize;
        for(int i = 0; i<frames; i++){
            res[i] += sinetable[pos] * component.second;
            pos = (int)round((pos + step)) % tableSize;
        }
    }
    //TODO: do i actually need this? why does the waveform look the same?
    for(auto e : res){
        e=e/(float)components.size();
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