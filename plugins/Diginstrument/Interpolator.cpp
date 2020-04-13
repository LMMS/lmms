#include "Interpolator.h"

//tmp
#include <iostream>

template <typename T, class S>
std::vector<Diginstrument::Component<T>> Diginstrument::Interpolator<T, S>::getSpectrum(const std::vector<T> &coordinates)
{
    std::vector<Diginstrument::Component<T>> res;
    std::vector<std::vector<T>> labels;
    std::vector<std::vector<S>> possiblePairs = data.getNeighbours(coordinates, labels);

    if (possiblePairs.size() == 0)
    { /*TODO: exception?*/
        return std::vector<Diginstrument::Component<T>>{};
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

    return std::vector<Diginstrument::Component<T>>{};
}
/*
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
    //accumulate energy in frequency-windows
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
*/
//TODO: this is probably totally wrong after the introduction of phase...
/*template <typename T, class S>
S Diginstrument::Interpolator<T, S>::linear(S &left, S &right, const T &target, const T &leftLabel, const T &rightLabel)
{
    //TODO: think about the shifting of splinespectrum
    //TODO: MAYBE: write separate for differenct spectrum types
    std::cout << "interpolation: " << target << ", between: " << leftLabel << ", " << rightLabel << std::endl;
    T rightWeight = target / (rightLabel - leftLabel);
    T leftWeight = 1.0f - rightWeight;
    //TODO: expand to stochastics
    std::vector<Diginstrument::Component<T>> harmonics;
    //tmp : 0 phase
    for (auto &h : left.getHarmonics())
    {
        harmonics.emplace_back(h);
    }
    for (auto &h : right.getHarmonics())
    {
        harmonics.emplace_back(h);
    }

    //if one was empty, we dont need to accumulate
    if (harmonics.size() == left.getHarmonics().size() || harmonics.size() == right.getHarmonics().size())
    {
        return S(target, std::move(harmonics), {});
    }
    //accumulate energy in frequency-windows 
    std::sort(harmonics.begin(), harmonics.end());
    std::vector<std::pair<T, T>> accumulated;
    accumulated.reserve(harmonics.size() / 2);
    auto it = harmonics.begin();
    T baseFrequency = it->frequency;
    while (it != harmonics.end())
    {
        T accumulatedAmplitude = 0;
        while (it != harmonics.end() && it->frequency <= baseFrequency + frequencyStep)
        {
            accumulatedAmplitude += it->amplitude;
            it++;
        }
        accumulated.push_back(std::make_pair(baseFrequency, accumulatedAmplitude));
        baseFrequency = it->frequency;
    }

    return S(target, std::move(harmonics), {});
}*/

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
void Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::addSpectrum(const SplineSpectrum<T, 4> &spectrum, std::vector<T> coordinates)
{
    //TODO:test, check, better
    data.insert(spectrum, coordinates);
}

template <typename T>
SplineSpectrum<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::linear(
    SplineSpectrum<T, 4> left,
    SplineSpectrum<T, 4> right,
    const T &target,
    const T &leftLabel,
    const T &rightLabel,
    bool shifting)
{
    //TODO
    //TODO: if split has <D+1 CPs
    const T rightRatio = (target - leftLabel) / (rightLabel - leftLabel);
    //const T leftDistance = target-leftLabel;

    //1) stretch pieces
    if (shifting /*tmp*/ && left.getSpline().getPeaks().size()>0 && right.getSpline().getPeaks().size()>0)
    {
        auto leftComponents = left.getComponents(0);
        auto rightComponents = right.getComponents(0);
        auto matches = PeakMatcher::matchPeaks(leftComponents, rightComponents);

        //TODO: process matches
        //TODO: test
        //tmp: debug
        std::cout<<"left has: "<<leftComponents.size()<<", right has: "<<rightComponents.size()<<" - matches: "<<matches.size()<<std::endl;
        //TODO: new or dead peaks get  matched to another - multiple targets for a peak
        for( auto & match : matches)
        {
            //tmp:debug
            std::cout<<leftComponents[match.left].frequency<< " - "<<rightComponents[match.right].frequency<<" : "<<match.distance<<std::endl;
            //tmp: limit
            if(match.distance > 200) continue;
            auto &l = left.getSpline().getPieces()[match.left];
            auto &r = right.getSpline().getPieces()[match.right];
            const T target = (r.getEnd() - l.getEnd()) * rightRatio + l.getEnd();
            //std::cout<<l.getBegin()<<", "<<l.getEnd()<<" to "<<target<<std::endl;
            //std::cout<<r.getBegin()<<", "<<r.getEnd()<<" to "<<target<<std::endl;
            left.getSpline().stretchPieceEndTo(match.left, target);
            right.getSpline().stretchPieceEndTo(match.right, target);
            //std::cout<<std::endl;    
        }
        std::cout<<std::endl;    
    }
    //TODO: splines that cover too short a distance cause problems: zero ratio split, maybe too few points to split?
    //2) consolidate peaks
    auto res = consolidatePieces(left.getSpline(), right.getSpline(), rightRatio);

    //tmp: visualization
    /*for(double i = 20; i<20000; i+=20)
    {
        std::cout<<std::fixed<<"("<<i<<","<<left[i].amplitude<<"),";
    }
    std::cout<<std::endl<<std::endl<<std::endl<<std::endl;
    for(double i = 20; i<20000; i+=20)
    {
        std::cout<<std::fixed<<"("<<i<<","<<right[i].amplitude<<"),";
    }
    std::cout<<std::endl<<std::endl<<std::endl<<std::endl;
    for(double i = 20; i<20000; i+=20)
    {
        std::cout<<std::fixed<<"("<<i<<","<<res[i][2]<<"),";
    }*/

    //tmp:debug:
    std::cout<<"RES: "<<std::endl;
    for(auto & c : SplineSpectrum<T, 4>(res).getComponents(0))
    {
        std::cout<<std::fixed<<c.frequency<<" : "<<c.amplitude<<std::endl;
    }
    std::cout<<std::endl<<std::endl;

    return SplineSpectrum<T, 4>(std::move(res));
}

template <typename T>
PiecewiseBSpline<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::consolidatePieces(PiecewiseBSpline<T, 4> &left, PiecewiseBSpline<T, 4> &right, T rightRatio)
{
    PiecewiseBSpline<T, 4> res;
    //use deque as stack, as stack has no iterator initialization and has deque as underlying structure anyway
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> leftPieces(left.getPieces().rbegin(), left.getPieces().rend());
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> rightPieces(right.getPieces().rbegin(), right.getPieces().rend());

    if(right.getPieces().size() == 0) return left;
    if(left.getPieces().size() == 0) return right;

    if (leftPieces.back().getBegin() != rightPieces.back().getBegin())
    {
        //TODO: error handling
        return res;
    }

    double leftEnd = leftPieces.back().getEnd();
    double rightEnd = rightPieces.back().getEnd();
    //while there is a piece left to process in either of the stacks
    while (!leftPieces.empty() && !rightPieces.empty())
    {
        //TODO: tmp: if one of the pieces is "too short" (results in split of ratio 0)
        //NOTE: i think i need maxFD, else it would want to split and thats impossible
        //NOTE: or just move one end directly, maybe even into target?
        //TODO: merge these instead to next piece
        if(!rightPieces.empty() && rightPieces.back().getEnd() - rightPieces.back().getBegin() <= maxFrequencyDistance)
        {
            std::cout<<"discarding from right: "<<rightPieces.back().getBegin()<<" - "<<rightPieces.back().getEnd()<<std::endl;
            rightPieces.pop_back();
            rightEnd = rightPieces.back().getEnd();
            continue;
        }
        if(!leftPieces.empty() && leftPieces.back().getEnd() - leftPieces.back().getBegin() <= maxFrequencyDistance)
        {
            std::cout<<"discarding from left: "<<leftPieces.back().getBegin()<<" - "<<leftPieces.back().getEnd()<<std::endl;
            leftPieces.pop_back();
            leftEnd = leftPieces.back().getEnd();
            continue;
        }
        //if the pieces don't fit, that is the ends are too far away
        if (fabs(leftEnd - rightEnd) > maxFrequencyDistance)
        {
            //if the left piece is shorter
            if (leftEnd < rightEnd)
            {
                //split right
                const T ratio = (leftEnd - rightPieces.back().getBegin()) / (rightPieces.back().getEnd() - rightPieces.back().getBegin());
                std::cout<<"splitting left @ "<<ratio<<std::endl;
                auto split = rightPieces.back().getSpline().split(ratio);
                //match left with split left
                res.add(matchPieces(leftPieces.back().getSpline(), split.first, rightRatio));
                //remove matched and split pieces
                rightPieces.pop_back();
                leftPieces.pop_back();
                //add split right back
                rightPieces.push_back(split.second);
            }
            else
            {
                //split left
                const T ratio = (rightEnd - leftPieces.back().getBegin()) / (leftPieces.back().getEnd() - leftPieces.back().getBegin());
                std::cout<<"splitting left @ "<<ratio<<std::endl;
                auto split = leftPieces.back().getSpline().split(ratio);
                //match right with split left
                res.add(matchPieces(split.first, rightPieces.back().getSpline(), rightRatio));
                //remove matched and split pieces
                leftPieces.pop_back();
                rightPieces.pop_back();
                //add split right back
                leftPieces.push_back(split.second);
            }
        }
        //Ends are within range of eachother
        else
        {
            //match pieces
            res.add(matchPieces(leftPieces.back().getSpline(), rightPieces.back().getSpline(), rightRatio));
            //remove matched pieces
            leftPieces.pop_back();
            rightPieces.pop_back();
            //adjust begin of next pieces TODO?
        }
        //step
        if (!leftPieces.empty())
        {
            leftEnd = leftPieces.back().getEnd();
        }
        if (!rightPieces.empty())
        {
            rightEnd = rightPieces.back().getEnd();
        }
    }
    return res;
}

template <typename T>
BSpline<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::matchPieces(BSpline<T, 4> left, BSpline<T, 4> right, T rightRatio)
{
    BSpline<T, 4> res;
    constexpr unsigned int D = 4;
    //TODO: TMP: max distance
    const T maxKnotDistance = 0.02;
    const T leftRatio = 1.0 - rightRatio;

    //clamp knot vector at the beginning
    std::vector<T> resKnotVector(D + 1, 0);
    std::vector<std::vector<T>> resCPs;

    resKnotVector.reserve(left.getKnotVector().size() + right.getKnotVector().size() - 2 * (D + 1));
    resCPs.reserve(left.getControlPoints().size() + right.getControlPoints().size());

    //sweep both splines, and consolidate their control points:
    //if a knot doesn't have a close corresponding knot on the other spline, insert one
    typename std::vector<T>::const_iterator leftKnot = left.getKnotVector().begin() + D + 1;
    typename std::vector<T>::const_iterator rightKnot = right.getKnotVector().begin() + D + 1;
    //until both knots exceeded the last internal knot
    while (leftKnot < left.getKnotVector().end() - D - 1 || rightKnot < right.getKnotVector().end() - D - 1)
    {
        //calculate the distance between knots
        if (fabs((*leftKnot) - (*rightKnot)) <= maxKnotDistance && leftKnot < left.getKnotVector().end() - D - 1 && rightKnot < right.getKnotVector().end() - D - 1)
        {
            //knots are close together: insert their linear interpolation
            resKnotVector.push_back((*leftKnot) * leftRatio + (*rightKnot) * rightRatio);
        }
        else
        {
            //knots are far away: insert the lower knot to get a pair in the next loop
            if (*leftKnot < *rightKnot)
            {
                //insert left knot into right
                const int index = std::distance(right.getKnotVector().begin(), rightKnot);
                right.insertKnot(*leftKnot);
                rightKnot = right.getKnotVector().begin() + index;
                resKnotVector.push_back(*leftKnot);
            }
            else
            {
                //insert right knot into left
                const int index = std::distance(left.getKnotVector().begin(), leftKnot);
                left.insertKnot(*rightKnot);
                leftKnot = left.getKnotVector().begin() + index;
                resKnotVector.push_back(*rightKnot);
            }
        }
        if (leftKnot < left.getKnotVector().end() - D - 1)
        {
            leftKnot++;
        }
        if (rightKnot < right.getKnotVector().end() - D - 1)
        {
            rightKnot++;
        }
    }
    //clamp knot vector at the end
    for (int i = 0; i < D + 1; i++)
    {
        resKnotVector.push_back(1.0);
    }

    //interpolate between control point pairs
    //do first point
    const std::vector<T> leftBegin = left.getControlPoints().front();
    const std::vector<T> rightBegin = right.getControlPoints().front();

    resCPs.emplace_back(std::vector<T>{
        std::min(leftBegin[0], rightBegin[0]),
        leftBegin[1] * leftRatio + rightBegin[1] * rightRatio,
        //leftBegin[1],
        leftBegin[2] * leftRatio + rightBegin[2] * rightRatio});
        //TODO: phase: how to interpolate "starting phase"?
    for (int i = 1; i < left.getControlPoints().size() - 1; i++)
    {
        const std::vector<T> leftCP = left.getControlPoints()[i];
        const std::vector<T> rightCP = right.getControlPoints()[i];
        //linear interpolation
        resCPs.emplace_back(std::vector<T>{
            (leftCP)[0] * leftRatio + (rightCP)[0] * rightRatio,
            (leftCP)[1] * leftRatio + (rightCP)[1] * rightRatio,
            //leftCP[1],
            (leftCP)[2] * leftRatio + (rightCP)[2] * rightRatio});
    }
    //do last point
    const std::vector<T> leftEnd = left.getControlPoints().back();
    const std::vector<T> rightEnd = right.getControlPoints().back();
    resCPs.emplace_back(std::vector<T>{
        //TODO: min-max, min-min or max-max?
        //OR NEITHER? CUZ FREQUENCY AND PHASE ARE INTERCONNECTED, DUMBASS?????
        //std::min(leftEnd[0], rightEnd[0]),
        leftEnd[0] * leftRatio + rightEnd[0] * rightRatio,
        leftEnd[1]*leftRatio + rightEnd[1]*rightRatio,
        //Interpolation::Linear(0.0, leftEnd[1],1.0, rightEnd[1],rightRatio), //seems to be the same as linear combination
        leftEnd[2] * leftRatio + rightEnd[2] * rightRatio});

    res.setControlPoints(std::move(resCPs));
    res.setKnotVector(std::move(resKnotVector));
    return res;
}

template <typename T>
SplineSpectrum<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::getSpectrum(const std::vector<T> &coordinates)
{
    //TODO
    std::vector<std::vector<T>> labels;
    std::vector<std::vector<SplineSpectrum<T, 4>>> possiblePairs = data.getNeighbours(coordinates, labels);

    if (possiblePairs.size() == 0)
    { /*TODO: exception?*/
        return SplineSpectrum<T, 4>(-1);
    }

    //track current coordinate
    unsigned int currentCoordinate = coordinates.size() - 1;
    //until there is only one possible pair left
    //which means we interpolated on the last current coordinate
    while (possiblePairs.size() > 1 || currentCoordinate < 0)
    {
        //process all possible pairs into half the amount of possible pairs
        unsigned int currentNode = 0;
        std::vector<std::vector<SplineSpectrum<T, 4>>> tmp;
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
    //TMP: DEBUG: with splines, shifting interpolation should be on time
    if (possiblePairs.front().size() == 1)
    {
        return possiblePairs.front().front();
    }
    if (possiblePairs.front().size() == 2)
    {
        return linear(possiblePairs.front().front(), possiblePairs.front().back(),
                      coordinates[currentCoordinate] /*was .front()*/, labels[/*TMP: was 0*/ currentCoordinate][0], labels[/*TMP: was 0*/ currentCoordinate][1], true);
    }

    return SplineSpectrum<T, 4>(-1);
}

template class Diginstrument::Interpolator<double, Diginstrument::NoteSpectrum<double>>;
template class Diginstrument::Interpolator<double, SplineSpectrum<double, 4>>;