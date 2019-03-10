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
#include <iostream>

#include "lmms_math.h"

SaProcessor::SaProcessor(SaControls *controls) :
	m_controls(controls),
	m_framesFilledUp(0),
	m_energyL(0),
	m_energyR(0),
	m_sampleRate(Engine::mixer()->processingSampleRate()),
	m_active(true),
	m_mode_stereo(true)
{
	m_inProgress = false;
	m_spectrumL = (fftwf_complex *) fftwf_malloc((FFT_BUFFER_SIZE + 1) * sizeof(fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc((FFT_BUFFER_SIZE + 1) * sizeof(fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(FFT_BUFFER_SIZE * 2, m_bufferL, m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(FFT_BUFFER_SIZE * 2, m_bufferR, m_spectrumR, FFTW_MEASURE);

	// initialize Blackman-Harris window, constants taken from
	// https://en.wikipedia.org/wiki/Window_function#AList_of_window_functions
	const float a0 = 0.35875;
	const float a1 = 0.48829;
	const float a2 = 0.14128;
	const float a3 = 0.01168;

	for (int i = 0; i < FFT_BUFFER_SIZE; i++)
	{
		m_fftWindow[i] = (a0 - a1 * cosf(2 * F_PI * i / (float)FFT_BUFFER_SIZE - 1)
							+ a2 * cosf(4 * F_PI * i / (float)FFT_BUFFER_SIZE - 1)
							- a3 * cos(6 * F_PI * i / (float)FFT_BUFFER_SIZE - 1));
	}
	clear();

	m_history.resize(WATERFALL_WIDTH * WATERFALL_HEIGHT * sizeof qRgb(0,0,0), 0);
}


SaProcessor::~SaProcessor()
{
	fftwf_destroy_plan(m_fftPlanL);
	fftwf_destroy_plan(m_fftPlanR);
	fftwf_free(m_spectrumL);
	fftwf_free(m_spectrumR);
}


void SaProcessor::analyse(sampleFrame *buf, const fpp_t frames)
{
	// only analyse if the view is visible
	if (m_active)
	{
		const bool stereo = m_controls->m_stereoModel.value();
		const int FFT_BUFFER_SIZE = 2048;

		m_inProgress = true;
		fpp_t f = 0;

		if (frames > FFT_BUFFER_SIZE)
		{
			m_framesFilledUp = 0;
			f = frames - FFT_BUFFER_SIZE;
		}

		// fill sample buffers
		for (; f < frames; f++)
		{
			if (stereo) {
				m_bufferL[m_framesFilledUp] = buf[f][0];
				m_bufferR[m_framesFilledUp] = buf[f][1];
				m_framesFilledUp++;
			} else {
				m_bufferL[m_framesFilledUp] = (buf[f][0] + buf[f][1]) * 0.5;
				m_bufferR[m_framesFilledUp] = (buf[f][0] + buf[f][1]) * 0.5;
				m_framesFilledUp++;
			}
		}

		// analysis can be executed only if buffers contain enough data
		if (m_framesFilledUp < FFT_BUFFER_SIZE)
		{
			m_inProgress = false;
			return;
		}

		// update sample rate
		m_sampleRate = Engine::mixer()->processingSampleRate();
		const int HIGHEST_FREQ = m_sampleRate / 2;

		// apply FFT window
		for (int i = 0; i < FFT_BUFFER_SIZE; i++)
		{
			m_bufferL[i] = m_bufferL[i] * m_fftWindow[i];
			m_bufferR[i] = m_bufferR[i] * m_fftWindow[i];
		}

		// analyse spectrum of the left channel
		fftwf_execute(m_fftPlanL);
		absspec(m_spectrumL, m_absSpectrumL, FFT_BUFFER_SIZE + 1);
		compressbands(m_absSpectrumL, m_bandsL, FFT_BUFFER_SIZE + 1,
					  MAX_BANDS,
					  (int)(LOWEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)),
					  (int)(HIGHEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)));
		m_energyL = maximum(m_bandsL, MAX_BANDS) / maximum(m_bufferL, FFT_BUFFER_SIZE);

		// repeat analysis for right channel only if stereo processing is enabled
		if (stereo) {
			fftwf_execute(m_fftPlanR);
			absspec(m_spectrumR, m_absSpectrumR, FFT_BUFFER_SIZE + 1);
			compressbands(m_absSpectrumR, m_bandsR, FFT_BUFFER_SIZE + 1,
						  MAX_BANDS,
						  (int)(LOWEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)),
						  (int)(HIGHEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)));
			m_energyR = maximum(m_bandsR, MAX_BANDS) / maximum(m_bufferR, FFT_BUFFER_SIZE);
		} else {
			memset(m_bandsR, 0, sizeof(m_bandsR));
			m_energyR = 0;
		}


		// move waterfall one line down and add newest result
		QRgb *pixel = (QRgb *)m_history.data();

		std::copy(	pixel,
					pixel + WATERFALL_WIDTH * WATERFALL_HEIGHT - WATERFALL_WIDTH,
					pixel + WATERFALL_WIDTH);

		for (int i = 0; i < WATERFALL_WIDTH && i < FFT_BUFFER_SIZE; i++){
			float ampL = isnan(m_energyL) ? 0 : powf(m_bandsL[i] / m_energyL, 0.45);
			float ampR = isnan(m_energyR) ? 0 : powf(m_bandsR[i] / m_energyR, 0.45);

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

std::cout << "energyL " << (int)m_energyL << "energyR " << (int)m_energyR << std::endl;

		m_framesFilledUp = 0;
		m_inProgress = false;
		m_active = false;
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


void SaProcessor::clear()
{
	m_framesFilledUp = 0;
	m_energyL = 0;
	m_energyR = 0;
	memset(m_bufferL, 0, sizeof(m_bufferL));
	memset(m_bufferR, 0, sizeof(m_bufferR));
	memset(m_bandsL, 0, sizeof(m_bandsL));
	memset(m_bandsR, 0, sizeof(m_bandsR));
}

