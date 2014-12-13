/*
 * Fader.cpp - fader-widget used in mixer - partly taken from Hydrogen
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


#include <QInputDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include "Fader.h"
#include "embed.h"
#include "Engine.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "TextFloat.h"
#include "MainWindow.h"


TextFloat * Fader::s_textFloat = NULL;
QPixmap * Fader::s_back = NULL;
QPixmap * Fader::s_leds = NULL;
QPixmap * Fader::s_knob = NULL;

Fader::Fader( FloatModel * _model, const QString & _name, QWidget * _parent ) :
	QWidget( _parent ),
	FloatModelView( _model, this ),
	m_model( _model ),
	m_fPeakValue_L( 0.0 ),
	m_fPeakValue_R( 0.0 ),
	m_persistentPeak_L( 0.0 ),
	m_persistentPeak_R( 0.0 ),
	m_fMinPeak( 0.01f ),
	m_fMaxPeak( 1.1 ),
	m_displayConversion( true ),
	m_moveStartPoint( -1 ),
	m_startValue( 0 ),
	m_peakGreen( 0, 0, 0 ),
	m_peakRed( 0, 0, 0 )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
	}
	if( ! s_back )
	{
		s_back = new QPixmap( embed::getIconPixmap( "fader_background" ) );
	}
	if( ! s_leds )
	{
		s_leds = new QPixmap( embed::getIconPixmap( "fader_leds" ) );
	}
	if( ! s_knob )
	{
		s_knob = new QPixmap( embed::getIconPixmap( "fader_knob" ) );
	}

	m_back = s_back;
	m_leds = s_leds;
	m_knob = s_knob;

	setWindowTitle( _name );
	setAttribute( Qt::WA_OpaquePaintEvent, false );
	setMinimumSize( 23, 116 );
	setMaximumSize( 23, 116);
	resize( 23, 116 );
	setModel( _model );
	setHintText( "Volume:","%");
}


Fader::Fader( FloatModel * model, const QString & name, QWidget * parent, QPixmap * back, QPixmap * leds, QPixmap * knob ) :
	QWidget( parent ),
	FloatModelView( model, this ),
	m_model( model ),
	m_fPeakValue_L( 0.0 ),
	m_fPeakValue_R( 0.0 ),
	m_persistentPeak_L( 0.0 ),
	m_persistentPeak_R( 0.0 ),
	m_fMinPeak( 0.01f ),
	m_fMaxPeak( 1.1 ),
	m_displayConversion( false ),
	m_moveStartPoint( -1 ),
	m_startValue( 0 ),
	m_peakGreen( 0, 0, 0 ),
	m_peakRed( 0, 0, 0 )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
	}

	m_back = back;
	m_leds = leds;
	m_knob = knob;

	setWindowTitle( name );
	setAttribute( Qt::WA_OpaquePaintEvent, false );
	setMinimumSize( m_back->width(), m_back->height() );
	setMaximumSize( m_back->width(), m_back->height() );
	resize( m_back->width(), m_back->height() );
	setModel( model );
	setHintText( "Volume:","%");
}


Fader::~Fader()
{
}




void Fader::contextMenuEvent( QContextMenuEvent * _ev )
{
	CaptionMenu contextMenu( windowTitle() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
	_ev->accept();
}




void Fader::mouseMoveEvent( QMouseEvent *mouseEvent )
{
	if( m_moveStartPoint >= 0 )
	{
		int dy = m_moveStartPoint - mouseEvent->globalY();

		float delta = dy * ( m_model->maxValue() - m_model->minValue() ) / (float) ( height() - ( *s_knob ).height() );

		model()->setValue( m_startValue + delta );

		updateTextFloat();
	}
}




void Fader::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( mouseEvent->button() == Qt::LeftButton &&
			! ( mouseEvent->modifiers() & Qt::ControlModifier ) )
	{
		if( mouseEvent->y() >= knobPosY() - ( *s_knob ).height() && mouseEvent->y() < knobPosY() )
		{
			updateTextFloat();
			s_textFloat->show();

			m_moveStartPoint = mouseEvent->globalY();
			m_startValue = model()->value();

			mouseEvent->accept();
		}
		else
		{
			m_moveStartPoint = -1;
		}
	}
	else
	{
		AutomatableModelView::mousePressEvent( mouseEvent );
	}
}



void Fader::mouseDoubleClickEvent( QMouseEvent* mouseEvent )
{
	bool ok;
	float newValue;

	// TODO: dbV handling
	if( m_displayConversion )
	{
		newValue = QInputDialog::getDouble( this, windowTitle(),
					tr( "Please enter a new value between %1 and %2:" ).
							arg( model()->minValue() * 100 ).
							arg( model()->maxValue() * 100 ),
						model()->value() * 100,
						model()->minValue() * 100,
						model()->maxValue() * 100, 4, &ok ) * 0.01f;
	}
	else
	{
		newValue = QInputDialog::getDouble( this, windowTitle(),
					tr( "Please enter a new value between %1 and %2:" ).
							arg( model()->minValue() ).
							arg( model()->maxValue() ),
						model()->value(),
						model()->minValue(),
						model()->maxValue(), 4, &ok );
	}

	if( ok )
	{
		model()->setValue( newValue );
	}
}



void Fader::mouseReleaseEvent( QMouseEvent * _me )
{
	s_textFloat->hide();
}


void Fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 )
	{
		m_model->incValue( 1 );
	}
	else
	{
		m_model->incValue( -1 );
	}
	updateTextFloat();
	s_textFloat->setVisibilityTimeOut( 1000 );
}



///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak( float fPeak, float &targetPeak, float &persistentPeak, QTime &lastPeakTime )
{
	if( fPeak <  m_fMinPeak )
	{
		fPeak = m_fMinPeak;
	}
	else if( fPeak > m_fMaxPeak )
	{
		fPeak = m_fMaxPeak;
	}

	if( targetPeak != fPeak)
	{
		targetPeak = fPeak;
		if( targetPeak >= persistentPeak )
		{
			persistentPeak = targetPeak;
			lastPeakTime.restart();
		}
		update();
	}

	if( persistentPeak > 0 && lastPeakTime.elapsed() > 1500 )
	{
		persistentPeak = qMax<float>( 0, persistentPeak-0.05 );
		update();
	}
}



void Fader::setPeak_L( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_L, m_persistentPeak_L, m_lastPeakTime_L );
}



void Fader::setPeak_R( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_R, m_persistentPeak_R, m_lastPeakTime_R );
}



// update tooltip showing value and adjust position while changing fader value
void Fader::updateTextFloat()
{
	if( ConfigManager::inst()->value( "app", "displaydbv" ).toInt() && m_displayConversion )
	{
		s_textFloat->setText( QString("Volume: %1 dBV").
				arg( 20.0 * log10( model()->value() ), 3, 'f', 2 ) );
	}
	else
	{
		s_textFloat->setText( m_description + " " + QString("%1 ").arg( m_displayConversion ? m_model->value() * 100 : m_model->value() ) + " " + m_unit );
	}
	s_textFloat->moveGlobal( this, QPoint( width() - ( *m_knob ).width() - 5, knobPosY() - 46 ) );
}


inline int Fader::calculateDisplayPeak( float fPeak )
{
	int peak = (int)( m_back->height() - ( fPeak / ( m_fMaxPeak - m_fMinPeak ) ) * m_back->height() );

	return qMin( peak, m_back->height() );
}

void Fader::paintEvent( QPaintEvent * ev)
{
	QPainter painter(this);

	// background
	painter.drawPixmap( ev->rect(), *m_back, ev->rect() );


	// peak leds
	//float fRange = abs( m_fMaxPeak ) + abs( m_fMinPeak );

	int peak_L = calculateDisplayPeak( m_fPeakValue_L - m_fMinPeak );
	int persistentPeak_L = qMax<int>( 3, calculateDisplayPeak( m_persistentPeak_L - m_fMinPeak ) );
	painter.drawPixmap( QRect( 0, peak_L, 11, 116 - peak_L ), *m_leds, QRect( 0, peak_L, 11, 116 - peak_L ) );

	if( m_persistentPeak_L > 0.05 )
	{
		painter.fillRect( QRect( 2, persistentPeak_L, 7, 1 ), ( m_persistentPeak_L < 1.0 )
			? peakGreen()
			: peakRed() );
	}

	int peak_R = calculateDisplayPeak( m_fPeakValue_R - m_fMinPeak );
	int persistentPeak_R = qMax<int>( 3, calculateDisplayPeak( m_persistentPeak_R - m_fMinPeak ) );
	painter.drawPixmap( QRect( 11, peak_R, 11, 116 - peak_R ), *m_leds, QRect( 11, peak_R, 11, 116 - peak_R ) );

	if( m_persistentPeak_R > 0.05 )
	{
		painter.fillRect( QRect( 14, persistentPeak_R, 7, 1 ), ( m_persistentPeak_R < 1.0 )
			? peakGreen()
			: peakRed() );
	}

	// knob
	painter.drawPixmap( 0, knobPosY() - ( *m_knob ).height(), *s_knob );
}


QColor Fader::peakGreen() const
{
	return m_peakGreen;
}

QColor Fader::peakRed() const
{
	return m_peakRed;
}

void Fader::setPeakGreen( const QColor & c )
{
	m_peakGreen = c;
}

void Fader::setPeakRed( const QColor & c )
{
	m_peakRed = c;
}



