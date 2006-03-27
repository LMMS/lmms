/*
 * impulse_editor.cpp - graphic waveform editor
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef _IMPULSE_EDITOR_H
#define _IMPULSE_EDITOR_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QPixmap>
#include <QCursor>

#else

#include <qwidget.h>
#include <qpixmap.h>
#include <qcursor.h>

#endif

#include "config.h"
#include "types.h"
#include "graph.h"
#include "pixmap_button.h"
#include "engine.h"
#include "led_checkbox.h"

class impulseEditor: public QWidget, public engineObject
{
	Q_OBJECT
public:
	impulseEditor( QWidget *parent, int _x, int _y, 
				engine * _engine, Uint32 _len = 128 );
	~impulseEditor();
	
	inline float * getValues() { return( m_sampleShape ); };
	inline bool isOn() { return( m_state->isChecked() ); };
	void FASTCALL setValues( float * _shape );
	
public slots:
	void sinWaveClicked( void );
	void triangleWaveClicked( void );
	void sawWaveClicked( void );
	void sqrWaveClicked( void );
	void noiseWaveClicked( void );
	void usrWaveClicked( void );
	void smoothClicked( void );
	void normalizeClicked( void );
	void sampleChanged();
	void setOn( bool _on );
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp( void );
	
private:

	graph * m_graph;
	pixmapButton * m_sinWaveBtn;
	pixmapButton * m_triangleWaveBtn;
	pixmapButton * m_sqrWaveBtn;
	pixmapButton * m_sawWaveBtn;
	pixmapButton * m_whiteNoiseWaveBtn;
	pixmapButton * m_usrWaveBtn;
	pixmapButton * m_smoothBtn;
	pixmapButton * m_normalizeBtn;
	pixmapButton * m_lastBtn;
	ledCheckBox * m_state;
	
	float * m_sampleShape;
	
	Uint32 m_sampleLength;
	float m_normalizeFactor;
	bool m_forward;
	
	QPixmap m_base;
};
	
#endif
