
#ifndef LMMS_SFZ_SAMPLE_POOL_H
#define LMMS_SFZ_SAMPLE_POOL_H

#include "SfzSampleBuffer.h"

#include <QString>
#include <map>

namespace lmms
{

class SfzSamplePool
{
public:
	SfzSampleBuffer* const loadSample(const QString& path);

	const int sampleCount() const { return m_samplePool.size(); }

private:
	std::map<QString, std::shared_ptr<SfzSampleBuffer>> m_samplePool;
};

} // namespace lmms

#endif // LMMS_SFZ_SAMPLE_POOL_H
