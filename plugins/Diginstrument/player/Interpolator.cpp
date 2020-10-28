#include "Interpolator.h"

//tmp
#include <iostream>

using namespace std;
using namespace Diginstrument;

/**
 * Interpolate a spectrum corresponding to the given coordinates
 */ 
template <typename T, class S>
S Diginstrument::Interpolator<T, S>::getSpectrum(const std::vector<T> &coordinates)
{
    return residual.processIntoRoot(coordinates,
        [this](const S &left, const S &right, const T &target, const T &leftLabel, const T &rightLabel, const unsigned int dimension)->S
        {
            return interpolateSpectra(left, right, target, leftLabel, rightLabel, dimensions[dimension].shifting);
        },
        [](const S &single)->S
        {
            return single;
        }
        );
}

/**
 * Interpolate a time-slice of the partials corresponding to the given coordinates, frame offset and amount of frames
 */
template <typename T, class S>
PartialSet<T> Diginstrument::Interpolator<T, S>::getPartials(const std::vector<T> &coordinates, unsigned int startFrame, unsigned int frames)
{
    return partials.processIntoRoot(coordinates,
            [this, startFrame, frames](const PartialSet<T> &left, const PartialSet<T> &right, const T &target, const T &leftLabel, const T &rightLabel, const unsigned int dimension)
            ->PartialSet<T>
            {
                //TODO: better slicing detection
                if(left.get().size()>0 && left.get().front().size()>frames)
                {
                    return interpolatePartialSet(left.getSlice(startFrame, frames), right.getSlice(startFrame, frames), target, leftLabel, rightLabel, dimensions[dimension].shifting);
                }
                return interpolatePartialSet(left, right, target, leftLabel, rightLabel, dimensions[dimension].shifting);
            },
            [startFrame, frames](const PartialSet<T> & single)->PartialSet<T> 
            {
                if(single.get().size()>0 && single.get().front().size()>frames)
                {
                    return single.getSlice(startFrame, frames);
                }
                return single;
            }
    );
}

template <typename T, class S>
PartialSet<T> Diginstrument::Interpolator<T, S>::interpolatePartialSet(const PartialSet<T> &left, const PartialSet<T> &right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting)
{
    //TODO
    //tmp: unimplemented warning
    cout<<"reached interpolation"<<endl;
    return left;
}

/**
 * Interpolate a spectrum from two spectra
 */
template <typename T, class S>
S Diginstrument::Interpolator<T, S>::interpolateSpectra(const S &left, const S &right, const T &target, const T &leftLabel, const T &rightLabel, const bool shifting)
{
    //TODO: if not shifting, do we need matching/bin accumulation?
    //TODO: BUGHUNT: negative regions in discrete examples (snare, bass drum)
    //TODO: what IS shifting?
    //TODO: if we are not shifting, components can overlap and amplify out of range -> need binning?
    vector<Match> matches;
    vector<unsigned int> unmatchedLeft;
    vector<unsigned int> unmatchedRight;

    //if either is empty, we can't match; we also don't match if we are not shifting
    //no matching
    if (left.empty() || !shifting)
    {
        //return attenuated right
        unmatchedRight.resize(right.getMatchables().size());
        for(int i = 0; i<unmatchedRight.size(); i++)
        {
            unmatchedRight[i]=i;
        }
    }
    if (right.empty() || !shifting)
    {
        //return attenuated left
        unmatchedLeft.resize(left.getMatchables().size());
        for(int i = 0; i<unmatchedLeft.size(); i++)
        {
            unmatchedLeft[i]=i;
        }
    }
    if(shifting && !right.empty() && !left.empty())
    {
        matches = PeakMatcher::matchPeaks(left.getMatchables(), right.getMatchables(), unmatchedLeft, unmatchedRight);
    }

    return constructSpectrum(left, right, target, leftLabel, rightLabel, matches, unmatchedLeft, unmatchedRight);
}

