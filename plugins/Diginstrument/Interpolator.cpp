#include "Interpolator.h"

//tmp
#include <iostream>

template <typename T, class S>
std::vector<std::pair<T, T>> Diginstrument::Interpolator<T, S>::getSpectrum(const std::vector<T> &coordinates)
{
    std::vector<std::pair<T, T>> res;
    std::vector<std::vector<T>> labels;
    std::vector<std::vector<S>> possiblePairs = data.getNeighbours(coordinates, labels);

    if (possiblePairs.size() == 0)
    { /*TODO: exception?*/
        return std::vector<std::pair<T, T>>({});
    }

    //track current coordinate
    unsigned int currentCoordinate = coordinates.size() - 1;
    //until there is only one possible pair left
    //which means we interpolated on the last current coordinate
    while (possiblePairs.size() > 1 || currentCoordinate < 0)
    {
        //process all possible pairs into half the amount of possible pairs
        unsigned int currentNode = 0;
        std::vector<std::vector<S>> tmp;
        tmp.reserve(possiblePairs.size() / 2);
        for (int i = 0; i < possiblePairs.size();)
        {
            //if there is no next pair
            if (i + 1 >= possiblePairs.size())
            {
                //the new pair will be a single
                //if it was a single, we propagate it
                if (possiblePairs[i].size() == 1)
                {
                    tmp.push_back({possiblePairs[i].front()});
                    currentNode++;
                }
                //if it is a pair, we interpolate and make it a new single
                tmp.push_back({linear(possiblePairs[i].front(), possiblePairs[i].back(), coordinates[currentCoordinate], labels[currentCoordinate][currentNode], labels[currentCoordinate][currentNode + 1])});
                currentNode += 2;
                i += 1;
                continue;
            }
            //else, we make a new possible pair from two possible pairs
            tmp.push_back({});
            if (possiblePairs[i].size() == 1)
            {
                tmp.back().push_back(possiblePairs[i].front());
                currentNode++;
            }
            if (possiblePairs[i].size() == 2)
            {
                tmp.back().push_back(linear(possiblePairs[i].front(), possiblePairs[i].back(),
                                            coordinates[currentCoordinate], labels[currentCoordinate][currentNode], labels[currentCoordinate][currentNode + 1]));
                currentNode += 2;
            };
            if (possiblePairs[i + 1].size() == 1)
            {
                tmp.back().push_back(possiblePairs[i + 1].front());
                currentNode++;
            }
            if (possiblePairs[i + 1].size() == 2)
            {
                tmp.back().push_back(linear(possiblePairs[i + 1].front(), possiblePairs[i + 1].back(),
                                            coordinates[currentCoordinate], labels[currentCoordinate][currentNode], labels[currentCoordinate][currentNode + 1]));
                currentNode += 2;
            };
            i += 2;
        }
        possiblePairs = std::move(tmp);
        currentCoordinate -= 1;
    }
    //now process the last node, where we interpolate on frequency
    if (possiblePairs.front().size() == 1)
    {
        return possiblePairs.front().front().getComponents(0);
    }
    if (possiblePairs.front().size() == 2)
    {
        return linearShift(possiblePairs.front().front(), possiblePairs.front().back(),
                           coordinates.front(), labels[0][0], labels[0][1])
            .getComponents(0);
    }

    return std::vector<std::pair<T, T>>({});
}

template <typename T, class S>
S Diginstrument::Interpolator<T, S>::linearShift(S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel)
{
    std::cout << "shifting interpolation: " << target << ", between: " << leftLabel << ", " << rightLabel << std::endl;
    T rightWeight = target / (rightLabel - leftLabel);
    T leftWeight = 1.0f - rightWeight;
    T leftRatio = target / leftLabel;
    T rightRatio = target / rightLabel;
    //TODO: expand to stochastics
    std::vector<std::pair<T, T>> harmonics;
    for (auto &h : left.getHarmonics())
    {
        harmonics.push_back(std::make_pair(leftRatio * h.first, leftWeight * h.second));
    }
    for (auto &h : right.getHarmonics())
    {
        harmonics.push_back(std::make_pair(rightRatio * h.first, rightWeight * h.second));
    }

    //if one was empty, we dont need to accumulate
    if (harmonics.size() == left.getHarmonics().size() || harmonics.size() == right.getHarmonics().size())
    {
        return S(target, std::move(harmonics), {});
    }
    /* accumulate energy in frequency-windows */
    std::sort(harmonics.begin(), harmonics.end());
    std::vector<std::pair<T, T>> accumulated;
    accumulated.reserve(harmonics.size() / 2);
    auto it = harmonics.begin();
    T baseFrequency = it->first;
    while (it != harmonics.end())
    {
        T accumulatedAmplitude = 0;
        while (it != harmonics.end() && it->first <= baseFrequency + frequencyStep)
        {
            accumulatedAmplitude += it->second;
            it++;
        }
        accumulated.push_back(std::make_pair(baseFrequency, accumulatedAmplitude));
        //TODO: Debug: Invalid read of size 4
        baseFrequency = it->first;
    }

    return S(target, std::move(harmonics), {});
}

