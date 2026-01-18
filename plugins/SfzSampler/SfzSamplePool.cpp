



#include "SfzSamplePool.h"
#include "SampleBuffer.h"

namespace lmms
{


SfzSampleBuffer* const SfzSamplePool::loadSample(const QString& path)
{
	// If the sample has already been loaded before, just return a pointer to it
	if (m_samplePool.contains(path))
	{
		return m_samplePool.at(path).get();
	}
	else if (auto buffer = SampleBuffer::fromFile(path))
	{
		m_samplePool.insert({path, std::make_shared<SfzSampleBuffer>(buffer->data(), buffer->size(), buffer->sampleRate())});
		return m_samplePool.at(path).get();
	}
	else
	{
		return nullptr;
	}
}


} // namespace lmms