template <typename T, typename S>
NoteSpectrum<T> Diginstrument::Interpolator<T, S>::constructSpectrum(
    const NoteSpectrum<T> & left,
    const NoteSpectrum<T> & right,
    const T &target, const T &leftLabel, const T &rightLabel,
    const vector<Match> & matches,
    const vector<unsigned int> & unmatchedLeft,
    const vector<unsigned int> & unmatchedRight
    )
{
    //TODO: is unmatchable even neccesary? if its static, it should get matched either way!
    //TMP: dont use unmatchable

    //calculate shifting metrics
    const T rightWeight = target / (rightLabel - leftLabel);
    const T leftWeight = 1.0f - rightWeight;
    const T leftRatio = target / leftLabel;
    const T rightRatio = target / rightLabel;

    vector<Component<T>> res;
    const auto leftMatchables = left.getMatchables();
    const auto rightMatchables = right.getMatchables();
    
    //TODO: BUGHUNT: violin ~1500hz: negative amplitudes; seems to be common
    //TODO: BUGHUNT: percussions (bass drum, snare): common negative amplitudes, weird "pulse" and crackling?
    //TODO: BUGHUNT: might have to rethink what shifting truly means

    //tmp:debug:
    //cout<<"left matchables: "<<leftMatchables.size()<<endl;
    //cout<<"right matchables: "<<rightMatchables.size()<<endl;

    //process matches; each match results in one component
    for (const auto &match : matches)
    {
        res.emplace_back((leftMatchables[match.left].frequency * leftRatio + rightMatchables[match.right].frequency * rightRatio) / 2.0, 0, leftMatchables[match.left].amplitude * leftWeight + rightMatchables[match.right].amplitude * rightWeight);
    }
    
    //TMP: we shift all unmatched components as well
    //TODO: TMP: FIXME: we assume that the first match is the fundamental frequency
    if(matches.size()>0)
    {
        const T leftFF = (target/leftMatchables[matches.front().left].frequency);
        for(const auto & unmatched : unmatchedLeft)
        {
            res.emplace_back(leftMatchables[unmatched].frequency * leftFF, 0, leftMatchables[unmatched].amplitude * leftWeight);
        }
        const T rightFF = (target/rightMatchables[matches.front().right].frequency);
        for(const auto & unmatched : unmatchedRight)
        {
            res.emplace_back(rightMatchables[unmatched].frequency * rightFF, 0, rightMatchables[unmatched].amplitude * rightWeight);
        }
    }
    //TODO: if we don't match, two components can overlap and amplify/interfere with eachother; how to handle this? maybe do frequency bins?
    else
    {
        //we can't shift; just attenuate them
        for (const auto &unmatched : unmatchedLeft)
        {
            res.emplace_back(leftMatchables[unmatched].frequency, 0, leftMatchables[unmatched].amplitude * leftWeight);
        }
        for (const auto &unmatched : unmatchedRight)
        {
            res.emplace_back(rightMatchables[unmatched].frequency, 0, rightMatchables[unmatched].amplitude * rightWeight);
        }
    }
    return Diginstrument::NoteSpectrum<T>(res, {});
}

