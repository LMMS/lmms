#pragma once

#include <string>

using namespace std;

namespace Diginstrument
{
class Dimension
{
  public:
    std::string name;
    double min, max;
    bool shifting;
    double defaultValue, currentValue;

    Dimension(std::string name, double min, double max, bool shifting = true) : name(name), min(min), max(max), shifting(shifting), defaultValue(min), currentValue(min) {}
    Dimension(std::string name, double min, double max, bool shifting, float defaultValue) : name(name), min(min), max(max), shifting(shifting), defaultValue(defaultValue), currentValue(defaultValue) {}
};
};