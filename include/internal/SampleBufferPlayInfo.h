//
// Created by reflexe on 26/03/19.
//

#ifndef LMMS_SAMPLEBUFFERPLAYINFO_H
#define LMMS_SAMPLEBUFFERPLAYINFO_H

#include "Mixer.h"
#include "lmms_export.h"

namespace internal {
	class LMMS_EXPORT SampleBufferPlayInfo {
	public:
		explicit SampleBufferPlayInfo(f_cnt_t frames)
				: m_endFrame{frames}, m_loopEndFrame{frames} {
		}

		int sampleLength(sample_rate_t sampleRate) const {
			return static_cast<int>(double(m_endFrame - m_startFrame) / sampleRate * 1000);
		}

		f_cnt_t getStartFrame() const {
			return m_startFrame;
		}

		f_cnt_t getEndFrame() const {
			return m_endFrame;
		}

		f_cnt_t getLoopStartFrame() const {
			return m_loopStartFrame;
		}

		f_cnt_t getLoopEndFrame() const {
			return m_loopEndFrame;
		}

		float getAmplification() const {
			return m_amplification;
		}

		void setStartFrame(f_cnt_t startFrame) {
			m_startFrame = startFrame;
		}

		void setEndFrame(f_cnt_t endFrame) {
			m_endFrame = endFrame;
		}

		void setLoopStartFrame(f_cnt_t loopStartFrame) {
			m_loopStartFrame = loopStartFrame;
		}

		void setLoopEndFrame(f_cnt_t loopEndFrame) {
			m_loopEndFrame = loopEndFrame;
		}

		void setAmplification(float amplification) {
			m_amplification = amplification;
		}

		const QString &getMaybeAudioFile() const
		{
			return m_maybeAudioFile;
		}

		void setMaybeAudioFile(const QString &maybeAudioFile)
		{
			m_maybeAudioFile = maybeAudioFile;
		}

	private:
		f_cnt_t m_startFrame = 0;
		f_cnt_t m_endFrame = 0;
		f_cnt_t m_loopStartFrame = 0;
		f_cnt_t m_loopEndFrame = 0;
		float m_amplification = 1.0f;
		QString m_maybeAudioFile;
	};
}


#endif //LMMS_SAMPLEBUFFERPLAYINFO_H
