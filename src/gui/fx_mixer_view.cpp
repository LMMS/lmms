#ifndef SINGLE_SOURCE_COMPILE

/*
 * fx_mixer_view.cpp - effect-mixer-view for LMMS
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


#include <QtGui/QButtonGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QStackedLayout>

#include "fx_mixer_view.h"
#include "fader.h"
#include "effect_rack_view.h"
#include "embed.h"
#include "main_window.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "song_editor.h"



class fxLine : public QWidget
{
public:
	fxLine( QWidget * _parent, fxMixerView * _mv, QString & _name ) :
		QWidget( _parent ),
		m_mv( _mv ),
		m_name( _name )
	{
		setFixedSize( 32, 232 );
		setAttribute( Qt::WA_OpaquePaintEvent, TRUE );
	}

	virtual void paintEvent( QPaintEvent * )
	{
		QPainter p( this );
		p.fillRect( rect(), QColor( 72, 76, 88 ) );
/*		p.setPen( QColor( 144, 152, 176 ) );
		p.drawLine( 0, 0, width()-1, 0 );
		p.drawLine( 0, 0, 0, height()-1 );
		p.setPen( QColor( 36, 38, 44 ) );
		p.drawLine( 0, height()-1, width()-1, height()-1 );
		p.drawLine( width()-1, 0, width()-1, height()-1 );*/
		p.setPen( QColor( 40, 42, 48 ) );
		p.drawRect( 0, 0, width()-2, height()-2 );
		p.setPen( QColor( 108, 114, 132 ) );
		p.drawRect( 1, 1, width()-2, height()-2 );
		p.setPen( QColor( 20, 24, 32 ) );
		p.drawRect( 0, 0, width()-1, height()-1 );
		
		p.rotate( -90 );
		p.setPen( m_mv->currentFxLine() == this ?
					QColor( 0, 255, 0 ) : Qt::white );
		p.setFont( pointSizeF( font(), 7.5f ) );
		p.drawText( -70, 20, m_name );
	}

	virtual void mousePressEvent( QMouseEvent * )
	{
		m_mv->setCurrentFxLine( this );
	}

	virtual void mouseDoubleClickEvent( QMouseEvent * )
	{
		bool ok;
		QString new_name = QInputDialog::getText( this,
				fxMixerView::tr( "Rename FX channel" ),
				fxMixerView::tr( "Enter the new name for this "
							"FX channel" ),
				QLineEdit::Normal, m_name, &ok );
		if( ok && !new_name.isEmpty() )
		{
			m_name = new_name;
			update();
		}
	}

private:
	fxMixerView * m_mv;
	QString & m_name;

} ;




