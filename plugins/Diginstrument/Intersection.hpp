#pragma once

namespace Intersection
{
/*Intersection point with the X axis, based on two given points*/
template <typename T>
T X(T ax, T ay, T bx, T by)
{
    const T m = (by-ay) / (bx-ax);
    return (ax-(ay/m));
}
};