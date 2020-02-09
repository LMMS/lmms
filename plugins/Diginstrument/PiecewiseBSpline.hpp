#pragma once

#include <algorithm>

#include "BSpline.hpp"

/*A piecewise B-Spline, made by linking D-degree splines together.
 The pieces must fit together to ensure C1 continuity.*/
template <typename T, unsigned int D>
class PiecewiseBSpline
{
private:
    class Piece
    {
        friend class PiecewiseBSpline;

    protected:
        BSpline<T, D> spline;
        T begin;
        T end;

    public:
        bool operator<(const Piece &other)
        {
            return (begin < other.beigin && end < other.end);
        }
        bool operator<(const T x)
        {
            return this->end < x;
        }

        Piece(BSpline<T, D> &&spline) : spline(std::move(spline))
        {
            begin = this->spline.getControlPoints().front().first;
            end = this->spline.getControlPoints().back().first;
        }

        Piece(const BSpline<T, D> &spline)
            : spline(spline),
              begin(spline.getControlPoints().front().first),
              end(spline.getControlPoints().back().first)
        {
        }
    };

    std::vector<Piece> pieces;

public:
    /*Add a B-Spline as the next piece. If the piece doesn't align, it will not be added.*/
    bool add(BSpline<T, D> &&spline);
    bool add(const BSpline<T, D> &spline);

    /*Return the peaks of the spline.*/
    std::vector<std::pair<T, T>> getPeaks() const;

    /*Evaluate the spline at x*/
    std::pair<T, T> operator[](T x);
};

template <typename T, unsigned int D>
bool PiecewiseBSpline<T, D>::add(BSpline<T, D> &&spline)
{
    if (pieces.size() == 0)
    {
        pieces.emplace_back(std::move(spline));
        return true;
    }
    //if it does not fit, don't add
    if (pieces.back().spline.getControlPoints().back().first != spline.getControlPoints().front().first || pieces.back().spline.getControlPoints().back().second != spline.getControlPoints().front().second)
    {
        return false;
    }
    //else add the piece
    pieces.emplace_back(std::move(spline));
    return true;
}

template <typename T, unsigned int D>
bool PiecewiseBSpline<T, D>::add(const BSpline<T, D> &spline)
{
    if (pieces.size() == 0)
    {
        pieces.emplace_back(spline);
        return true;
    }
    //if it does not fit, don't add
    if (pieces.back().spline.getControlPoints().back().first != spline.getControlPoints().front().first || pieces.back().spline.getControlPoints().back().second != spline.getControlPoints().front().second)
    {
        return false;
    }
    //else add the piece
    pieces.emplace_back(spline);
    return true;
}

template <typename T, unsigned int D>
std::pair<T, T> PiecewiseBSpline<T, D>::operator[](T x)
{
    //out of bounds, lower
    if (x < pieces.front().begin)
    {
        return pieces.front().spline[0];
    }
    //find the correct spline
    auto it = std::lower_bound(pieces.begin(), pieces.end(), x);
    //out of bounds, upper
    if (it == pieces.end())
    {
        return pieces.back().spline[1];
    }
    //map to [0,1]
    return it->spline[(x - it->begin) / (it->end - it->begin)];
}

template <typename T, unsigned int D>
std::vector<std::pair<T, T>> PiecewiseBSpline<T, D>::getPeaks() const
{
    std::vector<std::pair<T, T>> res;
    res.reserve(pieces.size());
    for(const auto & piece : pieces)
    {
        res.emplace_back(piece.spline.getControlPoints().back());
    }
    res.resize(res.size()-1);
    return res;
}
