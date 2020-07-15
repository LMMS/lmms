#pragma once

namespace Interpolation
{
/*Linear interpolation between a and b, at position x*/
template <typename T>
T Linear(T ax, T ay, T bx, T by, T x)
{
    if (bx == ax)
    {
        return ay;
    }
    const T pos = (x - ax) / (bx - ax);
    return (1.0 - pos) * ay + (pos)*by;
}

/* Cubic Lagrange polynomial interpolation at position x*/
template <typename T>
T CubicLagrange(T ax, T ay, T bx, T by, T cx, T cy, T dx, T dy, T x)
{
    const T la = ((x-bx)*(x-cx)*(x-dx)) / ((ax-bx)*(ax-cx)*(ax-dx));
    const T lb = ((x-ax)*(x-cx)*(x-dx)) / ((bx-ax)*(bx-cx)*(bx-dx));
    const T lc = ((x-bx)*(x-ax)*(x-dx)) / ((cx-bx)*(cx-ax)*(cx-dx));
    const T ld = ((x-bx)*(x-cx)*(x-ax)) / ((dx-bx)*(dx-cx)*(dx-ax));
    return ay*la + by*lb + cy*lc + dy* ld;
}
} // namespace Interpolation