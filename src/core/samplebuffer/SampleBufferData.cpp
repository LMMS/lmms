#include <samplerate.h>
#include <include/Mixer.h>
#include <include/base64.h>
#include "internal/SampleBufferData.h"

internal::SampleBufferData::SampleBufferData(internal::SampleBufferData::DataVector &&data,
											 sample_rate_t sampleRate)
		: m_data{std::move(data)}, m_frequency{BaseFreq}, m_sampleRate{sampleRate} {
}

internal::SampleBufferData
internal::SampleBufferData::loadFromBase64(const QString &_data, sample_rate_t sampleRate) {
	char *dst = nullptr;
	int dsize = 0;
	base64::decode(_data, &dst, &dsize);

	SampleBufferData::DataVector input(dsize / sizeof(sampleFrame));
	memcpy(input.data(),
		   dst,
		   input.size() * sizeof(sampleFrame));

	delete[] dst;

	return SampleBufferData(std::move(input), sampleRate);
}


internal::SampleBufferData::DataVector
internal::SampleBufferData::resampleData(const SampleBufferData::DataVector &inputData, sample_rate_t inputSampleRate,
										 sample_rate_t desiredSampleRate) {
	if (inputData.empty()) {
		// no need to resample empty data
		return DataVector{};
	}
	const auto dst_frames = static_cast<f_cnt_t>( inputData.size() /
												  (float) inputSampleRate * (float) desiredSampleRate );
	DataVector outputData(dst_frames);

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE *state;
	if ((state = src_new(SRC_SINC_MEDIUM_QUALITY,
						 DEFAULT_CHANNELS, &error)) != nullptr) {
		SRC_DATA src_data;
		src_data.end_of_input = 1;
		src_data.data_in = libSampleRateSrc(inputData.data())->data();
		src_data.data_out = outputData.data()->data();
		src_data.input_frames = inputData.size();
		src_data.output_frames = dst_frames;
		src_data.src_ratio = (double) desiredSampleRate / inputSampleRate;
		if ((error = src_process(state, &src_data))) {
			printf("SampleBuffer: error while resampling: %s\n",
				   src_strerror(error));
		}
		src_delete(state);
	} else {
		printf("Error: src_new() failed in sample_buffer.cpp!\n");
	}

	return outputData;
}

float internal::SampleBufferData::getAmplification() const
{
	return m_amplification;
}

void internal::SampleBufferData::setAmplification(float amplification)
{
	m_amplification = amplification;
}

void internal::SampleBufferData::setFrequency(float frequency)
{
	m_frequency = frequency;
}

const
sampleFrame *internal::SampleBufferData::getSampleFragment(f_cnt_t _index,
														   f_cnt_t _frames, LoopMode _loopmode, sampleFrame **_tmp,
														   bool *_backwards,
														   f_cnt_t _loopstart, f_cnt_t _loopend, f_cnt_t _end) const {
	if (_loopmode == LoopOff) {
		if (_index + _frames <= _end) {
			return data() + _index;
		}
	} else if (_loopmode == LoopOn) {
		if (_index + _frames <= _loopend) {
			return data() + _index;
		}
	} else {
		if (!*_backwards && _index + _frames < _loopend) {
			return data() + _index;
		}
	}

	*_tmp = MM_ALLOC(sampleFrame, _frames);

	if (_loopmode == LoopOff) {
		f_cnt_t available = _end - _index;
		memcpy(*_tmp, data() + _index, available * BYTES_PER_FRAME);
		memset(*_tmp + available, 0, (_frames - available) *
									 BYTES_PER_FRAME);
	} else if (_loopmode == LoopOn) {
		f_cnt_t copied = qMin(_frames, _loopend - _index);
		memcpy(*_tmp, data() + _index, copied * BYTES_PER_FRAME);
		f_cnt_t loop_frames = _loopend - _loopstart;
		while (copied < _frames) {
			f_cnt_t todo = qMin(_frames - copied, loop_frames);
			memcpy(*_tmp + copied, data() + _loopstart, todo * BYTES_PER_FRAME);
			copied += todo;
		}
	} else {
		f_cnt_t pos = _index;
		bool backwards = pos < _loopstart
						 ? false
						 : *_backwards;
		f_cnt_t copied = 0;


		if (backwards) {
			copied = qMin(_frames, pos - _loopstart);
			for (int i = 0; i < copied; i++) {
				(*_tmp)[i][0] = m_data[pos - i][0];
				(*_tmp)[i][1] = m_data[pos - i][1];
			}
			pos -= copied;
			if (pos == _loopstart) backwards = false;
		} else {
			copied = qMin(_frames, _loopend - pos);
			memcpy(*_tmp, data() + pos, copied * BYTES_PER_FRAME);
			pos += copied;
			if (pos == _loopend) backwards = true;
		}

		while (copied < _frames) {
			if (backwards) {
				f_cnt_t todo = qMin(_frames - copied, pos - _loopstart);
				for (int i = 0; i < todo; i++) {
					(*_tmp)[copied + i][0] = m_data[pos - i][0];
					(*_tmp)[copied + i][1] = m_data[pos - i][1];
				}
				pos -= todo;
				copied += todo;
				if (pos <= _loopstart) backwards = false;
			} else {
				f_cnt_t todo = qMin(_frames - copied, _loopend - pos);
				memcpy(*_tmp + copied, data() + pos, todo * BYTES_PER_FRAME);
				pos += todo;
				copied += todo;
				if (pos >= _loopend) backwards = true;
			}
		}
		*_backwards = backwards;
	}

	return *_tmp;
}

float internal::SampleBufferData::getFrequency() const {
	return m_frequency;
}

sample_rate_t internal::SampleBufferData::getSampleRate() const {
	return m_sampleRate;
}
