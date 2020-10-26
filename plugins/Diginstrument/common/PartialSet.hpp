#pragma once

#include <vector>

#include "Spectrum.hpp"

template <typename T>
class PartialSet
{
  private:
    std::vector<std::vector<Diginstrument::Component<T>>> partials;
    std::vector<std::pair<std::string, T>> labels;

  public:
    PartialSet(const std::vector<std::pair<std::string, T>> & labels): labels(labels) {}
    PartialSet(std::vector<std::vector<Diginstrument::Component<double>>> && partials, std::vector<std::pair<std::string, T>> && labels)
        : partials(std::move(partials)), labels(std::move(labels)) {}

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
};