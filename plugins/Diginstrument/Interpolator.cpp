#include "Interpolator.h"

std::vector<std::pair<float, float>> Diginstrument::Interpolator::getSpectrum(std::vector<float> coordinates)
{
    /*tmp*/
    std::vector<std::pair<float, float>> res;
    for(int i = 1; i<8; i+=2){
        res.push_back(std::make_pair(coordinates[0]*i, 1.0f/(float)i));
    }
    return res;
}

Diginstrument::Interpolator::Interpolator(){}
