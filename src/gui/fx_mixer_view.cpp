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
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtGui/QStackedWidget>

#include "fx_mixer_view.h"
#include "fader.h"
#include "effect_rack_view.h"
#include "engine.h"
#include "embed.h"
#include "main_window.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "pixmap_button.h"

#include "fluiq/widget_container.h"



class fxLine : public QWidget
{
public:
	fxLine( QWidget * _parent, fxMixerView * _mv, QString & _name ) :
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
        QPainter painter;
        painter.begin(this);
		engine::getLmmsStyle()->drawFxLine( &painter,
				this, m_name, m_mv->currentFxLine() == this );
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
	FLUIQ::CollapsibleWidget( Qt::Vertical ),
	modelView( NULL, this ),
	serializingObjectHook()
{
	fxMixer * m = engine::getFxMixer();
	m->setHook( this );

	QPalette pal = palette();
	//pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	//setPalette( pal );
	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
	setFixedHeight( 280 );

	setWindowTitle( tr( "FX MIXER" ) );
	setWindowIcon( embed::getIconPixmap( "fx_mixer" ) );

	// main-layout
	QWidget * mainWidget = new QWidget;
	QHBoxLayout * mainLayout = new QHBoxLayout;
	mainLayout->setMargin( 5 );
	mainLayout->setSpacing( 5 );
	mainLayout->addSpacing( 0 );
	mainWidget->setLayout( mainLayout );

	m_fxRacksView = new QStackedWidget;
	m_fxRacksView->setFixedSize( 250, 250 );
	mainLayout->addWidget( m_fxRacksView, 0, Qt::AlignTop );
	mainLayout->addSpacing( 8 );

	QScrollArea * scrollArea = new QScrollArea( mainWidget );
	scrollArea->setWidget( new QWidget );
	scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	scrollArea->setFrameShape( QScrollArea::NoFrame );
	scrollArea->setFixedHeight( 250 );
	scrollArea->horizontalScrollBar()->setSingleStep( 33 );
	scrollArea->horizontalScrollBar()->setPageStep( 33 );

	QWidget * scrollAreaWidget = scrollArea->widget();
	QHBoxLayout * scrollAreaLayout = new QHBoxLayout( scrollAreaWidget );
	scrollAreaLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
	scrollAreaLayout->setMargin( 0 );
	scrollAreaLayout->setSpacing( 1 );

	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		fxChannelView * view = &m_fxChannelViews[i];
		createFxLine( i, i == 0 ? mainWidget : scrollAreaWidget );
		if( i == 0 )
		{
			mainLayout->addWidget( view->m_fxLine, 0,
								Qt::AlignTop );
			mainLayout->addSpacing( 5 );
		}
		else
		{
			scrollAreaLayout->addWidget( view->m_fxLine );
		}
		
		view->m_rackView = new effectRackView(
					&m->m_fxChannels[i]->m_fxChain );
		view->m_rackView->setFixedWidth( 240 );
		view->m_rackView->mainLayout()->setMargin( 0 );
		m_fxRacksView->addWidget( view->m_rackView );
	}

	mainLayout->addWidget( scrollArea, 0, Qt::AlignTop );

	addWidget( mainWidget );


	setCurrentFxLine( m_fxChannelViews[0].m_fxLine );

	// timer for updating faders
	connect( engine::getMainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateFaders() ) );


	engine::getMainWindow()->centralWidgetContainer()->addWidget( this );

	// we want to receive dataChanged-signals in order to update
	setModel( m );
}




fxMixerView::~fxMixerView()
{
}




void fxMixerView::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	mainWindow::saveWidgetState( this, _this );
}




void fxMixerView::loadSettings( const QDomElement & _this )
{
	mainWindow::restoreWidgetState( this, _this );
}




void fxMixerView::setCurrentFxLine( fxLine * _line )
{
	m_currentFxLine = _line;
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		if( m_fxChannelViews[i].m_fxLine == _line )
		{
			m_fxRacksView->setCurrentIndex( i );
		}
		m_fxChannelViews[i].m_fxLine->update();
	}
}



void fxMixerView::setCurrentFxLine( int _line )
{
	if ( _line >= 0 && _line < NumFxChannels+1 )
	{
		setCurrentFxLine( m_fxChannelViews[_line].m_fxLine );
	}
}




void fxMixerView::clear( void )
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		m_fxChannelViews[i].m_rackView->clearViews();
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



void fxMixerView::createFxLine( int _line, QWidget * _parent )
{
	fxMixer * m = engine::getFxMixer();
	fxChannelView * view = &m_fxChannelViews[_line];
	view->m_fxLine = new fxLine( _parent, this,
					m->m_fxChannels[_line]->m_name );

	lcdSpinBox * l = new lcdSpinBox( 2, view->m_fxLine );
	l->model()->setRange( _line, _line );
	l->model()->setValue( _line );
	l->move( 2, 4 );
	l->setMarginWidth( 1 );


	view->m_fader = new fader( &m->m_fxChannels[_line]->m_volumeModel,
							view->m_fxLine );
	view->m_fader->move( 15-view->m_fader->width()/2,
					view->m_fxLine->height()-
					view->m_fader->height()-5 );

	view->m_muteBtn = new pixmapButton( view->m_fxLine, tr( "Mute" ) );
	view->m_muteBtn->setModel( &m->m_fxChannels[_line]->m_muteModel );
	view->m_muteBtn->setActiveGraphic(
				embed::getIconPixmap( "led_off" ) );
	view->m_muteBtn->setInactiveGraphic(
				embed::getIconPixmap( "led_green" ) );
	view->m_muteBtn->setCheckable( true );
	view->m_muteBtn->move( 9, view->m_fader->y()-16 );
	toolTip::add( view->m_muteBtn, tr( "Mute this FX channel" ) );
}


#include "moc_fx_mixer_view.cxx"

