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
 
#include "qt3support.h"

#ifdef QT4

#include <QtCore/QPoint>
#include <Qt/QtXml>
#include <QtCore/QMap>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QWhatsThis>

#else

#include <qpoint.h>
#include <qdom.h>
#include <qmap.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>

#endif

#include "impulse_editor.h"
#include "vibed.h"
#include "tooltip.h"
#include "oscillator.h"
#include "song_editor.h"


impulseEditor::impulseEditor( QWidget * _parent, int _x, int _y, 
					engine * _engine, Uint32 _len ) :
	QWidget( _parent/*, "impulseEditor"*/ ),
	engineObject( _engine ),
	m_sampleLength( _len ),
	m_normalizeFactor( 1.0f ),
	m_forward( TRUE )
{
	setFixedSize( 153, 124 );
	m_base = QPixmap::grabWidget( _parent, _x, _y );
#ifndef QT3
	QPalette pal = palette();
	pal.setBrush( backgroundRole(), QBrush( m_base ) );
	setPalette( pal );
#else
	setPaletteBackgroundPixmap( m_base );
#endif
	
	m_graph = new graph( this, eng() );
	m_graph->setForeground( PLUGIN_NAME::getIconPixmap( "wavegraph4" ) );
	m_graph->move( 0, 0 );	
	m_graph->setCursor( QCursor( Qt::CrossCursor ) );
	toolTip::add( m_graph, tr ( "Draw your own waveform here "
			"by dragging your mouse onto this graph" ) );

	connect( m_graph, SIGNAL ( sampleChanged( void ) ),
			this, SLOT ( sampleChanged( void ) ) );
	
	m_sinWaveBtn = new pixmapButton( this, eng() );
	m_sinWaveBtn->move( 136, 3 );
	m_sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"sin_wave_active" ) );
	m_sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"sin_wave_inactive" ) );
	m_sinWaveBtn->setChecked( TRUE );
	toolTip::add( m_sinWaveBtn,
		      tr( "Click here if you want a sine-wave for "
				"current oscillator." ) );
	connect( m_sinWaveBtn, SIGNAL (clicked ( void ) ),
			this, SLOT ( sinWaveClicked( void ) ) );

	
	m_triangleWaveBtn = new pixmapButton( this, eng() );
	m_triangleWaveBtn->move( 136, 20 );
	m_triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
	toolTip::add( m_triangleWaveBtn,
		      tr( "Click here if you want a triangle-wave "
				"for current oscillator." ) );
	connect( m_triangleWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( triangleWaveClicked( void ) ) );

	
	m_sawWaveBtn = new pixmapButton( this, eng() );
	m_sawWaveBtn->move( 136, 37 );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"saw_wave_inactive" ) );
	toolTip::add( m_sawWaveBtn,
		      tr( "Click here if you want a saw-wave for "
				"current oscillator." ) );
	connect( m_sawWaveBtn, SIGNAL (clicked ( void ) ),
			this, SLOT ( sawWaveClicked( void ) ) );

	
	m_sqrWaveBtn = new pixmapButton( this, eng() );
	m_sqrWaveBtn->move( 136, 54 );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"square_wave_inactive" ) );
	toolTip::add( m_sqrWaveBtn,
		      tr( "Click here if you want a square-wave for "
				"current oscillator." ) );
	connect( m_sqrWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( sqrWaveClicked( void ) ) );

	
	m_whiteNoiseWaveBtn = new pixmapButton( this, eng() );
	m_whiteNoiseWaveBtn->move( 136, 71 );
	m_whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
	toolTip::add( m_whiteNoiseWaveBtn,
			tr( "Click here if you want a white-noise for "
				"current oscillator." ) );
	connect( m_whiteNoiseWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( noiseWaveClicked( void ) ) );

	
	m_usrWaveBtn = new pixmapButton( this, eng() );
	m_usrWaveBtn->move( 136, 88 );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
				"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
				"usr_wave_inactive" ) );
	toolTip::add( m_usrWaveBtn,
		tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );
	connect( m_usrWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( usrWaveClicked( void ) ) );

	
	m_smoothBtn = new pixmapButton( this, eng() );
	m_smoothBtn->move( 3, 108 );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"smooth_inactive" ) );
	m_smoothBtn->setChecked( FALSE );
	toolTip::add( m_smoothBtn,
		      tr( "Click here to smooth waveform." ) );
	connect( m_smoothBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( smoothClicked( void ) ) );

	
	m_normalizeBtn = new pixmapButton( this, eng() );
	m_normalizeBtn->move( 20, 108 );
	m_normalizeBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_active" ) );
	m_normalizeBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
			"normalize_inactive" ) );
	m_normalizeBtn->setChecked( FALSE );
	toolTip::add( m_normalizeBtn,
			tr( "Click here to normalize waveform." ) );

	connect( m_normalizeBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( normalizeClicked( void ) ) );

 	m_state = new ledCheckBox( "", this, eng() );
 	m_state->move( 136, 109 );
 	m_state->setChecked( TRUE );
	toolTip::add( m_state,
		      tr( "Click here to enable/disable waveform." ) );
	
	m_sampleShape = new float[m_sampleLength];
	m_graph->setSamplePointer( m_sampleShape, m_sampleLength );
	
	m_lastBtn = m_sinWaveBtn;
	emit( sawWaveClicked() );
	
	move( _x, _y );

}




