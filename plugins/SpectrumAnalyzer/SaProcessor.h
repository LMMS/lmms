/* SaProcessor.h - declaration of SaProcessor class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef SAPROCESSOR_H
#define SAPROCESSOR_H

#include <atomic>
#include <fftw3.h>
#include <QMutex>
#include <QRgb>
#include <vector>



namespace lmms
{

template<class T>
class LocklessRingBuffer;

class SaControls;
class SampleFrame;


//! Receives audio data, runs FFT analysis and stores the result.
class SaProcessor
{
public:
	explicit SaProcessor(const SaControls *controls);
	virtual ~SaProcessor();

	// analysis thread and a method to terminate it
	void analyze(LocklessRingBuffer<SampleFrame> &ring_buffer);
	void terminate() {m_terminate = true;}

	// inform processor if any processing is actually required
	void setSpectrumActive(bool active);
	void setWaterfallActive(bool active);
	void flipRequest() {m_flipRequest = true;}	// request refresh of history buffer

	// configuration is taken from models in SaControls; some changes require
	// an exlicit update request (reallocation and window rebuild)
	void reallocateBuffers();
	void rebuildWindow();
	void clear();
	void clearHistory();

	const float *getSpectrumL() const {return m_normSpectrumL.data();}
	const float *getSpectrumR() const {return m_normSpectrumR.data();}
	const uchar *getHistory() const {return m_history.data();}

	// information about results and unit conversion helpers
	unsigned int inBlockSize() const {return m_inBlockSize;}
	unsigned int binCount() const;			//!< size of output (frequency domain) data block
	bool spectrumNotEmpty();				//!< check if result buffers contain any non-zero values

	unsigned int waterfallWidth() const;	//!< binCount value capped at 3840 (for display)
	unsigned int waterfallHeight() const {return m_waterfallHeight;}
	bool waterfallNotEmpty() const {return m_waterfallNotEmpty;}

	float binToFreq(unsigned int bin_index) const;
	float binBandwidth() const;

	float freqToXPixel(float frequency, unsigned int width) const;
	float xPixelToFreq(float x, unsigned int width) const;

	float ampToYPixel(float amplitude, unsigned int height) const;
	float yPixelToAmp(float y, unsigned int height) const;

	unsigned int getSampleRate() const;
	float getNyquistFreq() const;

	float getFreqRangeMin(bool linear = false) const;
	float getFreqRangeMax() const;
	float getAmpRangeMin(bool linear = false) const;
	float getAmpRangeMax() const;

	// Reallocation lock prevents the processor from changing size of its buffers.
	// It is used to keep consistent bin-to-frequency mapping while drawing the
	// spectrum and to make sure reading side does not find itself out of bounds.
	// The processor is meanwhile free to work on another block.
	QMutex m_reallocationAccess;
	// Data access lock prevents the processor from changing both size and content
	// of its buffers. It is used when writing to a result buffer, or when a friendly
	// class reads them and needs guaranteed data consistency.
	// It causes FFT analysis to be paused, so this lock should be used sparingly.
	// If using both locks at the same time, reallocation lock MUST be acquired first.
	QMutex m_dataAccess;


private:
	const SaControls *m_controls;

	// thread communication and control
	bool m_terminate;

	// currently valid configuration
	unsigned int m_zeroPadFactor = 2;		//!< use n-steps bigger FFT for given block size
	std::atomic<unsigned int> m_inBlockSize;//!< size of input (time domain) data block
	unsigned int m_fftBlockSize;			//!< size of padded block for FFT processing
	unsigned int m_sampleRate;

	// data buffers (roughly in the order of processing, from input to output)
	unsigned int m_framesFilledUp;
	std::vector<float> m_bufferL;			//!< time domain samples (left)
	std::vector<float> m_bufferR;			//!< time domain samples (right)
	std::vector<float> m_fftWindow;			//!< precomputed window function coefficients
	std::vector<float> m_filteredBufferL;	//!< time domain samples with window function applied (left)
	std::vector<float> m_filteredBufferR;	//!< time domain samples with window function applied (right)
	fftwf_plan m_fftPlanL;
	fftwf_plan m_fftPlanR;
	fftwf_complex *m_spectrumL;				//!< frequency domain samples (complex) (left)
	fftwf_complex *m_spectrumR;				//!< frequency domain samples (complex) (right)
	std::vector<float> m_absSpectrumL;		//!< frequency domain samples (absolute) (left)
	std::vector<float> m_absSpectrumR;		//!< frequency domain samples (absolute) (right)
	std::vector<float> m_normSpectrumL;		//!< frequency domain samples (normalized) (left)
	std::vector<float> m_normSpectrumR;     //!< frequency domain samples (normalized) (right)

	// spectrum history for waterfall: new normSpectrum lines are added on top
	std::vector<uchar> m_history_work;		//!< local history buffer for render
	std::vector<uchar> m_history;			//!< public buffer for reading
	bool m_flipRequest;						//!< update public buffer only when requested
	std::atomic<unsigned int> m_waterfallHeight;	//!< number of stored lines in history buffer
											// Note: high values may make it harder to see transients.
	const unsigned int m_waterfallMaxWidth = 3840;

	// book keeping
	bool m_spectrumActive;
	bool m_waterfallActive;
	std::atomic<unsigned int> m_waterfallNotEmpty;	//!< number of lines remaining visible on display
	bool m_reallocating;

	// merge L and R channels and apply gamma correction to make a spectrogram pixel
	QRgb makePixel(float left, float right) const;

	#ifdef SA_DEBUG
		unsigned int m_last_dump_time;
		unsigned int m_dump_count;
		float m_sum_execution;
		float m_max_execution;
	#endif
};


} // namespace lmms

#endif // SAPROCESSOR_H

