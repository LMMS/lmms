#pragma once

#include <vector>
#include <algorithm>

/*B-spline of degree D*/
template <typename T, unsigned int D>
class BSpline
{
private:
    std::vector<T> knotVector;
    std::vector<std::vector<T>> controlPoints;

    inline unsigned int findKnotIntervalIndex(T t) const
    {
        return std::distance(knotVector.begin(), std::upper_bound(knotVector.begin(), knotVector.end(), t) - 1);
    }

public:
    /*Evaluate the spline at t, which must be in the range [0,1].*/
    std::vector<T> operator[](T t) const;

    /*Split the spline into two other splines at t in range (0,1)*/
    std::pair<BSpline<T, D>, BSpline<T, D>> split(T t) const;

    /*Insert a new knot into the spline the given amount of times, without changing the shape. If the knot already exists, set multiplicity accordingly*/
    bool insertKnot(T t, unsigned int times = 1, unsigned int multiplicity = 0);

    /*Stretch the spline to fit the new 'begin' and 'end' coordinates.*/
    void stretchTo(T begin, T end);

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
    void setControlPoints(std::vector<std::vector<T>> &&vector)
    {
        this->controlPoints = std::move(vector);
    }

    void setControlPoints(const std::vector<std::vector<T>> &vector)
    {
        this->controlPoints = vector;
    }

    const std::vector<T> &getKnotVector() const
    {
        return knotVector;
    }

    const std::vector<std::vector<T>> &getControlPoints() const
    {
        return controlPoints;
    }

    BSpline() : knotVector({}), controlPoints({}) {}
    BSpline(const BSpline &other) : knotVector(other.knotVector), controlPoints(other.controlPoints) {}
};

template <typename T, unsigned int D>
std::vector<T> BSpline<T, D>::operator[](T t) const
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
    const unsigned int k = findKnotIntervalIndex(t);
    const unsigned int p = D;
    std::vector<std::vector<T>> d(p + 1 + k, std::vector<T>(controlPoints.front().size(), 0));
    for (int j = 0; j <= p; j++)
    {
        d[j] = controlPoints[j + k - p];
    }
    for (int r = 1; r <= p; r++)
    {
        for (int j = p; j >= r; j--)
        {
            float alpha = (t - knotVector[j + k - p]) / (knotVector[j + 1 + k - r] - knotVector[j + k - p]);
            //process all coordinates
            for (int c = 0; c < d[j].size(); c++)
            {
                d[j][c] = (1.0f - alpha) * d[j - 1][c] + alpha * d[j][c];
            }
        }
    }
    return d[p];
}

