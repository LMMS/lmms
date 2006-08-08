#ifndef SINGLE_SOURCE_COMPILE

/*
 * ladspa_browser.h - dialog to display information about installed LADSPA
 *                    plugins
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QLayout>

#else

#include <qlayout.h>

#endif

#include "ladspa_browser.h"

#include "tab_bar.h"
#include "tab_button.h"
#include "tab_widget.h"
#include "gui_templates.h"
#include "config_mgr.h"
#include "embed.h"
#include "debug.h"
#include "tooltip.h"
#include "ladspa_description.h"
#include "ladspa_port_dialog.h"
#include "effect.h"
#include "audio_device.h"
#include "buffer_allocator.h"
#include "effect_chain.h"

inline void ladspaBrowser::labelWidget( QWidget * _w, const QString & _txt )
{
	QLabel * title = new QLabel( _txt, _w );
	QFont f = title->font();
	f.setBold( TRUE );
	title->setFont( pointSize<12>( f ) );

#ifdef LMMS_DEBUG
	assert( dynamic_cast<QBoxLayout *>( _w->layout() ) != NULL );
#endif
	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 5 );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addWidget( title );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 10 );
}




ladspaBrowser::ladspaBrowser( engine * _engine ) :
	QDialog(),
	engineObject( _engine )
{
	setWindowIcon( embed::getIconPixmap( "setup_general" ) );
	setWindowTitle( tr( "LADSPA Plugin Browser" ) );
	setModal( TRUE );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
	QWidget * settings = new QWidget( this );
	QHBoxLayout * hlayout = new QHBoxLayout( settings );
	hlayout->setSpacing( 0 );
	hlayout->setMargin( 0 );

	m_tabBar = new tabBar( settings, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( TRUE );
	m_tabBar->setFixedWidth( 72 );

	QWidget * ws = new QWidget( settings );
	ws->setFixedSize( 500, 400 );

	QWidget * available = new QWidget( ws );
	available->setFixedSize( 500, 340 );
	QVBoxLayout * avl_layout = new QVBoxLayout( available );
	avl_layout->setSpacing( 0 );
	avl_layout->setMargin( 0 );
	labelWidget( available, tr( "Available Effects" ) );

	ladspaDescription * available_list = new ladspaDescription(available, _engine, VALID );
	connect( available_list, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
				SLOT( showPorts( const ladspa_key_t & ) ) );
	avl_layout->addWidget( available_list );


	QWidget * unavailable = new QWidget( ws );
	unavailable->setFixedSize( 500, 340 );
	QVBoxLayout * unavl_layout = new QVBoxLayout( unavailable );
	unavl_layout->setSpacing( 0 );
	unavl_layout->setMargin( 0 );
	labelWidget( unavailable, tr( "Unavailable Effects" ) );

	ladspaDescription * unavailable_list = new ladspaDescription(unavailable, _engine, INVALID );
	connect( unavailable_list, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
				 SLOT( showPorts( const ladspa_key_t & ) ) );
	unavl_layout->addWidget( unavailable_list );


	QWidget * instruments = new QWidget( ws );
	instruments->setFixedSize( 500, 340 );
	QVBoxLayout * inst_layout = new QVBoxLayout( instruments );
	inst_layout->setSpacing( 0 );
	inst_layout->setMargin( 0 );
	labelWidget( instruments, tr( "Instruments" ) );

	ladspaDescription * instruments_list = new ladspaDescription(instruments, _engine, SOURCE );
	connect( instruments_list, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
				 SLOT( showPorts( const ladspa_key_t  & ) ) );
	inst_layout->addWidget( instruments_list );


	QWidget * analysis = new QWidget( ws );
	analysis->setFixedSize( 500, 340 );
	QVBoxLayout * anal_layout = new QVBoxLayout( analysis );
	anal_layout->setSpacing( 0 );
	anal_layout->setMargin( 0 );
	labelWidget( analysis, tr( "Analysis Tools" ) );

	ladspaDescription * analysis_list = new ladspaDescription(analysis, _engine, SINK );
	connect( analysis_list, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
					 SLOT( showPorts( const ladspa_key_t & ) ) );
	anal_layout->addWidget( analysis_list );


	QWidget * other = new QWidget( ws );
	other->setFixedSize( 500, 340 );
	QVBoxLayout * other_layout = new QVBoxLayout( other );
	other_layout->setSpacing( 0 );
	other_layout->setMargin( 0 );
	labelWidget( other, tr( "Don't know" ) );

	ladspaDescription * other_list = new ladspaDescription(other, _engine, OTHER );
	connect( other_list, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
		 SLOT( showPorts( const ladspa_key_t & ) ) );
	other_layout->addWidget( other_list );
	
#ifndef QT4
#define setIcon setPixmap
#endif

	m_tabBar->addTab( available, tr( "Available Effects" ), 0, FALSE, TRUE 
			)->setIcon( embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( unavailable, tr( "Unavailable Effects" ), 1, FALSE, TRUE 
			)->setIcon( embed::getIconPixmap(
							"unavailable_sound" ) );
	m_tabBar->addTab( instruments, tr( "Instruments" ), 2, FALSE,
				TRUE )->setIcon( embed::getIconPixmap(
							"setup_midi" ) );
	m_tabBar->addTab( analysis, tr( "Analysis Tools" ), 3, FALSE, TRUE
			)->setIcon( embed::getIconPixmap( "analysis" ) );
	m_tabBar->addTab( other, tr( "Don't know" ), 4, TRUE, TRUE
			)->setIcon( embed::getIconPixmap( "uhoh" ) );

#undef setIcon

	m_tabBar->setActiveTab( 0 );

	hlayout->addWidget( m_tabBar );
	hlayout->addSpacing( 10 );
	hlayout->addWidget( ws );
	hlayout->addSpacing( 10 );
	hlayout->addStretch();

	QWidget * buttons = new QWidget( this );
	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );

	QPushButton * cancel_btn = new QPushButton( embed::getIconPixmap(
								"cancel" ),
							tr( "Close" ),
							buttons );
	connect( cancel_btn, SIGNAL( clicked() ), this, SLOT( reject() ) );

	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	vlayout->addWidget( settings );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( buttons );
	vlayout->addSpacing( 10 );
	vlayout->addStretch();

	show();


}




ladspaBrowser::~ladspaBrowser()
{
}




void ladspaBrowser::showPorts( const ladspa_key_t & _key )
{
	ladspaPortDialog ports( _key, eng() );
	ports.exec();
}




void ladspaBrowser::testLADSPA( const ladspa_key_t & _key )
{
	effect * eff1 = new effect( _key, eng() );
	effect * eff2 = new effect( _key, eng() );
	effectChain chain( eng() );

	chain.appendEffect( eff1 );
	chain.appendEffect( eff2 );
	
	fpab_t buf_size = eng()->getMixer()->audioDev()->channels();
	surroundSampleFrame * buffer = bufferAllocator::alloc<surroundSampleFrame>( buf_size );
	for( Uint8 i = 0; i < 100; i++ )
	{
		chain.processAudioBuffer( buffer, buf_size );
	}
	bufferAllocator::free( buffer );
}



#include "ladspa_browser.moc"

#endif

#endif
