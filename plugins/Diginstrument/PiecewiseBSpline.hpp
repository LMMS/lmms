#pragma once

#include <algorithm>

#include "BSpline.hpp"

/*A piecewise B-Spline, made by linking D-degree splines together.
 The pieces must fit together to ensure C1 continuity.*/
template <typename T, unsigned int D>
class PiecewiseBSpline
{
public:
    class Piece
    {
        friend class PiecewiseBSpline;

    protected:
        BSpline<T, D> spline;
        T begin;
        T end;

    public:
        bool operator<(const Piece &other) const
        {
            return (begin < other.beigin && end < other.end);
        }
        bool operator<(const T x) const
        {
            return this->end < x;
        }

        T getBegin() const
        {
            return begin;
        }

        T getEnd() const
        {
            return end;
        }

        const BSpline<T, D> &getSpline() const
        {
            return spline;
        }

        void stretchTo(T begin, T end)
        {
            spline.stretchTo(begin, end);
            this->begin = begin;
            this->end = end;
        }

        Piece(BSpline<T, D> &&spline) : spline(std::move(spline))
        {
            begin = this->spline.getControlPoints().front().front();
            end = this->spline.getControlPoints().back().front();
        }

        Piece(const BSpline<T, D> &spline)
            : spline(spline),
              begin(spline.getControlPoints().front().front()),
              end(spline.getControlPoints().back().front())
        {
        }
    };

    /*Add a B-Spline as the next piece. If the piece doesn't align, it will not be added.*/
    bool add(BSpline<T, D> &&spline);
    bool add(const BSpline<T, D> &spline);

    /*Return the peaks of the spline.*/
    std::vector<std::vector<T>> getPeaks() const;

    /*Evaluate the spline at x*/
    std::vector<T> operator[](T x) const;

    std::vector<Piece> &getPieces()
    {
        return pieces;
    }

    PiecewiseBSpline() : pieces({}) {}
    PiecewiseBSpline(const PiecewiseBSpline &spline) : pieces(spline.pieces) {}
    PiecewiseBSpline(PiecewiseBSpline &&spline) : pieces(std::move(spline.pieces)) {}

    //TODO
    const T getBegin() const
    {
        if (pieces.size() == 0)
            return -1;
        return pieces.front().begin;
    }
    //TODO
    const T getEnd() const
    {
        if (pieces.size() == 0)
            return -1;
        return pieces.back().end;
    }

private:
    std::vector<Piece> pieces;
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
    //tmp:disabled
    /*if (pieces.back().spline.getControlPoints().back()[0] != spline.getControlPoints().front()[0] || pieces.back().spline.getControlPoints().back().second != spline.getControlPoints().front().second)
    {
        return false;
    }*/
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
    //tmp:disabled
    /*if (pieces.back().spline.getControlPoints().back().first != spline.getControlPoints().front().first || pieces.back().spline.getControlPoints().back().second != spline.getControlPoints().front().second)
    {
        return false;
    }*/
    //else add the piece
    pieces.emplace_back(spline);
    return true;
}

template <typename T, unsigned int D>
std::vector<T> PiecewiseBSpline<T, D>::operator[](T x) const
{   
    if(pieces.size() == 0) { return std::vector<T>(); }
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
std::vector<std::vector<T>> PiecewiseBSpline<T, D>::getPeaks() const
{
    if (pieces.size() == 0)
        return {};
    std::vector<std::vector<T>> res;
    res.reserve(pieces.size());
    for (const auto &piece : pieces)
    {
        res.emplace_back(piece.spline.getControlPoints().back());
    }
    res.resize(res.size() - 1);
    return res;
}