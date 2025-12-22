
#ifndef LMMS_SFZ_SAMPLE_BUFFER_H
#define LMMS_SFZ_SAMPLE_BUFFER_H

#include "SampleFrame.h"
#include <memory>

namespace lmms
{

class SampleFrame;

class SfzSampleBuffer
{
public:
	SfzSampleBuffer() = default;
	SfzSampleBuffer(const SampleFrame* data, const f_cnt_t size, const float sampleRate);

	float at(const float index, const size_t channel) const;

	f_cnt_t size() const { return m_size; }

	float sampleRate() const { return m_sampleRate; }

private:
	static constexpr const size_t NUM_CHANNELS = 2;
	std::shared_ptr<float[][NUM_CHANNELS]> m_data;
	f_cnt_t m_size;
	float m_sampleRate;
};

} // namespace lmms

#endif // LMMS_SFZ_SAMPLE_BUFFER_H
