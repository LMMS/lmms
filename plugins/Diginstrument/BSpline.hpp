#pragma once

#include <vector>
#include <algorithm>

/*B-spline of degree D*/
template <typename T, unsigned int D>
class BSpline
{
private:
    std::vector<T> knotVector;
    std::vector<std::pair<T, T>> controlPoints;

public:
    /*Evaluate the spline at t, which must be in the range [0,1].*/
    std::pair<T, T> operator[](T t) const;

    /*Set the knot vector. Must be clamped and ordered. All elements must be in the range [0,1]*/
    void setKnotVector(const std::vector<T> &vector)
    {
        this->knotVector = vector;
    }

    void setKnotVector(std::vector<T> &&vector)
    {
        this->knotVector = std::move(vector);
    }

    /*Set control points.*/
    void setControlPoints(std::vector<std::pair<T, T>> &&vector)
    {
        this->controlPoints = std::move(vector);
    }

    void setControlPoints(const std::vector<std::pair<T, T>> &vector)
    {
        this->controlPoints = vector;
    }

    std::vector<T> getKnotVector() const
    {
        return knotVector;
    }

    std::vector<std::pair<T, T>> getControlPoints() const
    {
        return controlPoints;
    }

    BSpline() : knotVector({}), controlPoints({}) {}
    BSpline(const BSpline &other) : knotVector(other.knotVector), controlPoints(other.controlPoints) {}
};

template <typename T, unsigned int D>
std::pair<T, T> BSpline<T, D>::operator[](T t) const
{
    if (t == 0)
    {
        return controlPoints.front();
    }
    if (t == 1)
    {
        return controlPoints.back();
    }
    //deBoor algorithm
    auto lower = std::upper_bound(knotVector.begin(), knotVector.end(), t) - 1;
    unsigned int k = std::distance(knotVector.begin(), lower);
    unsigned int p = D;
    std::vector<std::pair<T, T>> d(p + 1 + k);
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
