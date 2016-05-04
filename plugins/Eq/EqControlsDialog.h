/*
 * eqcontrolsdialog.h - defination of EqControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef EQCONTROLSDIALOG_H
#define EQCONTROLSDIALOG_H

#include "EffectControlDialog.h"
#include "Fader.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "EqParameterWidget.h"
#include "MainWindow.h"
#include "EqSpectrumView.h"
#include "PixmapButton.h"
#include <QLabel>
#include <QPushButton>



class EqControls;

class EqControlsDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	EqControlsDialog( EqControls* controls );
	virtual ~EqControlsDialog()
	{
	}

	EqBand * setBand(EqControls *controls);

private:
	EqControls * m_controls;
	Fader* m_inGainFader;
	Fader* m_outGainFader;
	Fader* m_gainFader;
	Knob* m_resKnob;
	Knob* m_freqKnob;
	PixmapButton* m_inSpecB;
	PixmapButton* m_outSpecB;
	PixmapButton* m_activeBox;
	PixmapButton* m_lp12Box;
	PixmapButton* m_lp24Box;
	PixmapButton* m_lp48Box;
	PixmapButton* m_hp12Box;
	PixmapButton* m_hp24Box;
	PixmapButton* m_hp48Box;
	LedCheckBox* m_analyzeBox;
	EqParameterWidget* m_parameterWidget;
	EqSpectrumView* m_inSpec;
	EqSpectrumView* m_outSpec;
	QLabel* m_freqLabel;
	QLabel* m_resLabel1;
	QLabel* m_resLabel2;
	QLabel* m_bandWidthLabel;


	virtual void mouseDoubleClickEvent(QMouseEvent *event);

	EqBand* setBand( int index, BoolModel* active, FloatModel* freq, FloatModel* res, FloatModel* gain, QColor color, QString name, float* peakL, float* peakR, BoolModel *hp12, BoolModel *hp24, BoolModel *hp48, BoolModel *lp12, BoolModel *lp24, BoolModel *lp48 )
	{
		EqBand* filterModels = m_parameterWidget->getBandModels( index );
		filterModels->active = active;
		filterModels->freq = freq;
		filterModels->res = res;
		filterModels->color = color;
		filterModels->gain = gain;
		filterModels->peakL = peakL;
		filterModels->peakR = peakR;
		filterModels->hp12 = hp12;
		filterModels->hp24 = hp24;
		filterModels->hp48 = hp48;
		filterModels->lp12 = lp12;
		filterModels->lp24 = lp24;
		filterModels->lp48 = lp48;
		return filterModels;
	}

	int m_originalHeight;

};



#endif // EQCONTROLSDIALOG_H
