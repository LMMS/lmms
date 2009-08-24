/*
 * fader.cpp - fader-widget used in mixer - partly taken from Hydrogen
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


#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include "fader.h"
#include "embed.h"
#include "engine.h"
#include "caption_menu.h"
#include "MainWindow.h"



fader::fader( FloatModel * _model, const QString & _name, QWidget * _parent ) :
	QWidget( _parent ),
	FloatModelView( _model, this ),
	m_model( _model ),
	m_fPeakValue_L( 0.0 ),
	m_fPeakValue_R( 0.0 ),
	m_fMinPeak( 0.01f ),
	m_fMaxPeak( 1.1 ),
	m_back( embed::getIconPixmap( "fader_background" ) ),
	m_leds( embed::getIconPixmap( "fader_leds" ) ),
	m_knob( embed::getIconPixmap( "fader_knob" ) )
{
	setAccessibleName( _name );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setMinimumSize( 23, 116 );
	setMaximumSize( 23, 116);
	resize( 23, 116 );
	setModel( _model );
}



fader::~fader()
{
}




void fader::contextMenuEvent( QContextMenuEvent * _ev )
{
	captionMenu contextMenu( accessibleName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
	_ev->accept();
}




void fader::mouseMoveEvent( QMouseEvent *ev )
{
	float fVal = (float)( height() - ev->y() ) / (float)height();
	fVal = fVal * ( m_model->maxValue() - m_model->minValue() );

	fVal = fVal + m_model->minValue();

	m_model->setValue( fVal );
}




void fader::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			! ( _me->modifiers() & Qt::ControlModifier ) )
	{
		mouseMoveEvent( _me );
		_me->accept();
	}
	else
	{
		AutomatableModelView::mousePressEvent( _me );
	}
}




void fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 )
	{
		m_model->incValue( 5 );
	}
	else
	{
		m_model->incValue( -5 );
	}
}



///
/// Set peak value (0.0 .. 1.0)
///
void fader::setPeak_L( float fPeak )
{
	if ( fPeak <  m_fMinPeak ) {
		fPeak = m_fMinPeak;
	}
	else if ( fPeak > m_fMaxPeak ) {
		fPeak = m_fMaxPeak;
	}

	if ( m_fPeakValue_L != fPeak) {
		m_fPeakValue_L = fPeak;
		update();
	}
}




///
/// Set peak value (0.0 .. 1.0)
///
void fader::setPeak_R( float fPeak )
{
	if ( fPeak <  m_fMinPeak ) {
		fPeak = m_fMinPeak;
	}
	else if ( fPeak > m_fMaxPeak ) {
		fPeak = m_fMaxPeak;
	}

	if ( m_fPeakValue_R != fPeak ) {
		m_fPeakValue_R = fPeak;
		update();
	}
}




void fader::paintEvent( QPaintEvent * ev)
{
	QPainter painter(this);

	// background
//	painter.drawPixmap( rect(), m_back, QRect( 0, 0, 23, 116 ) );
	painter.drawPixmap( ev->rect(), m_back, ev->rect() );


	// peak leds
	//float fRange = abs( m_fMaxPeak ) + abs( m_fMinPeak );

	float realPeak_L = m_fPeakValue_L - m_fMinPeak;
	//int peak_L = 116 - ( realPeak_L / fRange ) * 116.0;
	int peak_L = (int)( 116 - ( realPeak_L / ( m_fMaxPeak - m_fMinPeak ) ) *
									116.0 );

	if ( peak_L > 116 ) {
		peak_L = 116;
	}
	painter.drawPixmap( QRect( 0, peak_L, 11, 116 - peak_L ), m_leds, QRect( 0, peak_L, 11, 116 - peak_L ) );


	float realPeak_R = m_fPeakValue_R - m_fMinPeak;
	int peak_R = (int)( 116 - ( realPeak_R / ( m_fMaxPeak - m_fMinPeak ) ) *
									116.0 );
	if ( peak_R > 116 ) {
		peak_R = 116;
	}
	painter.drawPixmap( QRect( 11, peak_R, 11, 116 - peak_R ), m_leds, QRect( 11, peak_R, 11, 116 - peak_R ) );

	// knob
	const uint knob_height = m_knob.height();
	const uint knob_width = m_knob.width();

	float fRange = m_model->maxValue() - m_model->minValue();

	float realVal = m_model->value() - m_model->minValue();

//		uint knob_y = (uint)( 116.0 - ( 86.0 * ( m_model->value() / fRange ) ) );
	uint knob_y = (uint)( 116.0 - ( (116.0-knob_height) * ( realVal / fRange ) ) );


	painter.drawPixmap( QRect( 4, knob_y - knob_height, knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
}




void fader::setMaxPeak( float fMax )
{
	m_fMaxPeak = fMax;
}




void fader::setMinPeak( float fMin )
{
	m_fMinPeak = fMin;
}



#include "moc_fader.cxx"

