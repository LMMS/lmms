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

Diginstrument::Interpolator::Interpolator()
{
    /*tmp: single component*/
    std::vector<Spectrum> tmp;
    std::vector<std::vector<float>> coords;
    tmp.push_back(Spectrum({std::make_pair(20000, 1)}));
    coords.push_back({20000, 1});
    tmp.push_back(Spectrum({std::make_pair(1200, 1)}));
    coords.push_back({1200, 1});
    tmp.push_back(Spectrum({std::make_pair(5100, 1)}));
    coords.push_back({5100, 1});
    tmp.push_back(Spectrum({std::make_pair(20, 1)}));
    coords.push_back({20, 1});
    tmp.push_back(Spectrum({std::make_pair(5000, 1)}));
    coords.push_back({5000, 1});
    //data.insert(std::move(tmp), coords);

    //tmp: debug and test
    std::cout<<"first layer of data:"<<std::endl;
    for(int i = 0; i<tmp.size();i++){
        data.insert(tmp[i], coords[i]);
    }
    Spectrum different({std::make_pair(22, 1)});
    Spectrum different2({std::make_pair(8, 1)});
    data.insert(different, std::vector<float>({20, 1}));
    data.insert(different2, std::vector<float>({20, 0.2f}));


    std::vector<std::vector<float>> testCoords;
    testCoords.push_back({666, 1});
    testCoords.push_back({6666, 1});
    testCoords.push_back({1200, 1});
    testCoords.push_back({22000, 1});
    testCoords.push_back({10, 1});
    testCoords.push_back({20, 0.5f});
    testCoords.push_back({22, 0.5f});
    for(auto coords : testCoords){
        std::cout<<"Neighbours of ["<<coords[0]<<"; "<<coords[1]<<"]:"<<std::endl;
        for(auto n : data.getNeighbours(coords)){
            for(auto comp: n.getComponents()){
                std::cout<<"    fr:"<<comp.first<<", amp: "<<comp.second<<std::endl;
            }
        }
    }
}
