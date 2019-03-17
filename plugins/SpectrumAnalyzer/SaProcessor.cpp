/* SaProcessor.cpp - implementation of SaProcessor class.
*
* Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
*
* This file is part of LMMS - https://lmms.io
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public
* License along with this program (see COPYING); if not, write to the
* Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA.
*
*/

#include "SaProcessor.h"

#include <cmath>
#include <iostream>	//FIXME DEBUG

#include "lmms_math.h"

SaProcessor::SaProcessor(SaControls *controls) :
	m_controls(controls),
	m_framesFilledUp(0),
	m_energyL(0),
	m_energyR(0),
	m_sampleRate(Engine::mixer()->processingSampleRate()),
	m_active(false),
	m_blockSizeIndex(3),
	m_blockSize(FFT_BLOCK_SIZES[3]),
	m_inProgress(false)
{
	m_fftWindow.resize(m_blockSize, 0);
	precomputeWindow(m_fftWindow.data(), m_blockSize, BLACKMAN_HARRIS);

	m_bufferL.resize(m_blockSize, 0);
	m_bufferR.resize(m_blockSize, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(binCount() * sizeof(fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(binCount() * sizeof(fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(m_blockSize, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(m_blockSize, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	m_absSpectrumL.resize(binCount(), 0);
	m_absSpectrumR.resize(binCount(), 0);
	m_normSpectrumL.resize(binCount(), 0);
	m_normSpectrumR.resize(binCount(), 0);

	m_history.resize(binCount() * WATERFALL_HEIGHT * sizeof qRgb(0,0,0), 0);

	clear();
}


SaProcessor::~SaProcessor()
{
	fftwf_destroy_plan(m_fftPlanL);
	fftwf_destroy_plan(m_fftPlanR);
	fftwf_free(m_spectrumL);
	fftwf_free(m_spectrumR);
}


void SaProcessor::analyse(sampleFrame *in_buffer, const fpp_t frame_count)
{
	// only analyse if the view is visible and not paused
	if (m_active && !m_controls->m_pauseModel.value())
	{
		m_inProgress = true;
		const bool stereo = m_controls->m_stereoModel.value();

		// check if FFT buffers need to be reallocated while it is safe to do so
		if (m_blockSizeIndex != m_controls->m_blockSizeModel.value()){
			reallocateBuffers(m_controls->m_blockSizeModel.value());
		}

		// process data
		fpp_t frame = 0;
		while (frame < frame_count){
			// fill sample buffers
			for (; frame < frame_count && m_framesFilledUp < m_blockSize; frame++, m_framesFilledUp++)
			{
				if (stereo) {	//FIXME: predelat na case (dat do SaControls) a pridat MonoRMS
					m_bufferL[m_framesFilledUp] = in_buffer[frame][0];
					m_bufferR[m_framesFilledUp] = in_buffer[frame][1];
				} else {
					m_bufferL[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
					m_bufferR[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
				}
			}
	
			// run analysis if buffers contain enough data
			if (m_framesFilledUp < m_blockSize) {
				m_inProgress = false;
				return;
			}
	
			// update sample rate
			m_sampleRate = Engine::mixer()->processingSampleRate();
	
			// apply FFT window
			for (int i = 0; i < m_blockSize; i++) {
				m_bufferL[i] = m_bufferL[i] * m_fftWindow[i];
				m_bufferR[i] = m_bufferR[i] * m_fftWindow[i];
			}
	
			// analyse spectrum of the left channel
			fftwf_execute(m_fftPlanL);
			absspec(m_spectrumL, m_absSpectrumL.data(), binCount());
			m_energyL = maximum(m_absSpectrumL) / maximum(m_bufferL);
			normalize(m_absSpectrumL, m_energyL, m_normSpectrumL);
	
			// repeat analysis for right channel only if stereo processing is enabled
			if (stereo) {
				fftwf_execute(m_fftPlanR);
				absspec(m_spectrumR, m_absSpectrumR.data(), binCount());
				m_energyR = maximum(m_absSpectrumR) / maximum(m_bufferR);
				normalize(m_absSpectrumR, m_energyR, m_normSpectrumR);
			} else {
				memset(m_absSpectrumR.data(), 0, sizeof(m_absSpectrumR.data()));
				memset(m_normSpectrumR.data(), 0, sizeof(m_normSpectrumR.data()));
				m_energyR = 0;
			}
	
			// move waterfall history one line down and add newest result on top
			QRgb *pixel = (QRgb *)m_history.data();
			std::copy(	pixel,
						pixel + binCount() * WATERFALL_HEIGHT - binCount(),
						pixel + binCount());
	
			for (int i = 0; i < binCount(); i++) {
				// apply gamma correction to make small values more visible
				// (should be around 0.42 to 0.45 for sRGB displays)
				float ampL = powf(m_normSpectrumL[i], 0.42);
				float ampR = powf(m_normSpectrumR[i], 0.42);
	
				if (stereo) {
					pixel[i] = qRgb(m_controls->m_colorL.red() * ampL + m_controls->m_colorR.red() * ampR,
									m_controls->m_colorL.green() * ampL + m_controls->m_colorR.green() * ampR,
									m_controls->m_colorL.blue() * ampL + m_controls->m_colorR.blue() * ampR);
				} else {
					pixel[i] = qRgb(m_controls->m_colorMono.lighter().red() * ampL,
									m_controls->m_colorMono.lighter().green() * ampL,
									m_controls->m_colorMono.lighter().blue() * ampL);
				}
			}


			m_framesFilledUp = 0;
		}

		m_inProgress = false;
	}
}


float SaProcessor::getEnergyL() const
{
	return m_energyL;
}

float SaProcessor::getEnergyR() const
{
	return m_energyR;
}


int SaProcessor::getSampleRate() const
{
	return m_sampleRate;
}


bool SaProcessor::getActive() const
{
	return m_active;
}

void SaProcessor::setActive(bool active)
{
	m_active = active;
}

bool SaProcessor::getInProgress()
{
	return m_inProgress;
}


void SaProcessor::reallocateBuffers(int new_size_index)
{
	int new_size;

	if (new_size_index == m_blockSizeIndex || new_size_index < 0) {return;}

	if (new_size_index < FFT_BLOCK_SIZES.size()){
		new_size = FFT_BLOCK_SIZES[new_size_index];
	} else {
		new_size = FFT_BLOCK_SIZES.back();
	}

	int new_bins = new_size / 2;

	// in case new size is smaller, reduce reported size immediately
	// to prevent SaSpectrumView from reading disappearing memory
	if (new_size < m_blockSize) {
		m_blockSize = new_size;
	}

	clear();

	// destroy old FFT plan and free the result buffer
	fftwf_destroy_plan(m_fftPlanL);
	fftwf_destroy_plan(m_fftPlanR);
	fftwf_free(m_spectrumL);
	fftwf_free(m_spectrumR);

	// allocate new space and create new plan
	m_bufferL.resize(new_size);
	m_bufferR.resize(new_size);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(new_bins * sizeof(fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(new_bins * sizeof(fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(new_size, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(new_size, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	m_absSpectrumL.resize(binCount(), 0);
	m_absSpectrumR.resize(binCount(), 0);
	m_normSpectrumL.resize(binCount(), 0);
	m_normSpectrumR.resize(binCount(), 0);

	m_history.resize(new_bins * WATERFALL_HEIGHT * sizeof qRgb(0,0,0), 0);

	precomputeWindow(m_fftWindow.data(), new_size, (FFT_WINDOWS) m_controls->m_windowModel.value());

	// in case new size is larger (or remains the same), update reported size
	// last to prevent SaSpectrumView from reading memory that's not ready yet
	if (new_size >= m_blockSize) {
		m_blockSize = new_size;
	}

	clear();
}


void SaProcessor::clear()
{
	m_framesFilledUp = 0;
	m_energyL = 0;
	m_energyR = 0;
	memset(m_bufferL.data(), 0, sizeof(m_bufferL.data()));
	memset(m_bufferR.data(), 0, sizeof(m_bufferR.data()));
	memset(m_absSpectrumL.data(), 0, sizeof(m_absSpectrumL.data()));
	memset(m_absSpectrumR.data(), 0, sizeof(m_absSpectrumR.data()));
	memset(m_normSpectrumL.data(), 0, sizeof(m_normSpectrumL.data()));
	memset(m_normSpectrumR.data(), 0, sizeof(m_normSpectrumR.data()));
	std::cout << "cleared " << sizeof(m_normSpectrumR.data()) << " bins" << std::endl;
}

