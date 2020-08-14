#include "Interpolator.h"

//tmp
#include <iostream>

using namespace std;
using namespace Diginstrument;

template <typename T, class S>
std::vector<Diginstrument::Component<T>> Diginstrument::Interpolator<T, S>::getSpectrum(const std::vector<T> &coordinates)
{
    //TODO: tmp: quality parameter
    return data.processIntoRoot(coordinates,
        [this](const S &left, const S &right, const T &target, const T &leftLabel, const T &rightLabel, const unsigned int dimension)
        {
            return interpolateSpectra(left, right, target, leftLabel, rightLabel, dimensions[dimension].second);
        }).getComponents(0);
}

template <typename T, class S>
S Diginstrument::Interpolator<T, S>::interpolateSpectra(const S &left, const S &right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting)
{
    //TODO: snare test has two distinct pulses, and goes into negative for some reason
    //TODO: why am i not using the shifting boolean?
    //NOTE: we do need to match components to do any meaningful shifting
    //ex.: a wide peak on left and a sharper, higher-pitched peak on right. Number of components not equal.
    //TODO: pseudocode:
    /*
        1) Match the peaks
        2) Interpolate the matched ones together, based on weights
            //move matched peaks to their weighted frequency
        3) Add the unmatched peaks, modulated amplitude with weights
        4) Accumulate bins (is this needed, if close ones are matched?)
            //if they are "close enough", consider them a bin
            //calculate an average frequency
            //accumulate or average amplitude?
    */

    vector<Component<T>> harmonics;
    //TODO: there should be a more sophisticated way of dealing with unmatched peaks. for example, if the left harmonic series is shorter then the right, just adding those in without shifting will sound terrible
    vector<unsigned int> unmatchedLeft;
    vector<unsigned int> unmatchedRight;

    const T rightWeight = target / (rightLabel - leftLabel);
    const T leftWeight = 1.0f - rightWeight;
    const T leftRatio = target / leftLabel;
    const T rightRatio = target / rightLabel;

    auto leftHarmonics = left.getHarmonics();
    auto rightHarmonics = right.getHarmonics();
    //TODO: acts weird if i hit an exact point (eg. 440)
    //TODO: acts even weirder with snare test: 4 spectra on same "pitch" and different time coordinates

    if (left.empty() || !shifting)
    {
        //return attenuated right
        unmatchedRight.resize(rightHarmonics.size());
        for(int i = 0; i<unmatchedRight.size(); i++)
        {
            unmatchedRight[i]=i;
        }
    }
    if (right.empty() || !shifting)
    {
        //return attenuated left
        unmatchedLeft.resize(leftHarmonics.size());
        for(int i = 0; i<unmatchedLeft.size(); i++)
        {
            unmatchedLeft[i]=i;
        }
    }
    if(shifting && !right.empty() && !left.empty())
    {
        //1) match peaks
        //TODO: generalize
        //TODO: refactor
        auto matches = PeakMatcher::matchPeaks(left.getHarmonics(), right.getHarmonics(), unmatchedLeft, unmatchedRight);

        //debug
        //cout << "number of harmonics: " << leftHarmonics.size() << " " << rightHarmonics.size() << endl;
        //cout << "matches" << endl;
        for (auto &match : matches)
        {
            //tmp:debug
            //std::cout << leftHarmonics[match.left].frequency << " - " << rightHarmonics[match.right].frequency << " : " << match.distance << std::endl;
            //tmp
            if (leftHarmonics[match.left].frequency == rightHarmonics[match.right].frequency)
            {
                harmonics.emplace_back(leftHarmonics[match.left].frequency, 0, leftHarmonics[match.left].amplitude * leftWeight + rightHarmonics[match.right].amplitude * rightWeight);
            }
            else
            {
                //TODO: NOTE: this simple method might not work generally, but for the pure sine it does
                harmonics.emplace_back((leftHarmonics[match.left].frequency * leftRatio + rightHarmonics[match.right].frequency * rightRatio) / 2.0, 0, leftHarmonics[match.left].amplitude * leftWeight + rightHarmonics[match.right].amplitude * rightWeight);
            }
        }
        //TODO: idea: unmatched overtones should be shifted up and then attenuated
        //TODO: TMP: FIXME: this assumes all components are to be shifted
        //TODO: TMP: FIXME: we assume that the first match is the fundamental frequency
        for(auto & overtone : unmatchedLeft)
        {
            leftHarmonics[overtone].frequency = leftHarmonics[overtone].frequency * (target/leftHarmonics[matches.front().left].frequency);
        }
        for(auto & overtone : unmatchedRight)
        {
            rightHarmonics[overtone].frequency = rightHarmonics[overtone].frequency * (target/rightHarmonics[matches.front().left].frequency);
        }
    }

    //TODO: why am i handling unmatched left and right separately?
    //tmp TODO: unmatched are still added, but no shifting for now
    for (auto &unmatched : unmatchedLeft)
    {
        harmonics.emplace_back(leftHarmonics[unmatched].frequency, 0, leftHarmonics[unmatched].amplitude * leftWeight);
    }
    for (auto &unmatched : unmatchedRight)
    {
        harmonics.emplace_back(rightHarmonics[unmatched].frequency, 0, rightHarmonics[unmatched].amplitude * rightWeight);
    }

    //tmp: debug
    /*cout << "unmatched left: ";
    for (auto e : unmatchedLeft)
    {
        cout << leftHarmonics[e].frequency << " ";
    }
    cout << endl;
    cout << "unmatched right: ";
    for (auto e : unmatchedRight)
    {
        cout << rightHarmonics[e].frequency << " ";
    }
    cout << endl;*/

    return Diginstrument::NoteSpectrum<double>(target, harmonics, {});
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::addSpectrum(const S &spectrum, std::vector<T> coordinates)
{
    //TODO:test, check, better
    data.insert(spectrum, coordinates);
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::clear()
{
    data.clear();
    dimensions.clear();
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::setDimensions(const std::vector<std::pair<std::string, bool>> &dimensions)
{
    this->dimensions = dimensions;
}

//TODO: remake with the more generic method
template <typename T>
SplineSpectrum<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::interpolateSpectra(
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
    if (shifting /*tmp*/ && left.getSpline().getPeaks().size() > 0 && right.getSpline().getPeaks().size() > 0)
    {
        auto leftComponents = left.getComponents(0);
        auto rightComponents = right.getComponents(0);
        auto matches = PeakMatcher::matchPeaks(leftComponents, rightComponents);

        //TODO: process matches
        //TODO: test
        //tmp: debug
        //std::cout<<"left has: "<<leftComponents.size()<<", right has: "<<rightComponents.size()<<" - matches: "<<matches.size()<<std::endl;
        //TODO: new or dead peaks get  matched to another - multiple targets for a peak
        for (auto &match : matches)
        {
            //tmp:debug
            //std::cout<<leftComponents[match.left].frequency<< " - "<<rightComponents[match.right].frequency<<" : "<<match.distance<<std::endl;
            //tmp: limit
            if (match.distance > 20)
                continue;
            auto &l = left.getSpline().getPieces()[match.left];
            auto &r = right.getSpline().getPieces()[match.right];
            const T target = (r.getEnd() - l.getEnd()) * rightRatio + l.getEnd();
            //std::cout<<l.getBegin()<<", "<<l.getEnd()<<" to "<<target<<std::endl;
            //std::cout<<r.getBegin()<<", "<<r.getEnd()<<" to "<<target<<std::endl;
            left.getSpline().stretchPieceEndTo(match.left, target);
            right.getSpline().stretchPieceEndTo(match.right, target);
            //std::cout<<std::endl;
        }
        //std::cout<<std::endl;
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
    /*std::cout<<"RES: "<<std::endl;
    for(auto & c : SplineSpectrum<T, 4>(res).getComponents(0))
    {
        std::cout<<std::fixed<<c.frequency<<" : "<<c.amplitude<<std::endl;
    }
    std::cout<<std::endl<<std::endl;*/

    return SplineSpectrum<T, 4>(std::move(res));
}

template <typename T>
PiecewiseBSpline<T, 4> Diginstrument::Interpolator<T, SplineSpectrum<T, 4>>::consolidatePieces(PiecewiseBSpline<T, 4> &left, PiecewiseBSpline<T, 4> &right, T rightRatio)
{
    PiecewiseBSpline<T, 4> res;
    //use deque as stack, as stack has no iterator initialization and has deque as underlying structure anyway
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> leftPieces(left.getPieces().rbegin(), left.getPieces().rend());
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> rightPieces(right.getPieces().rbegin(), right.getPieces().rend());

    if (right.getPieces().size() == 0)
        return left;
    if (left.getPieces().size() == 0)
        return right;

    if (leftPieces.back().getBegin() != rightPieces.back().getBegin())
    {
        //TODO: error handling
        return res;
    }

    double leftEnd = leftPieces.back().getEnd();
    double leftBegin = leftPieces.back().getBegin();
    double rightEnd = rightPieces.back().getEnd();
    double rightBegin = rightPieces.back().getBegin();
    //while there is a piece left to process in either of the stacks
    while (!leftPieces.empty() && !rightPieces.empty())
    {
        //TODO: tmp: if one of the pieces is "too short" (results in split of ratio 0)
        //NOTE: i think i need maxFD, else it would want to split and thats impossible
        //NOTE: or just move one end directly, maybe even into target?
        //TODO: merge these instead to next piece
        //TODO: note: this discards errors where begin>end as well
        if (rightPieces.back().getEnd() - rightPieces.back().getBegin() <= maxFrequencyDistance)
        {
            //std::cout<<"discarding from right: "<<rightPieces.back().getBegin()<<" - "<<rightPieces.back().getEnd()<<std::endl;
            //tmp: set opposite piece begin and res latest piece end : could be dangerous if new begin > end
            leftPieces.back().stretchTo(rightPieces.back().getEnd(), leftPieces.back().getEnd());
            res.getPieces().back().stretchTo(res.getPieces().back().getBegin(), rightPieces.back().getEnd());
            rightPieces.pop_back();
            //step left and right
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        if (leftPieces.back().getEnd() - leftPieces.back().getBegin() <= maxFrequencyDistance)
        {
            //std::cout<<"discarding from left: "<<leftPieces.back().getBegin()<<" - "<<leftPieces.back().getEnd()<<std::endl;
            //tmp: set opposite piece begin and res latest piece end : could be dangerous if new begin > end
            rightPieces.back().stretchTo(leftPieces.back().getEnd(), rightPieces.back().getEnd());
            res.getPieces().back().stretchTo(res.getPieces().back().getBegin(), leftPieces.back().getEnd());
            leftPieces.pop_back();
            //step left and right
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        //Note: cases with matching endpoints first, to use epsilon distance
        //matching endpoints
        if (fabs(leftBegin - rightBegin) <= maxFrequencyDistance && fabs(leftEnd - rightEnd) <= maxFrequencyDistance)
        {
            //add totally matching pieces
            res.add(matchPieces(leftPieces.back().getSpline(), rightPieces.back().getSpline(), rightRatio));
            //remove matched pieces
            rightPieces.pop_back();
            leftPieces.pop_back();
            //step left and right
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        //partial overlap, matching start points
        if (fabs(leftBegin - rightBegin) <= maxFrequencyDistance)
        {
            if (leftEnd > rightEnd)
            {
                //split left at right end
                const T ratio = (rightEnd - leftBegin) / (leftEnd - leftBegin);
                //tmp:
                //std::cout<<"splitting left @ "<<ratio<<std::endl;
                auto split = leftPieces.back().getSpline().split(ratio);
                //match split first with right
                res.add(matchPieces(split.first, rightPieces.back().getSpline(), rightRatio));
                //remove matched and split pieces
                rightPieces.pop_back();
                leftPieces.pop_back();
                //add split right back
                leftPieces.push_back(split.second);
            }
            if (leftEnd < rightEnd)
            {
                //split right at left end
                const T ratio = (leftEnd - rightBegin) / (rightEnd - rightBegin);
                //tmp:
                //std::cout<<"splitting right @ "<<ratio<<std::endl;
                auto split = rightPieces.back().getSpline().split(ratio);
                //tmp:
                //std::cout<<"split ("<<rightPieces.back().getBegin()<<", "<<rightPieces.back().getEnd()<<") into ("<<split.first.getControlPoints().front()[0]<<" , "<<split.first.getControlPoints().back()[0]<<") - ("<<split.second.getControlPoints().front()[0]<<", "<<split.second.getControlPoints().back()[0]<<")"<<std::endl;
                //match split first with right
                res.add(matchPieces(leftPieces.back().getSpline(), split.first, rightRatio));
                //remove matched and split pieces
                rightPieces.pop_back();
                leftPieces.pop_back();
                //add split right back
                rightPieces.push_back(split.second);
            }
            //step left and right
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        //partial overlap, matching end points
        //TODO: note: might be same as partial overlap with no endpoints...
        if (fabs(leftEnd - rightEnd) <= maxFrequencyDistance)
        {
            if (leftBegin < rightBegin)
            {
                //split left at right begin
                const T ratio = (rightBegin - leftBegin) / (leftEnd - leftBegin);
                //tmp:
                //std::cout<<"splitting left @ "<<ratio<<std::endl;
                auto split = leftPieces.back().getSpline().split(ratio);
                //add split part that had no everlap
                res.add(split.first);
                leftPieces.pop_back();
                leftPieces.push_back(split.second);
                //step left
                if (!leftPieces.empty())
                {
                    leftEnd = leftPieces.back().getEnd();
                    leftBegin = leftPieces.back().getBegin();
                }
                else
                {
                    //TODO
                }
                continue;
            }
            if (leftBegin > rightBegin)
            {
                //split right at left begin
                const T ratio = (leftBegin - rightBegin) / (rightEnd - rightBegin);
                //tmp:
                //std::cout<<"splitting right @ "<<ratio<<std::endl;
                auto split = rightPieces.back().getSpline().split(ratio);
                //add split part that had no everlap
                res.add(split.first);
                rightPieces.pop_back();
                rightPieces.push_back(split.second);
                //step right
                if (!rightPieces.empty())
                {
                    rightEnd = rightPieces.back().getEnd();
                    rightBegin = rightPieces.back().getBegin();
                }
                else
                {
                    //TODO
                }
                continue;
            }
        }
        //no overlap
        if (leftBegin < rightBegin && leftEnd < rightBegin)
        {
            //left only
            res.add(leftPieces.back().getSpline());
            leftPieces.pop_back();
            //step left
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        if (leftBegin > rightBegin && leftBegin > rightEnd)
        {
            //right only
            res.add(rightPieces.back().getSpline());
            rightPieces.pop_back();
            //step right
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        //if one encompasses the other
        //TODO?
        //partial overlap, no matching endpoints
        if (leftBegin > rightBegin && leftBegin < rightEnd && leftEnd > rightEnd)
        {
            //split right at left begin
            const T ratio = (leftBegin - rightBegin) / (rightEnd - rightBegin);
            //tmp:
            //std::cout<<"splitting right @ "<<ratio<<std::endl;
            auto split = rightPieces.back().getSpline().split(ratio);
            //add split part that had no everlap
            res.add(split.first);
            rightPieces.pop_back();
            rightPieces.push_back(split.second);
            //step right
            if (!rightPieces.empty())
            {
                rightEnd = rightPieces.back().getEnd();
                rightBegin = rightPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
        }
        if (leftBegin < rightBegin && leftEnd > rightBegin && leftEnd < rightEnd)
        {
            //split left at right begin
            const T ratio = (rightBegin - leftBegin) / (leftEnd - leftBegin);
            //tmp:
            //std::cout<<"splitting left @ "<<ratio<<std::endl;
            auto split = leftPieces.back().getSpline().split(ratio);
            //add split part that had no everlap
            res.add(split.first);
            leftPieces.pop_back();
            leftPieces.push_back(split.second);
            //step left
            if (!leftPieces.empty())
            {
                leftEnd = leftPieces.back().getEnd();
                leftBegin = leftPieces.back().getBegin();
            }
            else
            {
                //TODO
            }
            continue;
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
        leftEnd[1] * leftRatio + rightEnd[1] * rightRatio,
        //Interpolation::Linear(0.0, leftEnd[1],1.0, rightEnd[1],rightRatio), //seems to be the same as linear combination
        leftEnd[2] * leftRatio + rightEnd[2] * rightRatio});

    res.setControlPoints(std::move(resCPs));
    res.setKnotVector(std::move(resKnotVector));
    return res;
}

template class Diginstrument::Interpolator<double, Diginstrument::NoteSpectrum<double>>;
template class Diginstrument::Interpolator<double, SplineSpectrum<double, 4>>;