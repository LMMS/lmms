#pragma once

#include <vector>

#include "Spectrum.hpp"

template <typename T>
class PartialSet
{
  private:
    std::vector<std::vector<Diginstrument::Component<T>>> partials;
    std::vector<std::pair<std::string, T>> labels;
    unsigned int sampleRate;

  public:
    //default constructor
    PartialSet(){}
    //empty constructor
    PartialSet(const std::vector<std::pair<std::string, T>> & labels, unsigned int sampleRate): labels(labels), sampleRate(sampleRate){}
    //copy constructor
    PartialSet(const PartialSet<T> & other) : partials(other.partials), labels(other.labels), sampleRate(other.sampleRate){}
    //move constructor
    PartialSet(PartialSet<T> && other) : partials(std::move(other.partials)), labels(std::move(other.labels)), sampleRate(other.sampleRate){}
    //parameters by reference
    PartialSet(const std::vector<std::vector<Diginstrument::Component<double>>> & partials, const std::vector<std::pair<std::string, T>> & labels, unsigned int sampleRate)
        : partials(partials), labels(labels), sampleRate(sampleRate){}
    //move partials
    PartialSet(std::vector<std::vector<Diginstrument::Component<double>>> && partials, const std::vector<std::pair<std::string, T>> & labels, unsigned int sampleRate)
        : partials(std::move(partials)), labels(labels), sampleRate(sampleRate){}
    //move partials and labels
    PartialSet(std::vector<std::vector<Diginstrument::Component<double>>> && partials, std::vector<std::pair<std::string, T>> && labels, unsigned int sampleRate)
        : partials(std::move(partials)), labels(std::move(labels)), sampleRate(sampleRate){}


    void add(const std::vector<Diginstrument::Component<T>> & partial)
    {
        partials.push_back(partial);
    }

    void add(std::vector<Diginstrument::Component<T>> && partial)
    {
        partials.push_back(std::move(partial));
    }

    const std::vector<std::vector<Diginstrument::Component<T>>> & get() const
    {
        return partials;
    }

    std::vector<std::pair<std::string, T>> getLabels() const
    {
        return labels;
    }

    unsigned int getSampleRate() const
    {
        return sampleRate;
    }

    PartialSet<T> getSlice(unsigned int startFrame, unsigned int frames) const
    {
        //TODO: actually use sampleRate
        if(partials.size() == 0 || partials.front().size()==0 || startFrame>=partials.front().size()) return PartialSet<T>(this->labels, sampleRate);
        std::vector<std::vector<Diginstrument::Component<T>>> slice;
        for(const auto & p : this->partials)
        {
            if(p.size()<=startFrame+frames)
            {
                slice.emplace_back(p.begin()+startFrame, p.end());
            }
            else{ slice.emplace_back(p.begin()+startFrame, p.begin()+startFrame+frames); }
        }
        return PartialSet<T>(std::move(slice), this->labels, sampleRate);
    }
};