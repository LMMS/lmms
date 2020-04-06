#include "Synthesizer.h"

#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_OSCILLATORS 5

//tmp
#include <iostream>
int samplecounter = 0;
#include "Interpolation.hpp"

unsigned int Diginstrument::Synthesizer::sampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(const Spectrum<double> & startSpectrum, const Spectrum<double> & endSpectrum, const unsigned int frames, const unsigned int offset)
{
    std::vector<float> res(frames, 0);
    const unsigned int tableSize = sinetable.size();
    auto components = startSpectrum.getComponents(0);
    //sort components by amplitude, to mitigate jumps
    //TODO: reevaluate this after including phase 
    //TODO: after removing std::pair, add various comparators
    std::sort(components.begin(), components.end(), Component<double>::sortByAmplitudeDescending);
    //this will "reset" the bank: unused oscillators are destroyed or necessary new ones are constructed
    bank.resize(components.size());
    int nextBank = 0;

    //tmp:debug
    /*if(components.size()>0)
    {
        //std::cout<<components.front().frequency<<" "<<components.front().phase<<std::endl;
        std::cout<<std::fixed<<components.front().phase<<" ";
    }
    else{
        //std::cout<<"0 0"<<std::endl;
        std::cout<<0<<" ";
    }*/
    /*if(offset>68500 && offset<69500)
    {
        if(components.size()>0)
        {
            std::cout<<std::fixed<<"("<<offset<<","<<components.front().phase<<"),";
        }
        else{
        std::cout<<std::fixed<<"("<<offset<<",0),";
        }
    }*/
    //~debug

    for (auto component : components)
    {
        //tmp
        if (component.amplitude < 0.001)
            continue;

        //calculate step size according to frequency
        const double step = component.frequency * ((double)tableSize / (double)sampleRate);
        //project phase into sineTable
        int pos = (int)(((std::remainder(component.phase, 2*M_PI)+M_PI) / (2*M_PI)) * (double)(tableSize));

        //tmp: time for freq instead of phase
        float time = (float)offset/44100.0;

        const double endPhase = endSpectrum[component.frequency].phase;

        if (component.amplitude>0.1) std::cout<<component.frequency<<std::endl;


        for (int i = 0; i < frames; i++)
        {
            //res[i] += sinetable[bank[nextBank].position] * component.amplitude;
            //bank[nextBank].position = (int)round((bank[nextBank].position + step)) % tableSize;
            //res[i] += sinetable[pos] * component.amplitude;
            //pos = (int)round((pos + step)) % tableSize;
            const double intPhase = Interpolation::Linear((double)0, component.phase, (double)frames, endPhase, (double)i);
            //pos = (int)(((std::remainder(intPhase, 2*M_PI)+M_PI) / (2*M_PI)) * (double)(tableSize));
            //phase + cos : this works
            //YOU WERE MISSING THE AMPTTTTTT!!!
            //res[i]+= cos(intPhase+M_PI * component.frequency);
            //freq + cos
            res[i]+= component.amplitude * cos(2*M_PI*time*component.frequency);
            time+= 1.0/44100.0;
            //std::cout<<"("<<i+offset<<","<<intPhase<<"),";
            //std::cout<<"("<<i+offset<<","<<res[i]<<"),";
        }
        //bank[nextBank].position = component.phase;
        //nextBank++;
        //tmp
        samplecounter++;
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