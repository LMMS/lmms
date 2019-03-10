/* SaProcessor.h - declaration of SaProcessor class.
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

#ifndef SAPROCESSOR_H
#define SAPROCESSOR_H

#include <QImage>

#include "SaControls.h"

#include "fft_helpers.h"

class SaProcessor
{
public:
	SaProcessor(SaControls *controls);
	virtual ~SaProcessor();

	float m_bandsL[MAX_BANDS];
	float m_bandsR[MAX_BANDS];
	bool getInProgress();
	void clear();

	void analyse(sampleFrame *buf, const fpp_t frames);

	float getEnergyL() const;
	float getEnergyR() const;
	int getSampleRate() const;
	bool getActive() const;

	void setActive(bool active);

private:
	SaControls *m_controls;

	fftwf_plan m_fftPlanL;
	fftwf_plan m_fftPlanR;
	float m_bufferL[FFT_BUFFER_SIZE*2];
	float m_bufferR[FFT_BUFFER_SIZE*2];
	fftwf_complex * m_spectrumL;
	fftwf_complex * m_spectrumR;
	float m_absSpectrumL[FFT_BUFFER_SIZE+1];
	float m_absSpectrumR[FFT_BUFFER_SIZE+1];

	int m_framesFilledUp;
	float m_energyL;
	float m_energyR;
	int m_sampleRate;
	bool m_active;
	bool m_mode_stereo;
	bool m_inProgress;
	float m_fftWindow[FFT_BUFFER_SIZE];

	std::vector<uchar> m_history;

	friend class SaWaterfallView;
};
#endif // SAPROCESSOR_H

