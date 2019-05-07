#ifndef LMMS_SAMPLEBUFFERDATA_H
#define LMMS_SAMPLEBUFFERDATA_H

#include <vector>
#include <QtCore/QString>
#include "Mixer.h"
#include "Engine.h"
#include "MemoryManager.h"
#include "lmms_basics.h"

namespace internal {
	inline sample_rate_t mixerSampleRate() {
		return Engine::mixer()->processingSampleRate();
	}

	class SampleBufferData {
	public:
		typedef std::vector<sampleFrame, MmAllocator<sampleFrame>> DataVector;

		SampleBufferData(DataVector &&data,
						 sample_rate_t sampleRate);

		SampleBufferData() = default;


		static SampleBufferData loadFromBase64(const QString &_data,
											   sample_rate_t sampleRate);

		f_cnt_t frames() const {
			return static_cast<f_cnt_t>(m_data.size());
		}

		void addData(const DataVector &vector, sample_rate_t sampleRate) {
			if (sampleRate != m_sampleRate) {
				auto resampledVector = resampleData(vector, sampleRate, m_sampleRate);
				m_data.insert(m_data.end(), resampledVector.cbegin(), resampledVector.cend());
			} else {
				m_data.insert(m_data.end(), vector.cbegin(), vector.cend());
			}
		}

		void resetData(DataVector &&newData, sample_rate_t dataSampleRate) {
			m_sampleRate = dataSampleRate;
			m_data = std::move(newData);
		}

		void reverse() {
			std::reverse(m_data.begin(), m_data.end());
		}

		static DataVector
		resampleData(const DataVector &inputData, sample_rate_t inputSampleRate, sample_rate_t desiredSampleRate);

		const
		sampleFrame *getSampleFragment(f_cnt_t _index,
									   f_cnt_t _frames, LoopMode _loopmode,
									   sampleFrame **_tmp,
									   bool *_backwards,
									   f_cnt_t _loopstart, f_cnt_t _loopend,
									   f_cnt_t _end) const;

		sampleFrame *data() {
			return m_data.data();
		}

		const
		sampleFrame *data() const {
			return m_data.data();
		}

	private:
		// HACK: libsamplerate < 0.1.9's structs does not mark read-only variables
		//	     as const. It has been fixed in 0.1.9 but has not been
		//		 shipped for some distributions.
		//		 This function just returns a variable that should have
		//		 been `const` as non-const.
		inline static sampleFrame *libSampleRateSrc(const sampleFrame *ptr) {
			return const_cast<sampleFrame *>(ptr);
		}


		DataVector m_data;
		float m_frequency = BaseFreq;
		float m_amplification = 1.0f;
		sample_rate_t m_sampleRate = internal::GetMixerSampleRate();
	public:
		float getFrequency() const {
			return 0;
		}

		sample_rate_t getSampleRate() const {
			return m_sampleRate;
		}

		void setFrequency(float frequency) {
			m_frequency = frequency;
		}

		float getAmplification() const {
			return m_amplification;
		}

		void setAmplification(float amplification) {
			m_amplification = amplification;
		}
	};
}
#endif //LMMS_SAMPLEBUFFERDATA_H
