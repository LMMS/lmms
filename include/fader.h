/*
 * fader.h - fader-widget used in FX-mixer - partly taken from Hydrogen
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef FADER_H
#define FADER_H

#include <QtCore/QTime>
#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "AutomatableModelView.h"

class textFloat;


class fader : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor peakGreen READ peakGreen WRITE setPeakGreen )
	Q_PROPERTY( QColor peakRed READ peakRed WRITE setPeakRed )
	fader( FloatModel * _model, const QString & _name, QWidget * _parent );
	virtual ~fader();

	void setPeak_L( float fPeak );
	float getPeak_L() {	return m_fPeakValue_L;	}

	void setPeak_R( float fPeak );
	float getPeak_R() {	return m_fPeakValue_R;	}

	QColor peakGreen() const;
	QColor peakRed() const;
	void setPeakGreen( const QColor & c );
	void setPeakRed( const QColor & c );

private:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent *ev );
	virtual void mouseDoubleClickEvent( QMouseEvent* mouseEvent );
	virtual void mouseMoveEvent( QMouseEvent *ev );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent *ev );
	virtual void paintEvent( QPaintEvent *ev );

	int knobPosY() const
	{
		float fRange = m_model->maxValue() - m_model->minValue();
		float realVal = m_model->value() - m_model->minValue();

		return height() - ( ( height() - ( *s_knob ).height() ) * ( realVal / fRange ) );
	}

	FloatModel * m_model;

	void setPeak( float fPeak, float &targetPeak, float &persistentPeak, QTime &lastPeakTime );
	int calculateDisplayPeak( float fPeak );

	float m_fPeakValue_L;
	float m_fPeakValue_R;
	float m_persistentPeak_L;
	float m_persistentPeak_R;
	const float m_fMinPeak;
	const float m_fMaxPeak;

	QTime m_lastPeakTime_L;
	QTime m_lastPeakTime_R;

	static QPixmap * s_back;
	static QPixmap * s_leds;
	static QPixmap * s_knob;

	int m_moveStartPoint;
	float m_startValue;

	static textFloat * s_textFloat;
	void updateTextFloat();

	QColor m_peakGreen;
	QColor m_peakRed;
} ;


#endif
