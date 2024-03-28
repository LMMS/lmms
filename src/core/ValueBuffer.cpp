#include "ValueBuffer.h"

#include "interpolation.h"

namespace lmms
{


ValueBuffer::ValueBuffer(size_t length)
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
 
size_t ValueBuffer::length() const
{
	return this->size();
}

void ValueBuffer::interpolate(float start, float end_)
{
	float i = 0;
	std::generate(begin(), end(), [&]() {
		return linearInterpolate( start, end_, i++ / length());
	});
}


} // namespace lmms
