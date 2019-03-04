/* SaSpectrumView.h - declaration of SaSpectrumView class.
*
* Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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
#ifndef SASPECTRUMVIEW_H
#define SASPECTRUMVIEW_H

#include <QPainter>
#include <QWidget>

#include "fft_helpers.h"
#include "lmms_basics.h"
#include "lmms_math.h"


const int MAX_BANDS = 2048;
class SaProcessor
{
public:
	SaProcessor();
	virtual ~SaProcessor();

	float m_bands_l[MAX_BANDS];
	float m_bands_r[MAX_BANDS];
	bool getInProgress();
	void clear();

	void analyse(sampleFrame *buf, const fpp_t frames, bool stereo);

	float getEnergyL() const;
	float getEnergyR() const;
	int getSampleRate() const;
	bool getActive() const;
	bool getStereo() const;

	void setActive(bool active);

private:
	fftwf_plan m_fftPlan_l;
	fftwf_plan m_fftPlan_r;
	float m_buffer_l[FFT_BUFFER_SIZE*2];
	float m_buffer_r[FFT_BUFFER_SIZE*2];
	fftwf_complex * m_spectrum_l;
	fftwf_complex * m_spectrum_r;
	float m_absSpectrum_l[FFT_BUFFER_SIZE+1];
	float m_absSpectrum_r[FFT_BUFFER_SIZE+1];

	int m_framesFilledUp;
	float m_energy_l;
	float m_energy_r;
	int m_sampleRate;
	bool m_active;
	bool m_mode_stereo;
	bool m_inProgress;
	float m_fftWindow[FFT_BUFFER_SIZE];
};


class SaSpectrumView : public QWidget
{
	Q_OBJECT
public:
	explicit SaSpectrumView(SaProcessor *b, QWidget *_parent = 0);
	virtual ~SaSpectrumView(){}

	QColor getColor() const;
	void setColors(const QColor &mono, const QColor &left, const QColor &right);
	float freqToXPixel(float freq, int w);
	float gainToYPixel(float gain, int h, float pixelPerUnitHeight);


protected:
	virtual void paintEvent(QPaintEvent *event);

private slots:
	void periodicalUpdate();

private:
	QColor m_color_l;
	QColor m_color_r;
	QColor m_color_mono;
	SaProcessor *m_processor;
	QPainterPath m_path_l;
	QPainterPath m_path_r;
	float m_decaySum;
	float m_pixelsPerUnitWidth;
	float m_scale;
	int m_skipBands;
	bool m_periodicalUpdate;
	QList<float> m_bandHeight;

	float bandToFreq(int index);
};
#endif // SASPECTRUMVIEW_H
