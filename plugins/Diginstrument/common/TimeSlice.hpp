#pragma once

#include <vector>

#include "Spectrum.hpp"

using namespace std;

namespace Diginstrument
{
template<typename T, unsigned int D>
class TimeSlice : public Diginstrument::Spectrum<T>
{
  public:
    //TMP
    vector<vector<Diginstrument::Component<T>>> partials;

    std::vector<Diginstrument::Component<T>> getMatchables() const
    {
        //TODO: tmp
        //TODO: are partials and other components matchable? or do they need separate matching?
        return partials.front();
    }

    std::vector<Diginstrument::Component<T>> getUnmatchables() const
    {
        //TODO: UNIMPLEMENTED: unmatchables
        //TMP: unmatchables not yet implemented
        return {};
    }

    std::vector<Diginstrument::Component<T>> getComponents(const T quality) const
    {
        //TODO: use quality
        //TMP
        return getMatchables();
    }

    Component<T> operator[](const T frequency) const
    {
        //TODO: tmp
        return {0,0,0};
    }

    vector<vector<Diginstrument::Component<T>>> getPartials(size_t start = 0, size_t end = 0) const
    {
        //TODO: excepton?
        if(start==end==0) return partials;
        if(start+end>partials.size()) return {};
        return vector<Component<T>>(partials.begin()+start, partials.begin()+start+end);
    }

    const vector<vector<Diginstrument::Component<T>>> & getPartials() const
    {
        return partials;
    }
    
    bool empty() const
    {
        return partials.empty();
    }

    //TODO: constuctors
    TimeSlice() : Diginstrument::Spectrum<T>({}) {}
    TimeSlice(const vector<vector<Diginstrument::Component<T>>> & partials, std::vector<std::pair<std::string, T>> labels) : Diginstrument::Spectrum<T>(labels), partials(partials) {}
};
}