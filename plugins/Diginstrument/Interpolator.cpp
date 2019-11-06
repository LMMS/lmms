#include "Interpolator.h"

template <typename S>
std::vector<std::pair<float, float>> Diginstrument::Interpolator<S>::getSpectrum(std::vector<float> coordinates)
{
    //tmp
    const float target = coordinates[0];

    std::vector<std::pair<float, float>> res;
    std::vector<std::vector<S>> neighbours = data.getNeighbours(coordinates);
    std::vector<S> workingArray;
    workingArray.reserve(neighbours.size());

    if(neighbours.size()==0){/*TODO: exception?*/ return std::vector<std::pair<float, float>>({});}

    /*process the bottom layer of the binary tree, where there could be unary nodes if there was an exact match or out-of-bounds*/
    for(auto & neighbour : neighbours){
        if(neighbour.size() == 1){
            workingArray.push_back(std::move(neighbour[0]));
            continue;
        }
        if(neighbour.size() == 2){
            workingArray.push_back(linear(neighbour[0], neighbour[1], target));
        }
    }
    /* now process the binary tree, until only the top result is left */
    while(workingArray.size()>1){
        std::vector<S> tmp;
        tmp.reserve(workingArray.size()/2);
        auto it = workingArray.begin();
        while(it!=workingArray.end()-1){
            tmp.push_back(linear(*it++, *it++, target));
        }
        workingArray = std::move(tmp);
    }

    return workingArray.front().getComponents();
}

template <typename S>
S Diginstrument::Interpolator<S>::linear(S & left, S & right, const float & target)
{
    float rightWeight = target / (right.getFundamentalFrequency() - left.getFundamentalFrequency());
    float leftWeight = 1.0f - rightWeight;
    float leftRatio = target / left.getFundamentalFrequency();
    float rightRatio = target / right.getFundamentalFrequency();
    //TODO: expand to stochastics
    std::vector<std::pair<float, float>> harmonics;
    for(auto & h : left.getHarmonics()){
        harmonics.push_back(std::make_pair(leftRatio * h.first, leftWeight * h.second));
    }
    for(auto & h : right.getHarmonics()){
        harmonics.push_back(std::make_pair(rightRatio * h.first, rightWeight * h.second));
    }

    /* accumulate energy in frequency-windows */
    std::sort(harmonics.begin(), harmonics.end());
    std::vector<std::pair<float, float>> accumulated;
    accumulated.reserve(harmonics.size()/2);
    auto it = harmonics.begin();
    float baseFrequency = it->first;
    while(it!=harmonics.end()){
        float accumulatedAmplitude = 0;
        while(it!=harmonics.end() && it->first <= baseFrequency + frequencyStep){
            accumulatedAmplitude+= it->second;
            it++;
        }
        accumulated.push_back(std::make_pair(baseFrequency, accumulatedAmplitude));
        baseFrequency = it->first;
    }

    return S(target, std::move(harmonics),{});
}


template <typename S>
Diginstrument::Interpolator<S>::Interpolator()
{
    //tmp: identity data
    this->frequencyStep = 0;
    data.insert(S(2000,{std::make_pair(2000, 1)}, {}), {2000});
    data.insert(S(20, {std::make_pair(20, 1)}, {}), {20});
    data.insert(S(40000, {std::make_pair(40000, 1)}, {}), {40000});
}

template class Diginstrument::Interpolator<Diginstrument::NoteSpectrum>;