template <typename T, class S>
S Diginstrument::Interpolator<T, S>::linear(S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel)
{
    //TODO: think about the shifting of splinespectrum
    //TODO: MAYBE: write separate for differenct spectrum types
    std::cout << "interpolation: " << target << ", between: " << leftLabel << ", " << rightLabel << std::endl;
    T rightWeight = target / (rightLabel - leftLabel);
    T leftWeight = 1.0f - rightWeight;
    //TODO: expand to stochastics
    std::vector<std::pair<T, T>> harmonics;
    for (auto &h : left.getHarmonics())
    {
        harmonics.push_back(std::make_pair(h.first, leftWeight * h.second));
    }
    for (auto &h : right.getHarmonics())
    {
        harmonics.push_back(std::make_pair(h.first, rightWeight * h.second));
    }

    //if one was empty, we dont need to accumulate
    if (harmonics.size() == left.getHarmonics().size() || harmonics.size() == right.getHarmonics().size())
    {
        return S(target, std::move(harmonics), {});
    }
    /* accumulate energy in frequency-windows */
    std::sort(harmonics.begin(), harmonics.end());
    std::vector<std::pair<T, T>> accumulated;
    accumulated.reserve(harmonics.size() / 2);
    auto it = harmonics.begin();
    T baseFrequency = it->first;
    while (it != harmonics.end())
    {
        T accumulatedAmplitude = 0;
        while (it != harmonics.end() && it->first <= baseFrequency + frequencyStep)
        {
            accumulatedAmplitude += it->second;
            it++;
        }
        accumulated.push_back(std::make_pair(baseFrequency, accumulatedAmplitude));
        baseFrequency = it->first;
    }

    return S(target, std::move(harmonics), {});
}

// template <typename T, class S>
// Diginstrument::Interpolator<T, S>::Interpolator()
// {
//     //tmp: identity data
//     this->frequencyStep = 0; //??? whats this?
//     /*data.insert(S(2000,{std::make_pair(2000, 1), std::make_pair(4000, 0.5f), std::make_pair(6000, 0.33f), std::make_pair(8000, 0.25f)}, {}), {2000, 1, 0});
//     data.insert(S(20, {std::make_pair(20, 1), std::make_pair(40, 0.5f), std::make_pair(60, 0.33f), std::make_pair(80, 0.25f)}, {}), {20, 1, 0});
//     data.insert(S(40000, {std::make_pair(40000, 1)}, {}), {40000, 1, 0});*/

//     //tmp: decay point
//     /*data.insert(S(2000,{}, {}), {2000, 0, 0.5f});
//     data.insert(S(20, {}, {}), {20, 0, 0.5f});
//     data.insert(S(40000, {}, {}), {40000, 0, 0.5f});*/

//     //tmp: add more for more neighbours, but still not full neighbourhood
//     /*data.insert(S(2000,{}, {}), {2000, 1, 0.5f});
//     data.insert(S(20, {}, {}), {20, 1, 0.5f});
//     data.insert(S(40000, {}, {}), {40000, 1, 0.5f});*/
// }

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::addSpectrum(const S &spectrum, std::vector<T> coordinates)
{
    //TODO:test, check, better
    data.insert(spectrum, coordinates);
}

template <typename T>
void Diginstrument::Interpolator<T, SplineSpectrum<T>>::addSpectrum(const SplineSpectrum<T> &spectrum, std::vector<T> coordinates)
{
    //TODO:test, check, better
    data.insert(spectrum, coordinates);
}

template <typename T>
SplineSpectrum<T> Diginstrument::Interpolator<T, SplineSpectrum<T>>::linear(SplineSpectrum<T> &left, SplineSpectrum<T> &right, const T &target, const T &leftLabel, const T &rightLabel)
{
    //TODO:
}

template <typename T>
SplineSpectrum<T> Diginstrument::Interpolator<T, SplineSpectrum<T>>::linearShift(SplineSpectrum<T> &left, SplineSpectrum<T> &right, const T &target, const T &leftLabel, const T &rightLabel)
{
    //TODO
}

template <typename T>
SplineSpectrum<T> Diginstrument::Interpolator<T, SplineSpectrum<T>>::getSpectrum(const std::vector<T> &coordinates)
{
    //TODO
    //tmp
    const auto res = data.getNeighbours(coordinates);
    if (res.size() > 0 && res.front().size() > 0)
        return res.front().front();
    return SplineSpectrum<T>(0);
}

template class Diginstrument::Interpolator<double, Diginstrument::NoteSpectrum<double>>;
template class Diginstrument::Interpolator<double, SplineSpectrum<double>>;