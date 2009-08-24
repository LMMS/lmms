/*
 * fader.h - fader-widget used in FX-mixer - partly taken from Hydrogen
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _FADER_H
#define _FADER_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "AutomatableModelView.h"


class fader : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	fader( FloatModel * _model, const QString & _name, QWidget * _parent );
	virtual ~fader();

	void setMaxPeak( float _max );
	void setMinPeak( float _min );

	void setPeak_L( float peak );
	float getPeak_L() {	return m_fPeakValue_L;	}

	void setPeak_R( float peak );
	float getPeak_R() {	return m_fPeakValue_R;	}


private:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent *ev );
	virtual void mouseMoveEvent( QMouseEvent *ev );
	virtual void wheelEvent( QWheelEvent *ev );
	virtual void paintEvent( QPaintEvent *ev );

	FloatModel * m_model;

	float m_fPeakValue_L;
	float m_fPeakValue_R;
	float m_fMinPeak;
	float m_fMaxPeak;

	QPixmap m_back;
	QPixmap m_leds;
	QPixmap m_knob;
} ;


#endif
