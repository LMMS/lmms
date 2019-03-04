/* SaSpectrumView.cpp - implementation of SaSpectrumView class.
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

#include "SaSpectrumView.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"

SaProcessor::SaProcessor() :
	m_framesFilledUp(0),
	m_energy_l(0),
	m_energy_r(0),
	m_sampleRate(1),
	m_active(true),
	m_mode_stereo(true)
{
	m_inProgress = false;
	m_spectrum_l = (fftwf_complex *) fftwf_malloc((FFT_BUFFER_SIZE + 1) * sizeof(fftwf_complex));
	m_spectrum_r = (fftwf_complex *) fftwf_malloc((FFT_BUFFER_SIZE + 1) * sizeof(fftwf_complex));
	m_fftPlan_l = fftwf_plan_dft_r2c_1d(FFT_BUFFER_SIZE * 2, m_buffer_l, m_spectrum_l, FFTW_MEASURE);
	m_fftPlan_r = fftwf_plan_dft_r2c_1d(FFT_BUFFER_SIZE * 2, m_buffer_r, m_spectrum_r, FFTW_MEASURE);

	// initialize Blackman-Harris window, constants taken from
	// https://en.wikipedia.org/wiki/Window_function#A_list_of_window_functions
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
}


SaProcessor::~SaProcessor()
{
	fftwf_destroy_plan(m_fftPlan_l);
	fftwf_destroy_plan(m_fftPlan_r);
	fftwf_free(m_spectrum_l);
	fftwf_free(m_spectrum_r);
}


void SaProcessor::analyse(sampleFrame *buf, const fpp_t frames, bool stereo)
{
	// only analyse if the view is visible
	if (m_active)
	{
		m_mode_stereo = stereo;	// remember stereo setting for later (drawing)
		m_inProgress = true;
		const int FFT_BUFFER_SIZE = 2048;
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
				m_buffer_l[m_framesFilledUp] = buf[f][0];
				m_buffer_r[m_framesFilledUp] = buf[f][1];
				m_framesFilledUp++;
			} else {
				m_buffer_l[m_framesFilledUp] = (buf[f][0] + buf[f][1]) * 0.5;
				m_buffer_r[m_framesFilledUp] = (buf[f][0] + buf[f][1]) * 0.5;
				m_framesFilledUp++;
			}
		}

		// analysis can be executed only if buffers contain enough data
		if (m_framesFilledUp < FFT_BUFFER_SIZE)
		{
			m_inProgress = false;
			return;
		}

		m_sampleRate = Engine::mixer()->processingSampleRate();
		const int LOWEST_FREQ = 0;
		const int HIGHEST_FREQ = m_sampleRate / 2;

		// apply FFT window
		for (int i = 0; i < FFT_BUFFER_SIZE; i++)
		{
			m_buffer_l[i] = m_buffer_l[i] * m_fftWindow[i];
			m_buffer_r[i] = m_buffer_r[i] * m_fftWindow[i];
		}

		// analyse spectrum of the left channel
		fftwf_execute(m_fftPlan_l);
		absspec(m_spectrum_l, m_absSpectrum_l, FFT_BUFFER_SIZE + 1);
		compressbands(m_absSpectrum_l, m_bands_l, FFT_BUFFER_SIZE + 1,
					  MAX_BANDS,
					  (int)(LOWEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)),
					  (int)(HIGHEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)));
		m_energy_l = maximum(m_bands_l, MAX_BANDS) / maximum(m_buffer_l, FFT_BUFFER_SIZE);

		// repeat analysis for right channel only if stereo processing is enabled
		if (stereo) {
			fftwf_execute(m_fftPlan_r);
			absspec(m_spectrum_r, m_absSpectrum_r, FFT_BUFFER_SIZE + 1);
			compressbands(m_absSpectrum_r, m_bands_r, FFT_BUFFER_SIZE + 1,
						  MAX_BANDS,
						  (int)(LOWEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)),
						  (int)(HIGHEST_FREQ * (FFT_BUFFER_SIZE + 1) / (float)(m_sampleRate / 2)));
			m_energy_r = maximum(m_bands_r, MAX_BANDS) / maximum(m_buffer_r, FFT_BUFFER_SIZE);
		} else {
			memset(m_bands_r, 0, sizeof(m_bands_r));
			m_energy_r = 0;
		}

		m_framesFilledUp = 0;
		m_inProgress = false;
		m_active = false;
	}
}


float SaProcessor::getEnergyL() const
{
	return m_energy_l;
}

float SaProcessor::getEnergyR() const
{
	return m_energy_r;
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


bool SaProcessor::getStereo() const
{
	return m_mode_stereo;
}

bool SaProcessor::getInProgress()
{
	return m_inProgress;
}


void SaProcessor::clear()
{
	m_framesFilledUp = 0;
	m_energy_l = 0;
	m_energy_r = 0;
	memset(m_buffer_l, 0, sizeof(m_buffer_l));
	memset(m_buffer_r, 0, sizeof(m_buffer_r));
	memset(m_bands_l, 0, sizeof(m_bands_l));
	memset(m_bands_r, 0, sizeof(m_bands_r));
}


SaSpectrumView::SaSpectrumView(SaProcessor *b, QWidget *_parent) :
	QWidget(_parent),
	m_processor(b),
	m_periodicalUpdate(false)
{
	setFixedSize(450, 200);
	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicalUpdate()));
	setAttribute(Qt::WA_TranslucentBackground, true);
	m_skipBands = MAX_BANDS * 0.5;
	float totalLength = log10(20000);
	m_pixelsPerUnitWidth = width() / totalLength;
	m_scale = 1.5;
	m_color_mono = QColor(255, 255, 255, 255);
	m_color_l = QColor(255, 255, 255, 255);
	m_color_r = QColor(255, 255, 255, 255);
	for (int i = 0; i < MAX_BANDS; i++) {
		m_bandHeight.append(0);
	}
}


void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	const float energy_l = m_processor->getEnergyL();
	const float energy_r = m_processor->getEnergyR();
	const bool stereo = m_processor->getStereo();
	const int fh = height();
	const int LOWER_Y = -36;	// dB
	const float fallOff = 1.07;

	// dont draw anything if there is no input and smoothed graph decayed
	if (energy_l <= 0 && energy_r <= 0 && m_decaySum <= 0) {		
		return;
	}

	QPainter painter(this);
	painter.setPen(QPen(m_color_mono, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.setRenderHint(QPainter::Antialiasing, true);

	// nothing new: only paint the cached path
	if (m_processor->getInProgress() || m_periodicalUpdate == false) {
		if (stereo) {
			painter.fillPath(m_path_r, QBrush(m_color_r));
			painter.drawPath(m_path_r);
			painter.fillPath(m_path_l, QBrush(m_color_l));
			painter.drawPath(m_path_l);
		} else {
			painter.fillPath(m_path_l, QBrush(m_color_mono));
			painter.drawPath(m_path_l);
		}
		return;
	}

	// otherwise update the paths with new data
	m_periodicalUpdate = false;
	m_decaySum = 0;

	float peak;
	float *bands = m_processor->m_bands_l;
	float energy = energy_l;
	QPainterPath m_path;

	for (int i = 0; i <= 1; i++){
		m_path = QPainterPath();
		m_path.moveTo(0, height());

		for (int x = 0; x < MAX_BANDS; x++, bands++) {
			peak = (fh * 2.0 / 3.0 * (20 * (log10(*bands / energy)) - LOWER_Y) / (-LOWER_Y));	//FIXME linear scale
			if (peak < 0) {
				peak = 0;
			}
			else if (peak >= fh) {
				continue;
			}

			if (peak > m_bandHeight[x] || false /*!m_mode_smooth*/) {	//FIXME make possible to turn off slow decay?
				m_bandHeight[x] = peak;
			} else {
				m_bandHeight[x] = m_bandHeight[x] / fallOff;
			}

			if (m_bandHeight[x] < 0) {
				m_bandHeight[x] = 0;
			}

			m_path.lineTo(freqToXPixel(bandToFreq(x), width()), fh - m_bandHeight[x]);
			m_decaySum += m_bandHeight[x];
		}

		m_path.lineTo(width(), height());
		m_path.closeSubpath();

		// go for second iteration only if stereo processing is enabled
		if (i == 0) {
			m_path_l = m_path;
			if (stereo) {
				bands = m_processor->m_bands_r;
				energy = energy_r;
			} else {
				break;
			}
		} else {
			m_path_r = m_path;
		}
	}

	// draw computed paths
	if (stereo) {
		painter.fillPath(m_path_r, QBrush(m_color_r));
		painter.drawPath(m_path_r);
		painter.fillPath(m_path_l, QBrush(m_color_l));
		painter.drawPath(m_path_l);
	} else {
		painter.fillPath(m_path_l, QBrush(m_color_mono));
		painter.drawPath(m_path_l);
	}
}


QColor SaSpectrumView::getColor() const
{
	return m_color_mono;
}


void SaSpectrumView::setColors(const QColor &mono, const QColor &left, const QColor &right)
{
	m_color_mono = mono;
	m_color_l = left;
	m_color_r = right;
}


float SaSpectrumView::bandToFreq(int index)
{
	return index * m_processor->getSampleRate() / (MAX_BANDS * 2);
}


float SaSpectrumView::freqToXPixel(float freq, int w)	//FIXME log / linear scale
{
	float min = log10f(20);
	float max = log10f(20000);
	float range = max - min;
	return (log10f(freq) - min) / range * w;
}

float EqHandle::gainToYPixel(float gain, int h, float pixelPerUnitHeight)
{
	return h * 0.5 - gain * pixelPerUnitHeight;
}

void SaSpectrumView::periodicalUpdate()
{
	m_periodicalUpdate = true;
	m_processor->setActive(isVisible());
	update();
}
