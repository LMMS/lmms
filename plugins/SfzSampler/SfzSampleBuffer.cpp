

#include "SfzSampleBuffer.h"
#include "interpolation.h"

namespace lmms
{


SfzSampleBuffer::SfzSampleBuffer(const SampleFrame* data, const f_cnt_t size, const float sampleRate)
	: m_data(std::make_shared<float[][NUM_CHANNELS]>(size))
	, m_size(size)
	, m_sampleRate(sampleRate)
{
	for (f_cnt_t f = 0; f < size; ++f)
	{
		for (size_t channel = 0; channel < NUM_CHANNELS; ++channel)
		{
			m_data[f][channel] = data[f][channel];
		}
	}
}


float SfzSampleBuffer::at(const float index, const size_t channel) const
{
	if (index < 0 || index >= m_size) { return 0.0f; }

	const size_t indexFloor = static_cast<size_t>(index);

	float frac = index - indexFloor;

	size_t i0 = indexFloor == 0 ? 0 : indexFloor - 1;
	size_t i1 = indexFloor;
	size_t i2 = std::min(indexFloor + 1, m_size - 1);
	size_t i3 = std::min(indexFloor + 2, m_size - 1);
	
	float v0 = m_data[i0][channel];
	float v1 = m_data[i1][channel];
	float v2 = m_data[i2][channel];
	float v3 = m_data[i3][channel];

	return hermiteInterpolate(v0, v1, v2, v3, frac);
}


} // namespace lmms
