#pragma once

namespace Approximation
{
template <typename T>
inline std::pair<T, T> ParabolicNormalized(T a, T b, T c)
{
    const T p = (a - c) / (2 * (a - 2 * b + c));
    return std::make_pair(p, (b - ((a - c) * 0.25f * p)));
}
//An approximation method with quadratic curve fitting
//based on equations from: https://www.dsprelated.com/freebooks/sasp/Quadratic_Interpolation_Spectral_Peaks.html by JULIUS O. SMITH III
/*Fits a parabola on 3 points, and returns the parabola vertex*/
template <typename T>
std::pair<T, T> Parabolic(T ax, T ay, T bx, T by, T cx, T cy)
{
    const std::pair<T, T> p = ParabolicNormalized(ay, by, cy);
    if (p.first > 0)
    {
        return std::make_pair((bx + p.first * (cx - bx)), p.second);
    }
    return std::make_pair((bx + p.first * (bx - ax)), p.second);
}
} // namespace Approximation