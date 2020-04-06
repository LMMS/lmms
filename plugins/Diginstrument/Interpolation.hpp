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
} // namespace Interpolation