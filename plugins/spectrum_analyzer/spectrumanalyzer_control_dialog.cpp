/*
 * spectrumanaylzer_control_dialog.cpp - view for spectrum analyzer
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QLayout>
#include <QtGui/QPainter>

#include "spectrum_analyzer.h"
#include "song_editor.h"
#include "led_checkbox.h"


class spectrumView : public QWidget
{
public:
	spectrumView( spectrumAnalyzer * _s, QWidget * _parent ) :
		QWidget( _parent ),
		m_sa( _s )
	{
		setFixedSize( 400, 200 );
		connect( engine::getSongEditor(), SIGNAL( periodicUpdate() ),
				this, SLOT( update() ) );
	}

	virtual ~spectrumView()
	{
	}

	virtual void paintEvent( QPaintEvent * _pe )
	{
		QPainter p( this );
		p.fillRect( rect(), Qt::black );
		const double e = m_sa->m_energy;
		if( e <= 0 )
		{
			return;
		}

		const bool lin_y = m_sa->m_saControls.m_linearYAxis.value();
		double * b = m_sa->m_bands;
		const int LOWER_Y = -60;	// dB
		int h;
		if( m_sa->m_saControls.m_linearSpec.value() )
		{
			p.setPen( QColor( 0, 255, 0 ) );
			for( int x = 0; x < MAX_BANDS; ++x, ++b )
			{
				if( lin_y )
				{
					h = height() * (*b / e );
				}
				else
				{
					h = (int)( height() * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
				}
				p.drawLine( x, height(), x, height()-h );
			}
		}
		else
		{
			for( int x = 0; x < 31; ++x, ++b )
			{
				if( lin_y )
				{
					h = height() * (*b / e );
				}
				else
				{
					h = (int)( height() * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
				}
				p.fillRect( x*13, height()-h, 11, h, QColor( 0, 255, 0 ) );
			}
		}
	}

private:
	spectrumAnalyzer * m_sa;

} ;


spectrumAnalyzerControlDialog::spectrumAnalyzerControlDialog(
					spectrumAnalyzerControls * _controls ) :
	effectControlDialog( _controls )
{
	QVBoxLayout * l = new QVBoxLayout( this );
	spectrumView * v = new spectrumView( _controls->m_effect, this );

	ledCheckBox * lin_spec = new ledCheckBox( tr( "Linear spectrum" ), this );
	ledCheckBox * lin_y = new ledCheckBox( tr( "Linear Y axis" ), this );
	lin_spec->setModel( &_controls->m_linearSpec );
	lin_y->setModel( &_controls->m_linearYAxis );
	l->addWidget( v );
	l->addWidget( lin_spec );
	l->addWidget( lin_y );

}