fxMixerView::fxMixerView() :
	QWidget(),
	modelView( NULL )
{
	fxMixer * m = engine::getFxMixer();

	QPalette pal = palette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	setPalette( pal );
	setAutoFillBackground( TRUE );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

	setWindowTitle( tr( "FX-Mixer" ) );
	setWindowIcon( embed::getIconPixmap( "fx_mixer" ) );

	m_fxLineBanks = new QStackedLayout;
	m_fxLineBanks->setSpacing( 0 );
	m_fxLineBanks->setMargin( 1 );

	m_fxRacksLayout = new QStackedLayout;
	m_fxRacksLayout->setSpacing( 0 );
	m_fxRacksLayout->setMargin( 0 );

	// main-layout
	QHBoxLayout * ml = new QHBoxLayout;
	ml->setMargin( 0 );
	ml->setSpacing( 0 );
	ml->addSpacing( 6 );


	QHBoxLayout * banks[NumFxChannels/16];
	for( int i = 0; i < NumFxChannels/16; ++i )
	{
		QWidget * w = new QWidget( this );
		banks[i] = new QHBoxLayout( w );
		banks[i]->setMargin( 5 );
		banks[i]->setSpacing( 1 );
		m_fxLineBanks->addWidget( w );
	}

	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		fxChannelView * cv = &m_fxChannelViews[i];
		if( i == 0 )
		{
			cv->m_fxLine = new fxLine( NULL, this,
						m->m_fxChannels[i]->m_name  );
			ml->addWidget( cv->m_fxLine );
			ml->addSpacing( 10 );
		}
		else
		{
			const int bank = (i-1) / 16;
			cv->m_fxLine = new fxLine( NULL, this,
						m->m_fxChannels[i]->m_name );
			banks[bank]->addWidget( cv->m_fxLine );
		}
		lcdSpinBox * l = new lcdSpinBox( 2, cv->m_fxLine );
		l->model()->setRange( i, i );
		l->model()->setValue( i );
		l->move( 2, 4 );
		l->setMarginWidth( 1 );

		cv->m_fader = new fader( &m->m_fxChannels[i]->m_volumeModel,
								cv->m_fxLine );
		cv->m_fader->move( 15-cv->m_fader->width()/2,
						cv->m_fxLine->height()-130 );
		cv->m_rackView = new effectRackView(
				&m->m_fxChannels[i]->m_fxChain, this );
		m_fxRacksLayout->addWidget( cv->m_rackView );
		if( i == 0 )
		{
			QVBoxLayout * l = new QVBoxLayout;
			l->addSpacing( 10 );
			QButtonGroup * g = new QButtonGroup( this );
			m_bankButtons = g;
			g->setExclusive( TRUE );
			for( int j = 0; j < 4; ++j )
			{
				QToolButton * btn = new QToolButton;
				btn->setText( QString( 'A'+j ) );
				btn->setCheckable( TRUE );
				btn->setSizePolicy( QSizePolicy::Preferred,
						QSizePolicy::Expanding );
				l->addWidget( btn );
				g->addButton( btn, j );
				btn->setChecked( j == 0);
			}
			l->addSpacing( 10 );
			ml->addLayout( l );
			connect( g, SIGNAL( buttonClicked( int ) ),
				m_fxLineBanks, SLOT( setCurrentIndex( int ) ) );
		}
	}

	ml->addLayout( m_fxLineBanks );
	ml->addLayout( m_fxRacksLayout );

	setLayout( ml );
	updateGeometry();

	m_fxLineBanks->setCurrentIndex( 0 );
	setCurrentFxLine( m_fxChannelViews[0].m_fxLine );

	// timer for updating faders
	connect( engine::getSongEditor(), SIGNAL( periodicUpdate() ),
				this, SLOT( updateFaders() ) );


	// add ourself to workspace
	engine::getMainWindow()->workspace()->addSubWindow( this );

	// we want to receive dataChanged-signals in order to update
	setModel( m );
}




fxMixerView::~fxMixerView()
{
}




void fxMixerView::setCurrentFxLine( fxLine * _line )
{
	m_currentFxLine = _line;
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		if( m_fxChannelViews[i].m_fxLine == _line )
		{
			m_fxRacksLayout->setCurrentIndex( i );
		}
		m_fxChannelViews[i].m_fxLine->update();
	}
}



void fxMixerView::setCurrentFxLine( int _line )
{
	if ( _line >= 0 && _line < NumFxChannels+1 )
	{
		setCurrentFxLine( m_fxChannelViews[_line].m_fxLine );
		
		m_bankButtons->button( (_line-1) / 16 )->click();
	}
}




void fxMixerView::clear( void )
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		m_fxChannelViews[i].m_rackView->clear();
	}
}




void fxMixerView::updateFaders( void )
{
	fxMixer * m = engine::getFxMixer();
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		const float opl = m_fxChannelViews[i].m_fader->getPeak_L();
		const float opr = m_fxChannelViews[i].m_fader->getPeak_R();
		const float fall_off = 1.2;
		if( m->m_fxChannels[i]->m_peakLeft > opl )
		{
			m_fxChannelViews[i].m_fader->setPeak_L(
					m->m_fxChannels[i]->m_peakLeft );
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_L( opl/fall_off );
		}
		if( m->m_fxChannels[i]->m_peakRight > opr )
		{
			m_fxChannelViews[i].m_fader->setPeak_R(
					m->m_fxChannels[i]->m_peakRight );
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_R( opr/fall_off );
		}
	}
}



#include "fx_mixer_view.moc"

#endif