template <typename T, typename S>
SplineSpectrum<T, 4> Diginstrument::Interpolator<T, S>::constructSpectrum(
    const SplineSpectrum<T, 4> & left,
    const SplineSpectrum<T, 4> & right,
    const T &target, const T &leftLabel, const T &rightLabel,
    const std::vector<Match> & matches,
    const std::vector<unsigned int> & unmatchedLeft,
    const std::vector<unsigned int> & unmatchedRight
    )
{
    //TODO from previous impl: "if split has <D+1 CPs" : could we have too few points to split?
    //TODO: test stretching more, maybe it does introduce unacceptable distortion; suspiciously narrow slope if we stretch back (22000 to 400) - could be just the log scale tho!
    //TODO: does the order of stretching matter? if a peak is stretched higher then another which still needs to be stretched, it could cause problems?
    //TODO: where to place match maximum distance?
    //shifting metric
    const T rightRatio = (target - leftLabel) / (rightLabel - leftLabel);
    //copy the splines, as we will be modifying them
    auto leftCopy = left;
    auto rightCopy = right;

    //TODO: is stretching unmatched the desired behaviour?
    //TODO: TMP: FIXME: we assume that the first match is the fundamental frequency
    //TMP: TODO: FIXME: this "dumb stretching" could potentially cause pieces to overlap, if the shift target is > nextPiece.end or < piece.begin
    //TMP:disabled unmatched shifting
    //TODO: bughunt: this made the spline version of 440 die, because it shifted a low component even lower, causing the dumb stretching to have a end(0.07)<begin(10) schenario
    //TODO: consider if shifting unmatched components is even desireable.
    //      -> when can we get unmatched?
    /*if(matches.size()>0)
    {
        const T leftFF = (target/leftCopy.getMatchables()[matches.front().left].frequency);
        for(const auto & unmatched : unmatchedLeft)
        {
            //TODO: did this attenuate?
            auto & piece = leftCopy.getSpline().getPieces()[unmatched];
            auto & nextPiece = leftCopy.getSpline().getPieces()[unmatched+1];
            const auto target = leftCopy.getMatchables()[unmatched].frequency * leftFF;
            piece.stretchTo(piece.getBegin(), target);
            nextPiece.stretchTo(target, nextPiece.getEnd());
        }
        const T rightFF = (target/rightCopy.getMatchables()[matches.front().right].frequency);
        for(const auto & unmatched : unmatchedRight)
        {
            auto & piece = rightCopy.getSpline().getPieces()[unmatched];
            auto & nextPiece = rightCopy.getSpline().getPieces()[unmatched+1];
            const auto target = rightCopy.getMatchables()[unmatched].frequency * rightFF;
            piece.stretchTo(piece.getBegin(), target);
            nextPiece.stretchTo(target, nextPiece.getEnd());
        }
    }*/
    
    //stretch matched peaks to target
    for(auto & match : matches)
    {
        auto &l = leftCopy.getSpline().getPieces()[match.left];
        auto &r = rightCopy.getSpline().getPieces()[match.right];
        const T target = (r.getEnd() - l.getEnd()) * rightRatio + l.getEnd();
        leftCopy.getSpline().stretchPieceEndTo(match.left, target);
        rightCopy.getSpline().stretchPieceEndTo(match.right, target);
    }

    //merge the two splines
    auto res = consolidatePieces(leftCopy.getSpline(), rightCopy.getSpline(), rightRatio);

    return SplineSpectrum<T, 4>(std::move(res));
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::addSpectrum(const S &spectrum, std::vector<T> coordinates)
{
    //TODO:test, check, better
    residual.insert(spectrum, coordinates);
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::addSpectra(const std::vector<S> &spectra)
{
    //TODO:test, check, better
    for(const auto & s : spectra)
    {
        vector<T> labels;
        for(const auto & l : s.getLabels())
        {
            labels.push_back(l.second);
        }
        residual.insert(s, labels);
    }
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::addPartialSets(const std::vector<PartialSet<T>> & partialSets)
{
    //TODO:test, check, better
    for(const auto & p : partialSets)
    {
        vector<T> labels;
        for(const auto & l : p.getLabels())
        {
            labels.push_back(l.second);
        }
        partials.insert(p, labels);
    }
}


template <typename T, class S>
void Diginstrument::Interpolator<T, S>::clear()
{
    partials.clear();
    residual.clear();
    dimensions.clear();
}

template <typename T, class S>
void Diginstrument::Interpolator<T, S>::setDimensions(const std::vector<Diginstrument::Dimension> &dimensions)
{
    this->dimensions = dimensions;
}

template <typename T, class S>
const std::vector<Dimension> & Diginstrument::Interpolator<T, S>::getDimensions() const
{
    return this->dimensions;
}

//TODO:better doc
//NOTE: spline should be symmetric!
//NOTE: all piece begins should match
//Merge two piecewise B-Splines. Peaks have already been shifted.
template <typename T, typename S>
PiecewiseBSpline<T, 4> Diginstrument::Interpolator<T, S>::consolidatePieces(PiecewiseBSpline<T, 4> &left, PiecewiseBSpline<T, 4> &right, T rightRatio)
{
    //TMP: TODO: max distance of ends
    const T maxDistance = 0.01;
    //TODO: padding
    //TODO: FIXME: if we have a very short piece, splitting it could result in a ratio of 0
    PiecewiseBSpline<T, 4> res;
    //use deque as stack, as stack has no iterator initialization and has deque as underlying structure anyway
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> leftPieces(left.getPieces().rbegin(), left.getPieces().rend());
    std::deque<typename PiecewiseBSpline<T, 4>::Piece> rightPieces(right.getPieces().rbegin(), right.getPieces().rend());
    //pad front
    //NOTE: begins should always match!
    while (!leftPieces.empty() && !rightPieces.empty())
    {
        //matched pieces
        if( abs(leftPieces.back().getEnd() - rightPieces.back().getEnd()) <= maxDistance )
        {
            res.add(mergePieces(leftPieces.back().getSpline(), rightPieces.back().getSpline(), rightRatio));
            leftPieces.pop_back();
            rightPieces.pop_back();
            continue;
        }
        //split right at left end
        if(leftPieces.back().getEnd() < rightPieces.back().getEnd())
        {
            const T ratio = (leftPieces.back().getEnd() - rightPieces.back().getBegin()) / (rightPieces.back().getEnd() - rightPieces.back().getBegin());
            auto split = rightPieces.back().getSpline().split(ratio);
            //TMP: TODO: FIXME: BUGHUNT: must adjust splitpoint, as it is inaccurate for some reason (See TODO in BSpline::split)
            split.first.stretchTo(rightPieces.back().getBegin(), leftPieces.back().getEnd());
            split.second.stretchTo(leftPieces.back().getEnd(), rightPieces.back().getEnd());
            //match left with split first
            res.add(mergePieces(leftPieces.back().getSpline(), split.first, rightRatio));
            //remove matched and split pieces
            rightPieces.pop_back();
            leftPieces.pop_back();
            //add split right back
            rightPieces.push_back(split.second);
            continue;
        }
        //split left at right end
        if(leftPieces.back().getEnd() > rightPieces.back().getEnd())
        {
            //split left at right end
            const T ratio = (rightPieces.back().getEnd() - leftPieces.back().getBegin()) / (leftPieces.back().getEnd() - leftPieces.back().getBegin());
            auto split = leftPieces.back().getSpline().split(ratio);
            //TMP: TODO: FIXME: must adjust splitpoint, as it is inaccurate for some reason (See TODO in BSpline::split)
            //NOTES: the problem seems to stem from these hugely long pieces, where of course the resolution will be smeared
            //Maybe the CPs dont even need to be equidistant (arc length in paper) and/or dont really need uniform knot vector, if the resolution is high enough?
            //EXPERIMENT results: if the CPs are symmetric and the knotvector uniform, the mapping should be accurate
            split.first.stretchTo(leftPieces.back().getBegin(), rightPieces.back().getEnd());
            split.second.stretchTo(rightPieces.back().getEnd(), leftPieces.back().getEnd());
            //match split first with right
            res.add(mergePieces(split.first, rightPieces.back().getSpline(), rightRatio));
            //remove matched and split pieces
            rightPieces.pop_back();
            leftPieces.pop_back();
            //add split right back
            leftPieces.push_back(split.second);
            continue;
        }
    }
    //TODO: pad back
    return res;
}

//TODO: better documentation
//Interpolates two BSplines with identical start and end
//inserts knots to make controlpoint pairs to interpolate on
template <typename T, typename S>
BSpline<T, 4> Diginstrument::Interpolator<T, S>::mergePieces(BSpline<T, 4> left, BSpline<T, 4> right, T rightRatio)
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
    //TMP: all phase removed, only 2 coordinates (1 is amp)

    resCPs.emplace_back(std::vector<T>{
        std::min(leftBegin[0], rightBegin[0]),
        leftBegin[1] * leftRatio + rightBegin[1] * rightRatio
        //leftBegin[1],
        //leftBegin[2] * leftRatio + rightBegin[2] * rightRatio
        });
    //TODO: phase: how to interpolate "starting phase"?
    for (int i = 1; i < left.getControlPoints().size() - 1; i++)
    {
        const std::vector<T> leftCP = left.getControlPoints()[i];
        const std::vector<T> rightCP = right.getControlPoints()[i];
        //linear interpolation
        resCPs.emplace_back(std::vector<T>{
            (leftCP)[0] * leftRatio + (rightCP)[0] * rightRatio,
            (leftCP)[1] * leftRatio + (rightCP)[1] * rightRatio
            //leftCP[1],
            //(leftCP)[2] * leftRatio + (rightCP)[2] * rightRatio
            });
    }
    //do last point
    const std::vector<T> leftEnd = left.getControlPoints().back();
    const std::vector<T> rightEnd = right.getControlPoints().back();
    resCPs.emplace_back(std::vector<T>{
        //TODO: min-max, min-min or max-max?
        //OR NEITHER? CUZ FREQUENCY AND PHASE ARE INTERCONNECTED, DUMBASS?????
        //std::min(leftEnd[0], rightEnd[0]),
        leftEnd[0] * leftRatio + rightEnd[0] * rightRatio,
        leftEnd[1] * leftRatio + rightEnd[1] * rightRatio
        //Interpolation::Linear(0.0, leftEnd[1],1.0, rightEnd[1],rightRatio), //seems to be the same as linear combination
        //leftEnd[2] * leftRatio + rightEnd[2] * rightRatio
        });

    res.setControlPoints(std::move(resCPs));
    res.setKnotVector(std::move(resKnotVector));
    return res;
}

template class Diginstrument::Interpolator<double, Diginstrument::NoteSpectrum<double>>;
template class Diginstrument::Interpolator<double, SplineSpectrum<double, 4>>;