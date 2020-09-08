#pragma once

#include <string>

using namespace std;

namespace Diginstrument
{
class Dimension
{
  public:
    std::string name;
    int min, max;
    bool shifting;

    Dimension(std::string name, int min, int max, bool shifting = true) : name(name), min(min), max(max), shifting(shifting) {}
};
};