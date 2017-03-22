#include "ValueBuffer.h"

#include "interpolation.h"

ValueBuffer::ValueBuffer()
{}

ValueBuffer::ValueBuffer(int length)
	: std::vector<float>(length)
{}

void ValueBuffer::fill(float value)
{
	std::fill(begin(), end(), value);
}

float ValueBuffer::value(int offset) const
{
	return at(offset % length());
}

const float *ValueBuffer::values() const
{
	return data();
}

float *ValueBuffer::values()
{
	return data();
}

int ValueBuffer::length() const
{
	return size();
}

void ValueBuffer::interpolate(float start, float end_)
{
	float i = 0;
	std::generate(begin(), end(), [&]() {
		return linearInterpolate( start, end_, i++ / length());
	});
}
