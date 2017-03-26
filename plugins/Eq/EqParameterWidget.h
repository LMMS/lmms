/*
 * eqparameterwidget.cpp - defination of EqParameterWidget class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2015 Steffen Baranowsky <BaraMGB/at/freenet/dot/de>
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


#ifndef EQPARAMETERWIDGET_H
#define EQPARAMETERWIDGET_H

#include <QWidget>

#include "EffectControls.h"
#include "EqCurve.h"
#include "TextFloat.h"

class EqControls;

class EqBand
{
public :
	EqBand();
	FloatModel *gain;
	FloatModel *res;
	FloatModel *freq;
	BoolModel *active;
	BoolModel *hp12;
	BoolModel *hp24;
	BoolModel *hp48;
	BoolModel *lp12;
	BoolModel *lp24;
	BoolModel *lp48;
	QColor color;
	int x;
	int y;
	QString name;
	float *peakL;
	float *peakR;
};




class EqParameterWidget : public QWidget
{
	Q_OBJECT
public:
	explicit EqParameterWidget( QWidget *parent = 0, EqControls * controls = 0 );
	~EqParameterWidget();
	QList<EqHandle*> *m_handleList;

	const int bandCount()
	{
		return 8;
	}

	EqBand* getBandModels( int i );
	void changeHandle( int i );

private:
	float m_pixelsPerUnitWidth;
	float m_pixelsPerUnitHeight;
	float m_pixelsPerOctave;
	float m_scale;
	int m_displayWidth, m_displayHeigth;
	EqControls *m_controls;
	EqBand *m_bands;
	EqHandle *m_handle;
	EqCurve *m_eqcurve;


private slots:
	void updateModels();
	void updateHandle();
};
#endif // EQPARAMETERWIDGET_H
