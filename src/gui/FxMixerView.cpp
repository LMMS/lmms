/*
 * FxMixerView.cpp - effect-mixer-view for LMMS
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

#include <QtGlobal>
#include <QDebug>

#include <QtGui/QButtonGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QStackedLayout>
#include <QtGui/QScrollArea>

#include "FxMixerView.h"
#include "knob.h"
#include "engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"

FxMixerView::FxMixerView() :
	QWidget(),
	ModelView( NULL, this ),
	SerializingObjectHook()
{
	FxMixer * m = engine::fxMixer();
	m->setHook( this );

	QPalette pal = palette();
	//pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	//setPalette( pal );
	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

	setWindowTitle( tr( "FX-Mixer" ) );
	setWindowIcon( embed::getIconPixmap( "fx_mixer" ) );

	m_fxRacksLayout = new QStackedLayout;
	m_fxRacksLayout->setSpacing( 0 );
	m_fxRacksLayout->setMargin( 0 );

	// main-layout
	QHBoxLayout * ml = new QHBoxLayout;
	ml->setMargin( 0 );
	ml->setSpacing( 0 );
	ml->addSpacing( 6 );

	channelArea = new QScrollArea(this);
	chLayout = new QHBoxLayout(channelArea);

	// add master channel
	m_fxChannelViews.resize(m->numChannels());
	m_fxChannelViews[0] = new FxChannelView(this, this, 0);
	FxChannelView * masterView = m_fxChannelViews[0];
	m_fxRacksLayout->addWidget( masterView->m_rackView );

	ml->addWidget(masterView->m_fxLine);
	ml->addSpacing(5);
	QSize fxLineSize = masterView->m_fxLine->size();

	chLayout->setSizeConstraint(QLayout::SetMinimumSize);
	channelArea->setWidgetResizable(true);

	// add mixer channels
	for( int i = 1; i < m_fxChannelViews.size(); ++i )
	{
		m_fxChannelViews[i] = new FxChannelView(channelArea, this, i);
		chLayout->addWidget(m_fxChannelViews[i]->m_fxLine);
		m_fxRacksLayout->addWidget( m_fxChannelViews[i]->m_rackView );
	}
	// add the scrolling section to the main layout
	ml->addLayout(chLayout);

	// show the add new effect channel button
	QPushButton * newChannelBtn = new QPushButton("new", this );
	newChannelBtn->setFont(QFont("sans-serif", 10, 1, false));
	newChannelBtn->setFixedSize(fxLineSize);
	connect( newChannelBtn, SIGNAL(clicked()), this, SLOT(addNewChannel()));
	ml->addWidget( newChannelBtn );

	ml->addLayout( m_fxRacksLayout );


	setLayout( ml );
	updateGeometry();

	setCurrentFxLine( m_fxChannelViews[0]->m_fxLine );

	// timer for updating faders
	connect( engine::mainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateFaders() ) );


	// add ourself to workspace
	QMdiSubWindow * subWin = 
		engine::mainWindow()->workspace()->addSubWindow( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	subWin->layout()->setSizeConstraint(QLayout::SetMinimumSize);

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 5, 310 );

	// we want to receive dataChanged-signals in order to update
	setModel( m );
}

FxMixerView::~FxMixerView()
{
}



void FxMixerView::addNewChannel()
{
	// add new fx mixer channel and redraw the form.
	FxMixer * mix = engine::fxMixer();

	int newChannelIndex = mix->createChannel();
	m_fxChannelViews.push_back(new FxChannelView(channelArea, this,
												 newChannelIndex));
	chLayout->addWidget(m_fxChannelViews[newChannelIndex]->m_fxLine);
	m_fxRacksLayout->addWidget( m_fxChannelViews[newChannelIndex]->m_rackView );
}



void FxMixerView::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void FxMixerView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}


FxMixerView::FxChannelView::FxChannelView(QWidget * _parent, FxMixerView * _mv,
										  int _chIndex )
{
	m_fxLine = new FxLine(_parent, _mv, _chIndex);

	FxMixer * m = engine::fxMixer();
	m_fader = new fader( &m->effectChannel(_chIndex)->m_volumeModel,
					tr( "FX Fader %1" ).arg( _chIndex ), m_fxLine );
	m_fader->move( 15-m_fader->width()/2,
					m_fxLine->height()-
					m_fader->height()-5 );

	m_muteBtn = new pixmapButton( m_fxLine, tr( "Mute" ) );
	m_muteBtn->setModel( &m->effectChannel(_chIndex)->m_muteModel );
	m_muteBtn->setActiveGraphic(
				embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic(
				embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( true );
	m_muteBtn->move( 9,  m_fader->y()-16);
	toolTip::add( m_muteBtn, tr( "Mute this FX channel" ) );

	m_rackView = new EffectRackView(
			&m->m_fxChannels[_chIndex]->m_fxChain, _mv );
}


void FxMixerView::setCurrentFxLine( FxLine * _line )
{
	// select
	m_currentFxLine = _line;
	m_fxRacksLayout->setCurrentIndex( _line->channelIndex() );

	// set up send knob
	for(int i = 0; i < m_fxChannelViews.size(); ++i)
	{
		updateFxLine(i);
	}
}


void FxMixerView::updateFxLine(int i)
{
	FxMixer * mix = engine::fxMixer();

	// does current channel send to this channel?
	FloatModel * sendModel = mix->channelSendModel(m_currentFxLine->channelIndex(), i);
	if( sendModel == NULL )
	{
		// does not send, hide send knob
		m_fxChannelViews[i]->m_fxLine->m_sendKnob->setVisible(false);
	}
	else
	{
		// it does send, show knob and connect
		m_fxChannelViews[i]->m_fxLine->m_sendKnob->setVisible(true);
		m_fxChannelViews[i]->m_fxLine->m_sendKnob->setModel(sendModel);
	}


	m_fxChannelViews[i]->m_fxLine->update();
	m_fxChannelViews[i]->m_fxLine->m_sendBtn->updateLightStatus();
}

void FxMixerView::setCurrentFxLine( int _line )
{
	setCurrentFxLine( m_fxChannelViews[_line]->m_fxLine );
}




void FxMixerView::clear()
{
	for( int i = 0; i < m_fxChannelViews.size(); ++i )
	{
		m_fxChannelViews[i]->m_rackView->clearViews();
	}
}




void FxMixerView::updateFaders()
{
	FxMixer * m = engine::fxMixer();
	for( int i = 0; i < m_fxChannelViews.size(); ++i )
	{
		const float opl = m_fxChannelViews[i]->m_fader->getPeak_L();
		const float opr = m_fxChannelViews[i]->m_fader->getPeak_R();
		const float fall_off = 1.2;
		if( m->m_fxChannels[i]->m_peakLeft > opl )
		{
			m_fxChannelViews[i]->m_fader->setPeak_L(
					m->m_fxChannels[i]->m_peakLeft );
		}
		else
		{
			m_fxChannelViews[i]->m_fader->setPeak_L( opl/fall_off );
		}
		if( m->m_fxChannels[i]->m_peakRight > opr )
		{
			m_fxChannelViews[i]->m_fader->setPeak_R(
					m->m_fxChannels[i]->m_peakRight );
		}
		else
		{
			m_fxChannelViews[i]->m_fader->setPeak_R( opr/fall_off );
		}
	}
}



#include "moc_FxMixerView.cxx"