impulseEditor::~impulseEditor()
{
}




void impulseEditor::sinWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_sinWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// generate a Sinus wave using static oscillator-method
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = oscillator::sinSample( i /
				static_cast<float>( m_sampleLength ) );
	}
	
	sampleChanged();
}




void impulseEditor::triangleWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_triangleWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// generate a Triangle wave using static oscillator-method
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = oscillator::triangleSample( i /
				static_cast<float>( m_sampleLength ) );
	}
	
	sampleChanged();

}




void impulseEditor::sawWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_sawWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// generate a Saw wave using static oscillator-method
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = oscillator::sawSample( i /
				static_cast<float>( m_sampleLength ) );
	}

	sampleChanged();
}




void impulseEditor::sqrWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_sqrWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// generate a Sqr wave using static oscillator-method
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = oscillator::squareSample( i /
				static_cast<float>( m_sampleLength ) );
	}

	sampleChanged();
}




void impulseEditor::noiseWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE);
	m_lastBtn = m_whiteNoiseWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// generate a Noise wave using static oscillator-method
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = oscillator::noiseSample( i /
				static_cast<float>( m_sampleLength ) );
	}

	sampleChanged();

}




void impulseEditor::usrWaveClicked( void )
{
	m_lastBtn->setChecked( FALSE );
	m_lastBtn = m_usrWaveBtn;
	m_lastBtn->setChecked( TRUE );
	// zero sample_shape
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = 0;
	}

	// load user shape
	sampleBuffer buffer( eng() );
	QString af = buffer.openAudioFile();
	if( af != "" )
	{
		buffer.setAudioFile( af );
		
		// copy buffer data
		if( m_sampleLength < static_cast<Uint32>( buffer.frames() ) )
		{
			m_sampleLength = m_sampleLength;
		}
		else
		{
			m_sampleLength = static_cast<int>( buffer.frames() );
		}
		for( Uint32 i = 0; i < m_sampleLength; i++ )
		{
			m_sampleShape[i] = static_cast<float>(
							buffer.data()[0][i] );
		}
	}
	
	sampleChanged();
}




void impulseEditor::smoothClicked( void )
{
	m_smoothBtn->setChecked( TRUE );
	m_smoothBtn->update();

	float* temp = new float[m_sampleLength];
	memcpy( temp, m_sampleShape, sizeof( float ) * m_sampleLength );

	// Smoothing
	m_sampleShape[0] = temp[0] / 2.0f;
	for( Uint32 i = 1; i < m_sampleLength - 1; i++ )
	{
		m_sampleShape[i] = ( temp[i - 1] + 
					temp[i] + 
					temp[i + 1] ) / 3.0f;
	}
	m_sampleShape[m_sampleLength - 1] = temp[m_sampleLength - 1] / 2.0f;
	m_forward = FALSE;
	
	// Clean up
	delete[] temp;

	// paint
	update();
	m_graph->update();

	eng()->getSongEditor()->setModified();
	
	m_smoothBtn->setChecked( FALSE );
	m_smoothBtn->update();
}




void impulseEditor::normalizeClicked( void )
{
	m_normalizeBtn->setChecked( TRUE );
	m_normalizeBtn->update();
	
	float max = 0.0001f;
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		if( fabsf(m_sampleShape[i]) > max && m_sampleShape[i] != 0.0f )
		{ 
			max = fabs( m_sampleShape[i] );
		}
	}
	m_normalizeFactor = max;
	
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] /= m_normalizeFactor;
	}
	
	update();
	m_graph->update();
	
	eng()->getSongEditor()->setModified();
	
	m_normalizeBtn->setChecked( FALSE );
	m_normalizeBtn->update();
}	




void impulseEditor::sampleChanged()
{
	// analyze
	float max = 0.0001f;
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		if( fabsf(m_sampleShape[i]) > max && m_sampleShape[i] != 0.0f )
		{ 
			max = fabs( m_sampleShape[i] );
		}
	}
	m_normalizeFactor = max;

	// update
	if( m_graph != NULL )
	{
		m_graph->update();
	}

	eng()->getSongEditor()->setModified();
}




void impulseEditor::setOn( bool _on )
{
	if( _on )
	{
		m_state->setChecked( TRUE );
	}
	else
	{
		m_state->setChecked( FALSE );
	}
}




void impulseEditor::contextMenuEvent( QContextMenuEvent * )
{
	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( "Impulse Editor" ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
			       this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
}




void impulseEditor::displayHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
			      whatsThis() );
#else
	QWhatsThis::display( QWhatsThis::textFor( this ), mapToGlobal(
						rect().bottomRight() ) );
#endif
}




void FASTCALL impulseEditor::setValues( float * _shape )
{
	for( Uint32 i = 0; i < m_sampleLength; i++ )
	{
		m_sampleShape[i] = _shape[i];
	}
}




#include "impulse_editor.moc"

