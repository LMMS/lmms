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
#include "fader.h"
#include "knob.h"
#include "EffectRackView.h"
#include "engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "pixmap_button.h"


class SendIndicator : public QWidget
{
	public:
		SendIndicator( QWidget * _parent ) :
			QWidget( _parent )
		{
			setFixedSize(23, 16);
		}

};

class FxLine : public QWidget
{
public:
	FxLine( QWidget * _parent, FxMixerView * _mv, QString & _name,
			int _channelIndex) :
		QWidget( _parent ),
		m_channelIndex( _channelIndex ),
		m_mv( _mv ),
		m_name( _name )
	{
		setFixedSize( 32, 287 );
		setAttribute( Qt::WA_OpaquePaintEvent, true );
		setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
	}

	virtual void paintEvent( QPaintEvent * )
	{
		bool sendToThis = engine::fxMixer()->channelSendModel(
			m_mv->currentFxLine()->m_channelIndex, m_channelIndex) != NULL;
		QPainter painter;
		painter.begin( this );
		engine::getLmmsStyle()->drawFxLine( &painter, this, m_name,
			m_mv->currentFxLine() == this, sendToThis );
		painter.end();
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

	knob * m_sendKnob;
	int m_channelIndex;
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

	m_fxChannelViews.resize(m->numChannels());
	channelArea = new QScrollArea(this);
	chLayout = new QHBoxLayout(channelArea);

	// add master channel
	FxChannelView * masterView = &m_fxChannelViews[0];
	addFxLine(0, this, ml);
	ml->addSpacing(5);
	QSize fxLineSize = masterView->m_fxLine->size();

	chLayout->setSizeConstraint(QLayout::SetMinimumSize);
	channelArea->setWidgetResizable(true);

	// add mixer channels
	for( int i = 1; i < m_fxChannelViews.size(); ++i )
	{
		addFxLine(i, channelArea, chLayout);
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

	setCurrentFxLine( m_fxChannelViews[0].m_fxLine );

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


void FxMixerView::addFxLine(int i, QWidget * parent, QLayout * layout)
{
	FxMixer * m = engine::fxMixer();

	FxChannelView * cv = &m_fxChannelViews[i];

	cv->m_fxLine = new FxLine( parent, this,
						   m->m_fxChannels[i]->m_name, i );
	layout->addWidget(cv->m_fxLine);

	// mixer sends knob
	cv->m_fxLine->m_sendKnob = new knob(0, cv->m_fxLine,
									  tr("Channel send amount"));
	cv->m_fxLine->m_sendKnob->move(0, 22);
	cv->m_fxLine->m_sendKnob->setVisible(false);

	// send light indicator


	// channel number
	lcdSpinBox * l = new lcdSpinBox( 2, cv->m_fxLine );
	l->model()->setRange( i, i );
	l->model()->setValue( i );
	l->move( 2, 58 );
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
	m_fxRacksLayout->addWidget( cv->m_rackView );
}


FxMixerView::~FxMixerView()
{
}



void FxMixerView::addNewChannel()
{
	// add new fx mixer channel and redraw the form.
	FxMixer * mix = engine::fxMixer();

	int newChannelIndex = mix->createChannel();
	m_fxChannelViews.push_back(FxChannelView());

	addFxLine(newChannelIndex, channelArea, chLayout);
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
	FxMixer * mix = engine::fxMixer();

	// select
	m_currentFxLine = _line;
	m_fxRacksLayout->setCurrentIndex( _line->m_channelIndex );

	// set up send knob
	for(int i = 0; i < m_fxChannelViews.size(); ++i)
	{
		// does current channel send to this channel?
		FloatModel * sendModel = mix->channelSendModel(_line->m_channelIndex, i);
		if( sendModel == NULL )
		{
			// does not send, hide send knob
			m_fxChannelViews[i].m_fxLine->m_sendKnob->setVisible(false);
		}
		else
		{
			// it does send, show knob and connect
			m_fxChannelViews[i].m_fxLine->m_sendKnob->setVisible(true);
			m_fxChannelViews[i].m_fxLine->m_sendKnob->setModel(sendModel);
		}

		m_fxChannelViews[i].m_fxLine->update();
	}
}



void FxMixerView::setCurrentFxLine( int _line )
{
	setCurrentFxLine( m_fxChannelViews[_line].m_fxLine );
}




void FxMixerView::clear()
{
	for( int i = 0; i < m_fxChannelViews.size(); ++i )
	{
		m_fxChannelViews[i].m_rackView->clearViews();
	}
}




void FxMixerView::updateFaders()
{
	FxMixer * m = engine::fxMixer();
	for( int i = 0; i < m_fxChannelViews.size(); ++i )
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



#include "moc_FxMixerView.cxx"

