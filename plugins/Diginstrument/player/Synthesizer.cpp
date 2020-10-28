#include "Synthesizer.h"

#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_OSCILLATORS 5

//TODO: refactor, trim/remove old code
//TODO: spline synthesis
//TODO: how to deal with quality?

//tmp
#include <iostream>

unsigned int Diginstrument::Synthesizer::outSampleRate = DEFAULT_SAMPLE_RATE;
std::vector<float> Diginstrument::Synthesizer::sinetable(0);

std::vector<float> Diginstrument::Synthesizer::playNote(std::vector<Diginstrument::Component<double>> components, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate)
{
    std::vector<float> res(frames, 0);
    //tmp
    //std::cout<<"components: "<<components.size()<<std::endl;
    int nextbank = 0;
    bank.resize(components.size());
    //ordering banks by amplitude can be a problem, if amps change close to eachother
    std::sort(components.begin(), components.end(), Component<double>::sortByAmplitudeDescending);
    for(auto & component : components)
    {
        //tmp
        if (component.amplitude < 0.001)
            continue;

        const unsigned int step = std::round(component.frequency * ((double)sinetable.size() / (double)sampleRate));
        //tmp: magnitude spectrogram: moved conversion from magnitude to amplitude here
        component.amplitude = sqrt( (component.frequency * component.amplitude) / (double)sampleRate);

        for (int i = 0; i < frames; i++)
        {
            res[i]+=component.amplitude * sinetable[bank[nextbank]];
            bank[nextbank] = (bank[nextbank] + step) % sinetable.size();
        }
        nextbank++;
    }
    return res;
}

std::vector<float> Diginstrument::Synthesizer::playNote(const PartialSet<double> & slice, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate)
{
    //TODO
    std::vector<float> res(frames, 0);
    //tmp: debug
    std::cout<<"partials: "<<slice.get().size()<<std::endl;
    for(const auto & partial : slice.get())
    {
        for(int i = 0; i<partial.size(); i++)
        {
            res[i] += cos(partial[i].phase) * partial[i].amplitude;
        }
    }
    return res;
}


//TODO: refactor spline synthesis
// std::vector<float> Diginstrument::Synthesizer::playNote(const Spectrum<double> & spectrum, const unsigned int frames, const unsigned int offset, const unsigned int & sampleRate)
// {
//     //TODO: banks
//     //TODO: reset banks?
//     //TODO: smapleRate only needed for mag to amp
//     //TODO: refactoring
//     std::vector<float> res(frames, 0);
//     const unsigned int tableSize = sinetable.size();
//     auto components = spectrum.getComponents(0);
//     //sort components by amplitude, to mitigate jumps
//     //TODO: is this needed with peak matching?
//     //std::sort(components.begin(), components.end(), Component<double>::sortByAmplitudeDescending);
//     //tmp
//     if(offset == 0)
//     {
//         bank.resize(0);
//     }
//     //TODO: try new algorithm
//     /*for matches 
//         update bank
//     for unmatcheds
//         new bank
//     for banks 
//         play
//     old banks?
//     */
//     //tmp
//      //tmp
//     constexpr int updateLimit = 2048;
//     std::set<Diginstrument::Component<double>> unmatched(components.begin(), components.end());
//     auto matches = Diginstrument::PeakMatcher::matchPeaks(components, this->bank);
//     //tmp: new algorithm
//     //update matched components
//     for (const auto & match : matches)
//     {
//         bank[match.right].frequency = components[match.left].frequency;
//         bank[match.right].amplitude = components[match.left].amplitude;
//         //tmp
//         if(updateCounters[match.right]>updateLimit)
//         {
//            bank[match.right].phase =  std::round(((std::remainder(components[match.left].phase, 2*M_PI) + M_PI) / (M_PI * 2)) * (double)tableSize);
//         }
//         unmatched.erase(components[match.left]);
//         updateCounters[match.right] = 0;
//     }
//     //create new components
//     for (const auto & component : unmatched)
//     {
//         //TODO: test/better start phase
//         const double startingStep = std::round(((std::remainder(component.phase, 2*M_PI) + M_PI) / (M_PI * 2)) * (double)tableSize);
//         bank.emplace_back(component.frequency, startingStep, component.amplitude);
//         updateCounters.push_back(0);
//     }
//     //TODO: dead banks
//     for (int i = 0; i<bank.size(); i++)
//     {   
//         auto & component = bank[i];
//         if (component.amplitude < 0.001)
//             continue;
//         if (updateCounters[i]>updateLimit)
//             component.amplitude = 0;
            
//         //tmp
//         double amp = (sqrt( (component.frequency * component.amplitude) / (double)sampleRate));

//         const double step = component.frequency * ((double)tableSize / (double)outSampleRate);
//         for (int i = 0; i < frames; i++)
//         {
//             res[i] += sinetable[component.phase] * /*component.amplitude*/ amp;
//             component.phase = (int)round((component.phase + step)) % tableSize;
//         }
//         updateCounters[i]+=frames;
//     }

//     //tmp: bank debug
//     /*for(int i = 0; i<bank.size();i++)
//     {
//         if(updateCounters[i]<updateLimit) std::cout<<std::fixed<<bank[i].frequency<<" "<<bank[i].amplitude<<" "<<(int)bank[i].phase<<" ";
//         else std::cout<<std::fixed<<bank[i].frequency<<" "<<0<<" "<<(int)bank[i].phase<<" ";
//     }
//     std::cout<<std::endl;*/

//     //TODO: do i actually need this? why does the waveform look the same?
//     /*for(auto e : res){
//         e=e/(float)spectrum.getComponents(10).size();
//     }*/
//     return res;
// }

Diginstrument::Synthesizer::Synthesizer() : bank(/*DEFAULT_OSCILLATORS*/)
{
    if (Diginstrument::Synthesizer::sinetable.size() <= 0)
    {
        if (Diginstrument::Synthesizer::outSampleRate <= 0)
        {
            setSampleRate(DEFAULT_SAMPLE_RATE);
        }
        buildSinetable();
    }
}

void Diginstrument::Synthesizer::setSampleRate(const unsigned int outSampleRate)
{
    if (Diginstrument::Synthesizer::outSampleRate == outSampleRate)
        return;
    Diginstrument::Synthesizer::outSampleRate = outSampleRate;
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