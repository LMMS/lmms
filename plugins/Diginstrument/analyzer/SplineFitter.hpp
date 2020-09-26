#pragma once

#include <vector>
#include <algorithm>

#include <Eigen/Dense>

#include "../common/BSpline.hpp"

/*Approximation by least square fitting of D-degree B-Spline.*/
template <typename T, unsigned int D>
class SplineFitter
{
public:
    /*Returns the n+1 coefficients for N[i,D](u), where i equals the index in the returned array. The knot vector must be clamped!*/
    static std::vector<T> coefficients(int n, T u, const std::vector<T> &knotVector);

    /*Returns a clamped knot vector of dataPoints+D+1 size with uniform spacing.*/
    static std::vector<T> makeUniformClampedKnotVector(unsigned int points);

    /*Returns a clamped knot vector of dataPoints+D+1 size with the internal knots equal to the last parameters.*/
    static std::vector<T> makeIdentityClampedKnotVector(unsigned int points, const std::vector<T> &parameters);

    /*Returns a vector of uniform parameters. Size is equal to the number of input points.*/
    static std::vector<T> generateUniformParameters(const std::vector<std::vector<T>> &points);

    /*Returns a vector of identity parameters, meaning they are the first coordinate mapped to [0,1]. Size is equal to the number of input points.*/
    static std::vector<T> generateIdentityParameters(const std::vector<std::vector<T>> &points);

    /*Least square fits a D degree B-Spline to the data points using the given amount of control points.*/
    static BSpline<T, D> fit(const std::vector<std::vector<T>> &dataPoints, unsigned int controlPoints);
};

template <typename T, unsigned int D>
BSpline<T, D> SplineFitter<T, D>::fit(const std::vector<std::vector<T>> &points, unsigned int controlPoints)
{
    const int m = points.size() - 1;
    const int n = controlPoints - 1;
    const int pointDimension = points.front().size();

    std::vector<T> t = generateIdentityParameters(points);
    std::vector<T> knotVector = makeIdentityClampedKnotVector(n + 1, t);

    Eigen::MatrixXd Q(n + 1, pointDimension);
    Eigen::MatrixXd P(m + 1, pointDimension);
    Eigen::MatrixXd A(m + 1, n + 1);
    A.setZero();
    Q.setZero();
    P.setZero();

    //A[i,j] = N[j,D](t[i])
    for (int i = 0; i <= m; i++)
    {
        std::vector<T> N = coefficients(n, t[i], knotVector);
        for (int j = 0; j <= n; j++)
        {
            A(i, j) = N[j];
        }
    }

    //Fill P with the data points
    for (int i = 0; i <= m; i++)
    {   
        for(int j = 0; j<pointDimension; j++)
        {
            P(i, j) = points[i][j];
            P(i, j) = points[i][j];
        }
    }

    //Solve A*Q = P
    Q = A.householderQr().solve(P);

    //Construct the B-Spline
    BSpline<T, D> res;
    std::vector<std::vector<T>> resPoints(n + 1);
    resPoints.front() = points.front();
    resPoints.back() = points.back();
    for (int i = 1; i < resPoints.size() - 1; i++)
    {
        std::vector<T> resPoint(pointDimension,0);
        for(int j = 0; j<pointDimension;j++)
        {
            resPoint[j] = Q(i,j);
        }
        resPoints[i] = resPoint;
    }
    res.setControlPoints(std::move(resPoints));
    res.setKnotVector(std::move(knotVector));
    return res;
}

//based on https://pages.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/bspline-curve-coef.html
template <typename T, unsigned int D>
std::vector<T> SplineFitter<T, D>::coefficients(int n, T u, const std::vector<T> &knotVector)
{
    //Note: not the most efficient, but a lot better than the recursive formula
    const int m = knotVector.size() - 1;
    std::vector<T> N(n + 1 + D, 0);
    // rule out special cases
    if (u == knotVector[0])
    {
        N[0] = 1;
        N.resize(n + 1);
        return N;
    }
    if (u == knotVector[m])
    {
        N[n] = 1;
        N.resize(n + 1);
        return N;
    }

    auto it = std::upper_bound(knotVector.begin(), knotVector.end(), u);
    int k = std::distance(knotVector.begin(), it) - 1;
    N[k] = 1; //degree 0 coefficient
    for (int d = 1; d <= D; d++)
    {
        N[k - d] = ((knotVector[k + 1] - u) / (knotVector[k + 1] - knotVector[k - d + 1])) * N[k - d + 1]; // right (south-west corner) term only
        for (int i = k - d + 1; i < k; i++)
        {
            N[i] = ((u - knotVector[i]) / (knotVector[i + d] - knotVector[i])) * N[i] + ((knotVector[i + d + 1] - u) / (knotVector[i + d + 1] - knotVector[i + 1])) * N[i + 1];
        }
        N[k] = ((u - knotVector[k]) / (knotVector[k + d] - knotVector[k])) * N[k]; // left (north-west corner) term only
    }
    N.resize(n + 1);
    return N;
}

template <typename T, unsigned int D>
std::vector<T> SplineFitter<T, D>::makeUniformClampedKnotVector(unsigned int dataPoints)
{
    std::vector<T> knotVector(dataPoints + D + 1);
    for (int i = 0; i < D + 1; i++)
    {
        knotVector[i] = 0;
    }
    for (int i = knotVector.size() - 1; i > knotVector.size() - D - 2; i--)
    {
        knotVector[i] = 1;
    }
    const double distance = 1.0 / (double)(knotVector.size() - 2 * (D + 1) + 1);
    for (int i = D + 1; i < knotVector.size() - 1 - D; i++)
    {
        knotVector[i] = (i - D) * distance;
    }
    return knotVector;
}

template <typename T, unsigned int D>
std::vector<T> SplineFitter<T, D>::generateUniformParameters(const std::vector<std::vector<T>> &dataPoints)
{
    std::vector<T> t(dataPoints.size());
    const T L = (T)1 / (T)(dataPoints.size() - 1);
    for (int k = 0; k < dataPoints.size(); k++)
    {
        t[k] = k * L;
    }
    return t;
}

template <typename T, unsigned int D>
std::vector<T> SplineFitter<T, D>::generateIdentityParameters(const std::vector<std::vector<T>> &dataPoints)
{
    std::vector<T> t;
    t.reserve(dataPoints.size());
    const T L = dataPoints.back().front() - dataPoints.front().front();
    const T x0 = dataPoints.front().front();
    for (const std::vector<T> &p : dataPoints)
    {
        t.emplace_back((p.front() - x0) / L);
    }
    return t;
}

template <typename T, unsigned int D>
std::vector<T> SplineFitter<T, D>::makeIdentityClampedKnotVector(unsigned int points, const std::vector<T> &parameters)
{
    std::vector<T> knotVector(points + D + 1, 0);
    for (int i = 0; i < D + 1; i++)
    {
        knotVector[i] = 0;
    }
    for (int i = knotVector.size() - 1; i > knotVector.size() - D - 2; i--)
    {
        knotVector[i] = 1;
    }
    for (int i = 1; i < points - D; i++)
    {
        const int index = points - i;
        for (int j = parameters.size() - i - 1; j > parameters.size() - i - D - 1; j--)
        {
            knotVector[index] += parameters[j];
        }
        knotVector[index] /= (T)D;
    }
    return knotVector;
}
