#pragma once

template <typename T>
class Point
{
public:
    T x;
    T y;

    Point<T>(T x, T y) : x(x), y(y){};
    Point<T>() : x(0), y(0){};

    Point<T> operator*(T scalar) const
    {
        return Point<T>(x * scalar, y * scalar);
    }

    Point<T> operator+(const Point<T> other) const
    {
        return Point<T>(x + other.x, y + other.y);
    }

    Point<T> operator-(const Point<T> other) const
    {
        return Point<T>(x - other.x, y - other.y);
    }

    Point<T> operator=(const Point<T> &other)
    {
        return Point<T>(other.x, other.y);
    }
};