template <typename T, unsigned int D>
std::pair<BSpline<T, D>, BSpline<T, D>> BSpline<T, D>::split(T t) const
{
    //deBoor algorithm
    const unsigned int k = findKnotIntervalIndex(t);
    const unsigned int p = D;
    const unsigned int s = 0; //multiplicity of t
    std::vector<std::vector<T>> d(p + 1 + k, std::vector<T>(controlPoints.front().size(), 0));
    //Pl_0...Pl_k-p = P_0...P_k-p
    std::vector<std::vector<T>> leftCPs(controlPoints.begin(), (controlPoints.begin() + k - p + 1));
    std::vector<std::vector<T>> reverseRightCPs;
    reverseRightCPs.reserve(p);
    leftCPs.reserve(leftCPs.size() + p);

    for (int j = 0; j <= p; j++)
    {
        d[j] = controlPoints[j + k - p];
    }
    for (int r = 1; r <= p; r++)
    {
        //calculate new column
        for (int j = p; j >= r; j--)
        {
            float alpha = (t - knotVector[j + k - p]) / (knotVector[j + 1 + k - r] - knotVector[j + k - p]);
            //process all coordinates
            for (int c = 0; c < d[j].size(); c++)
            {
                d[j][c] = (1.0f - alpha) * d[j - 1][c] + alpha * d[j][c];
            }
        }
        //add the points
        leftCPs.push_back(d[r]);
        reverseRightCPs.push_back(d[p]);
    }

    std::reverse(reverseRightCPs.begin(), reverseRightCPs.end());
    reverseRightCPs.resize(reverseRightCPs.size() + (controlPoints.size() - k + s));
    //Pr_k-s...Pr_n = P_k-s...P_n
    std::copy(controlPoints.begin() + k - s, controlPoints.end(), reverseRightCPs.begin() + p);

    //construct knotvectors
    std::vector<T> leftKnotVector(knotVector.begin(), knotVector.begin() + k + 1);
    leftKnotVector.reserve(k + D + 2);
    std::vector<T> rightKnotVector(D + 1, t);
    rightKnotVector.reserve(D + 1 + (knotVector.size() - k - 1));
    for (int i = 0; i < D + 1; i++)
    {
        leftKnotVector.push_back(t);
    }
    for (int i = k + 1; i < knotVector.size(); i++)
    {
        rightKnotVector.push_back(knotVector[i]);
    }
    //normalize [0,t] to [0,1]
    const T leftRatio = 1.0 / t;
    for (auto &k : leftKnotVector)
    {
        k *= leftRatio;
    }
    //normalize [t,1] to [0,1]
    const T rightRatio = 1.0 / (1 - t);
    for (auto &k : rightKnotVector)
    {
        k = (k - t) * rightRatio;
    }

    BSpline<T, D> left;
    BSpline<T, D> right;
    left.setControlPoints(std::move(leftCPs));
    left.setKnotVector(std::move(leftKnotVector));
    right.setControlPoints(std::move(reverseRightCPs));
    right.setKnotVector(std::move(rightKnotVector));

    return std::make_pair(std::move(left), std::move(right));
}

/*Based on https://pages.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/multiple-time.html*/
template <typename T, unsigned int D>
bool BSpline<T, D>::insertKnot(T t, unsigned int times, unsigned int multiplicity)
{
    if (times + multiplicity > D)
    {
        //Cannot be inserted without changing shape
        return false;
    }
    const unsigned int k = findKnotIntervalIndex(t);
    const unsigned int p = D;
    //initialize working arrays
    controlPoints.reserve(controlPoints.size() + times);
    std::vector<std::vector<T>> workingArray(p - multiplicity, std::vector<T>(controlPoints.front().size()));
    //recalculate affected control points
    for (int r = 1; r <= times; r++)
    {
        for (int i = k - p + r; i < k - multiplicity; i++)
        {
            //calculate next column into working array
            const T a = (t - knotVector[i]) / (knotVector[i + p - r + 1] - knotVector[i]);
            //for all coordinates
            for (int c = 0; c < controlPoints.front().size(); c++)
            {
                workingArray[i - k + p - r][c] = (1.0 - a) * controlPoints[i - 1][c] + a * controlPoints[i][c];
            }
        }
        //insert P[k-s]
        std::vector<T> tmp(controlPoints.front().size(), 0);
        const T a = (t - knotVector[k - multiplicity]) / (knotVector[k - multiplicity + p - r + 1] - knotVector[k - multiplicity]);
        //for all coordinates
        for (int c = 0; c < tmp.size(); c++)
        {
            tmp[c] = (1.0 - a) * controlPoints[k - multiplicity - 1][c] + a * controlPoints[k - multiplicity][c];
        }
        controlPoints.insert(controlPoints.begin() + k - multiplicity, std::move(tmp));
        //rotate result into controlPoints
        std::copy(workingArray.begin(), workingArray.begin() + p - multiplicity - 1, controlPoints.begin() + k - p + 1);
    }
    //insert knots
    knotVector.insert(knotVector.begin() + k + 1, times, t);
    return true;
}

template <typename T, unsigned int D>
void BSpline<T, D>::stretchTo(T begin, T end)
{
    const T oldBegin = controlPoints.front()[0];
    const T ratio = (end - begin) / (controlPoints.back()[0] - oldBegin);
    for(auto & p : controlPoints)
    {
        p[0] = ratio * (p[0]-oldBegin) + begin;
    }
}