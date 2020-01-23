#pragma once

#include <vector>
#include <algorithm>

#include "Point.hpp"

/* An example:
    BSpline<float, 2> spline;
	spline.setControlPoints({{0, 0},
							{0.2, 0},
							{0.4, 0},
							{0.6, 1},
							{0.8, 0},
							{1, 0}});
	spline.setKnotVector({0,0,0, 0.25, 0.50, 0.50, 1, 1, 1});

    The curve should coincide with the control polinom at 0.6
*/

//TODO: maybe exceptions, other error handling
/*B-spline of degree D*/
template <typename T, unsigned int D>
class BSpline
{
private:
    std::vector<T> knotVector;
    std::vector<Point<T>> controlPoints;

public:
    Point<T> operator()(T t);
    void setKnotVector(const std::vector<T> &vector)
    {
        //knot vector size = control points + degree + 1
        //knot vector should be normalized between [0,1] and ordered
        this->knotVector = vector;
    }
    void setControlPoints(const std::vector<Point<T>> &vector)
    {
        this->controlPoints = vector;
    }
};

// t should be normalized between [0,1]
template <typename T, unsigned int D>
Point<T> BSpline<T, D>::operator()(T t)
{
    //deBoor algorithm
    auto lower = std::upper_bound(knotVector.begin(), knotVector.end(), t) - 1;
    unsigned int k = std::distance(knotVector.begin(), lower);
    unsigned int p = D;
    std::vector<Point<T>> d(p + 1);
    for (int j = 0; j <= p; j++)
    {
        d[j] = controlPoints[j + k - p];
    }
    for (int r = 1; r <= p; r++)
    {
        for (int j = p; j >= r; j--)
        {
            float alpha = (t - knotVector[j + k - p]) / (knotVector[j + 1 + k - r] - knotVector[j + k - p]);
            d[j].first = (1.0f - alpha) * d[j - 1].first + alpha * d[j].first;
            d[j].second = (1.0f - alpha) * d[j - 1].second + alpha * d[j].second;
        }
    }
    return d[p];
}