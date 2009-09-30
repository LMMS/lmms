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
#include <QKeyEvent>

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
#include <QtGui/QStyle>

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

	//QPalette pal = palette();
	//pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	//setPalette( pal );
	setAutoFillBackground( true );

	setWindowTitle( tr( "FX-Mixer" ) );
	setWindowIcon( embed::getIconPixmap( "fx_mixer" ) );

	// main-layout
	QHBoxLayout * ml = new QHBoxLayout;

	// Channel area
	m_channelAreaWidget = new QWidget;
	chLayout = new QHBoxLayout(m_channelAreaWidget);
	chLayout->setSizeConstraint(QLayout::SetMinimumSize);
	chLayout->setSpacing( 0 );
	chLayout->setMargin( 0 );
	m_channelAreaWidget->setLayout(chLayout);

	// add master channel
	m_fxChannelViews.resize(m->numChannels());
	m_fxChannelViews[0] = new FxChannelView(this, this, 0);

	FxChannelView * masterView = m_fxChannelViews[0];
	ml->addWidget( masterView->m_fxLine, 0, Qt::AlignTop );

	QSize fxLineSize = masterView->m_fxLine->size();

	// add mixer channels
	for( int i = 1; i < m_fxChannelViews.size(); ++i )
	{
		m_fxChannelViews[i] = new FxChannelView(m_channelAreaWidget, this, i);
		chLayout->addWidget(m_fxChannelViews[i]->m_fxLine);
	}
	// add the scrolling section to the main layout
	channelArea = new QScrollArea(this);
	channelArea->setWidget(m_channelAreaWidget);
	channelArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	channelArea->setFrameStyle( QFrame::NoFrame );
	channelArea->setMinimumWidth( fxLineSize.width() * 6 );
	channelArea->setFixedHeight( fxLineSize.height() + 
			style()->pixelMetric( QStyle::PM_ScrollBarExtent ) );
	ml->addWidget(channelArea);

	// show the add new effect channel button
	QPushButton * newChannelBtn = new QPushButton("new", this );
	newChannelBtn->setFont(QFont("sans-serif", 10, 1, false));
	newChannelBtn->setFixedSize(fxLineSize);
	connect( newChannelBtn, SIGNAL(clicked()), this, SLOT(addNewChannel()));
	ml->addWidget( newChannelBtn, 0, Qt::AlignTop );

	
	// Create EffectRack and set initial index to master channel
	m_rackView = new EffectRackView( &m->m_fxChannels[0]->m_fxChain, this );
	ml->addWidget( m_rackView, 0, Qt::AlignTop );
	setCurrentFxLine( m_fxChannelViews[0]->m_fxLine );

	setLayout( ml );
	updateGeometry();

	// timer for updating faders
	connect( engine::mainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateFaders() ) );


	// add ourself to workspace
	QMdiSubWindow * subWin = 
		engine::mainWindow()->workspace()->addSubWindow( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	layout()->setSizeConstraint( QLayout::SetMinAndMaxSize );
	subWin->layout()->setSizeConstraint( QLayout::SetMinAndMaxSize );

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
	m_fxChannelViews.push_back(new FxChannelView(m_channelAreaWidget, this,
												 newChannelIndex));
	chLayout->addWidget(m_fxChannelViews[newChannelIndex]->m_fxLine);

	updateFxLine(newChannelIndex);
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
}


void FxMixerView::setCurrentFxLine( FxLine * _line )
{
	// select
	m_currentFxLine = _line;
	m_rackView->setModel( &engine::fxMixer()->m_fxChannels[_line->channelIndex()]->m_fxChain );

	// set up send knob
	for(int i = 0; i < m_fxChannelViews.size(); ++i)
	{
		updateFxLine(i);
	}
}


void FxMixerView::updateFxLine(int index)
{
	FxMixer * mix = engine::fxMixer();

	// does current channel send to this channel?
	int selIndex = m_currentFxLine->channelIndex();
	FxLine * thisLine = m_fxChannelViews[index]->m_fxLine;
	FloatModel * sendModel = mix->channelSendModel(selIndex, index);
	if( sendModel == NULL )
	{
		// does not send, hide send knob
		thisLine->m_sendKnob->setVisible(false);
	}
	else
	{
		// it does send, show knob and connect
		thisLine->m_sendKnob->setVisible(true);
		thisLine->m_sendKnob->setModel(sendModel);
	}

	// disable the send button if it would cause an infinite loop
	thisLine->m_sendBtn->setVisible(! mix->isInfiniteLoop(selIndex, index));
	thisLine->m_sendBtn->updateLightStatus();
	thisLine->update();
}


void FxMixerView::deleteChannel(int index)
{
	// remember selected line
	int selLine = m_currentFxLine->channelIndex();

	// can't delete master
	if( index == 0 )
		return;

	// delete the real channel
	engine::fxMixer()->deleteChannel(index);

	// delete the view
	chLayout->removeWidget(m_fxChannelViews[index]->m_fxLine);
	delete m_fxChannelViews[index]->m_fader;
	delete m_fxChannelViews[index]->m_muteBtn;
	delete m_fxChannelViews[index]->m_fxLine;
	delete m_fxChannelViews[index];

	// make sure every channel knows what index it is
	for(int i=0; i<m_fxChannelViews.size(); ++i)
	{
		if( i > index )
		{
			m_fxChannelViews[i]->m_fxLine->setChannelIndex(i-1);
		}
	}
	m_fxChannelViews.remove(index);

	// select the next channel
	if( selLine >= m_fxChannelViews.size() )
	{
		selLine = m_fxChannelViews.size()-1;
	}
	setCurrentFxLine(selLine);

}


void FxMixerView::keyPressEvent(QKeyEvent * e)
{
	switch(e->key())
	{
		case Qt::Key_Delete:
			deleteChannel(m_currentFxLine->channelIndex());
			break;
	}
}



void FxMixerView::setCurrentFxLine( int _line )
{
	setCurrentFxLine( m_fxChannelViews[_line]->m_fxLine );
}



void FxMixerView::clear()
{
	m_rackView->clearViews();
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

