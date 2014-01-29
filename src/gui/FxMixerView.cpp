/*
 * FxMixerView.cpp - effect-mixer-view for LMMS
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QStackedLayout>

#include "FxMixerView.h"
#include "fader.h"
#include "EffectRackView.h"
#include "engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "LcdWidget.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "pixmap_button.h"



class FxLine : public QWidget
{
public:
	FxLine( QWidget * _parent, FxMixerView * _mv, QString & _name ) :
		QWidget( _parent ),
		m_mv( _mv ),
		m_name( _name )
	{
		setFixedSize( 32, 232 );
		setAttribute( Qt::WA_OpaquePaintEvent, true );
		setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
	}

	virtual void paintEvent( QPaintEvent * )
	{
		QPainter p( this );
		QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
		QColor sh_color = QApplication::palette().color( QPalette::Active,
							QPalette::Shadow );
		QColor te_color = QApplication::palette().color( QPalette::Active,
							QPalette::Text );
		QColor bt_color = QApplication::palette().color( QPalette::Active,
							QPalette::BrightText );

		p.fillRect( rect(), 
			m_mv->currentFxLine() == this ? bg_color.lighter(130) : bg_color );

		p.setPen( bg_color.darker(130) );
		p.drawRect( 0, 0, width()-2, height()-2 );

		p.setPen( bg_color.lighter(150) );
		p.drawRect( 1, 1, width()-2, height()-2 );

		p.setPen( m_mv->currentFxLine() == this ? sh_color : bg_color.darker(130) );
		p.drawRect( 0, 0, width()-1, height()-1 );

		p.rotate( -90 );
		p.setFont( pointSizeF( font(), 7.5f ) );

		p.setPen( sh_color );
		p.drawText( -91, 21, m_name );
		
		p.setPen( m_mv->currentFxLine() == this ? bt_color : te_color );
		p.drawText( -90, 20, m_name );
		
	}

	virtual void mousePressEvent( QMouseEvent * )
	{
		m_mv->setCurrentFxLine( this );
	}

	virtual void mouseDoubleClickEvent( QMouseEvent * )
	{
		bool ok;
		QString new_name = QInputDialog::getText( this,
				FxMixerView::tr( "Rename FX channel" ),
				FxMixerView::tr( "Enter the new name for this "
							"FX channel" ),
				QLineEdit::Normal, m_name, &ok );
		if( ok && !new_name.isEmpty() )
		{
			m_name = new_name;
			update();
		}
	}


private:
	FxMixerView * m_mv;
	QString & m_name;

} ;




FxMixerView::FxMixerView() :
	QWidget(),
	ModelView( NULL, this ),
	SerializingObjectHook()
{
	FxMixer * m = engine::fxMixer();
	m->setHook( this );

/*	QPalette pal = palette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	setPalette( pal );*/


	setAutoFillBackground( true );
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
		FxChannelView * cv = &m_fxChannelViews[i];
		if( i == 0 )
		{
			cv->m_fxLine = new FxLine( NULL, this,
						m->m_fxChannels[i]->m_name  );
			ml->addWidget( cv->m_fxLine );
			ml->addSpacing( 10 );
		}
		else
		{
			const int bank = (i-1) / 16;
			cv->m_fxLine = new FxLine( NULL, this,
						m->m_fxChannels[i]->m_name );
			banks[bank]->addWidget( cv->m_fxLine );
		}
		LcdWidget* l = new LcdWidget( 2, cv->m_fxLine );
		l->setValue( i );
		l->move( 3, 4 );
		l->setMarginWidth( 1 );


		cv->m_fader = new fader( &m->m_fxChannels[i]->m_volumeModel,
						tr( "FX Fader %1" ).arg( i ),
								cv->m_fxLine );
		cv->m_fader->move( 15-cv->m_fader->width()/2,
						cv->m_fxLine->height()-
						cv->m_fader->height()-5 );

		cv->m_muteBtn = new pixmapButton( cv->m_fxLine, tr( "Mute" ) );
		cv->m_muteBtn->setModel( &m->m_fxChannels[i]->m_muteModel );
		cv->m_muteBtn->setActiveGraphic(
					embed::getIconPixmap( "led_off" ) );
		cv->m_muteBtn->setInactiveGraphic(
					embed::getIconPixmap( "led_green" ) );
		cv->m_muteBtn->setCheckable( true );
		cv->m_muteBtn->move( 9,  cv->m_fader->y()-16);
		toolTip::add( cv->m_muteBtn, tr( "Mute this FX channel" ) );

		cv->m_rackView = new EffectRackView(
				&m->m_fxChannels[i]->m_fxChain, this );
		cv->m_rackView->setMinimumWidth( 244 );

		m_fxRacksLayout->addWidget( cv->m_rackView );
		if( i == 0 )
		{
			QVBoxLayout * l = new QVBoxLayout;
			l->addSpacing( 10 );
			QButtonGroup * g = new QButtonGroup( this );
			m_bankButtons = g;
			g->setExclusive( true );
			for( int j = 0; j < 4; ++j )
			{
				QToolButton * btn = new QToolButton;
				btn->setText( QString( 'A'+j ) );
				btn->setCheckable( true );
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
	connect( engine::mainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateFaders() ) );


	// add ourself to workspace
	QMdiSubWindow * subWin =
		engine::mainWindow()->workspace()->addSubWindow( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	//subWin->layout()->setSizeConstraint(QLayout::SetFixedSize);

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 5, 310 );

	// we want to receive dataChanged-signals in order to update
	setModel( m );
}




FxMixerView::~FxMixerView()
{
}




void FxMixerView::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void FxMixerView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void FxMixerView::setCurrentFxLine( FxLine * _line )
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



void FxMixerView::setCurrentFxLine( int _line )
{
	if ( _line >= 0 && _line < NumFxChannels+1 )
	{
		setCurrentFxLine( m_fxChannelViews[_line].m_fxLine );

		m_bankButtons->button( (_line-1) / 16 )->click();
	}
}




void FxMixerView::clear()
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		m_fxChannelViews[i].m_rackView->clearViews();
	}
}




void FxMixerView::updateFaders()
{
	FxMixer * m = engine::fxMixer();
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		const float opl = m_fxChannelViews[i].m_fader->getPeak_L();
		const float opr = m_fxChannelViews[i].m_fader->getPeak_R();
		const float fall_off = 1.2;
		if( m->m_fxChannels[i]->m_peakLeft > opl )
		{
			m_fxChannelViews[i].m_fader->setPeak_L( m->m_fxChannels[i]->m_peakLeft );
			m->m_fxChannels[i]->m_peakLeft = 0;
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_L( opl/fall_off );
		}

		if( m->m_fxChannels[i]->m_peakRight > opr )
		{
			m_fxChannelViews[i].m_fader->setPeak_R( m->m_fxChannels[i]->m_peakRight );
			m->m_fxChannels[i]->m_peakRight = 0;
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_R( opr/fall_off );
		}
	}
}



#include "moc_FxMixerView.cxx"

