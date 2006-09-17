#if 0
#ifndef SINGLE_SOURCE_COMPILE

/*
 * left_frame.cpp - presents the plugin selector portion of a ladspa rack
 *
 * Copyright (c) 2006 Danny McRae <khjklujn@netscape.net>
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
#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <QtCore/QString>

#else

#include <qstring.h>

#endif


#include "ladspa_description.h"
#include "mixer.h"
#include "audio_device.h"


ladspaSubPluginDescriptionWidget::ladspaSubPluginDescriptionWidget( QWidget * _parent, engine * _engine ):
	QWidget( _parent
#ifdef QT3
			, "ladspaSubPluginDescriptionWidget"
#endif
						),
	m_ladspaManager( _engine->getLADSPAManager() )
{
	m_boxer = new QVBoxLayout( this );
#ifndef QT3
	m_grouper = new QGroupBox( tr( "Description" ), this );
#else
	m_grouper = new QGroupBox( 9, Qt::Vertical, 
						tr( "Description" ), this );
#endif

	m_label = new QLabel( m_grouper );
	m_label->setText( tr( "Label:" ) );

	m_maker = new QLabel( m_grouper );
	m_maker->setText( tr( "Maker:" ) );

	m_copyright = new QLabel( m_grouper );
	m_copyright->setText( tr( "Copyright:" ) );

	m_requiresRealTime = new QLabel( m_grouper );
	m_requiresRealTime->setText( tr( "Requires Real Time:" ) );

	m_realTimeCapable = new QLabel( m_grouper );
	m_realTimeCapable->setText( tr( "Real Time Capable:" ) );

	m_inplaceBroken = new QLabel( m_grouper );
	m_inplaceBroken->setText( tr( "Inplace Broken:" ) );
	
	m_channelsIn = new QLabel( m_grouper );
	m_channelsIn->setText( tr( "Channels In:" ) );

	m_channelsOut = new QLabel( m_grouper );
	m_channelsOut->setText( tr( "Channels Out:" ) );
	
	m_boxer->addWidget( m_grouper );
}




ladspaSubPluginDescriptionWidget::~ladspaSubPluginDescriptionWidget()
{
}




void ladspaSubPluginDescriptionWidget::update( const ladspa_key_t & _key )
{
	m_label->setText( tr( "Name: " ) + 
			m_ladspaManager->getName( _key ) );
	m_maker->setText( tr( "Maker: " ) + 
			m_ladspaManager->getMaker( _key ) );
	m_copyright->setText( tr( "Copyright: " ) + 
			m_ladspaManager->getCopyright( _key ) );
	
	if( m_ladspaManager->hasRealTimeDependency( _key ) )
	{
		m_requiresRealTime->setText( tr( "Requires Real Time: Yes" ) );
	}
	else
	{
		m_requiresRealTime->setText( tr( "Requires Real Time: No" ) );
	}
	if( m_ladspaManager->isRealTimeCapable( _key ) )
	{
		m_realTimeCapable->setText( tr( "Real Time Capable: Yes" ) );
	}
	else
	{
		m_realTimeCapable->setText( tr( "Real Time Capable: No" ) );
	}		
	if( m_ladspaManager->isInplaceBroken( _key ) )
	{
		m_inplaceBroken->setText( tr( "In Place Broken: Yes" ) );
	}
	else
	{
		m_inplaceBroken->setText( tr( "In Place Broken: No" ) );
	}
	QString chan = QString::number( m_ladspaManager->getDescription( 
						_key )->inputChannels );
	m_channelsIn->setText( tr( "Channels In: " ) + chan );
	chan = QString::number( m_ladspaManager->getDescription( 
						_key )->outputChannels );
	m_channelsOut->setText( tr( "Channels Out: " ) + chan );	
}





ladspaDescription::ladspaDescription( QWidget * _parent, 
					engine * _engine, 
					ladspaPluginType _type ):
	QWidget( _parent
#ifdef QT3
			, "ladspaDescription"
#endif
						)
{
	m_ladspaManager = _engine->getLADSPAManager();
	
	setMinimumWidth( 200 );
	m_boxer = new QVBoxLayout( this );
#ifndef QT3
	m_grouper = new QGroupBox( tr( "Plugins" ), this );
#else
	m_grouper = new QGroupBox( 1, Qt::Vertical, tr( "Plugins" ), this );
#endif

	l_sortable_plugin_t plugins;
	switch( _type )
	{
		case SOURCE:
			plugins = m_ladspaManager->getInstruments();
			break;
		case TRANSFER:
			plugins = m_ladspaManager->getValidEffects();
			break;
		case VALID:
			plugins = m_ladspaManager->getValidEffects();
			break;
		case INVALID:
			plugins = m_ladspaManager->getInvalidEffects();
			break;
		case SINK:
			plugins = m_ladspaManager->getAnalysisTools();
			break;
		case OTHER:
			plugins = m_ladspaManager->getOthers();
			break;
		default:
			break;
	}
	
	for( l_sortable_plugin_t::iterator it = plugins.begin();
		    it != plugins.end(); it++ )
	{
		if( _type != VALID || 
			m_ladspaManager->getDescription( 
				(*it).second )->inputChannels <= 
				  _engine->getMixer()->audioDev()->channels() )
		{ 
			m_pluginNames.append( (*it).first );
			m_pluginKeys.append( (*it).second );
		}
	}
	
	m_pluginList = new Q3ListBox( m_grouper );
	m_pluginList->insertStringList( m_pluginNames );
	connect( m_pluginList, SIGNAL( highlighted( int ) ),
				SLOT( onHighlighted( int ) ) );	
	connect( m_pluginList, SIGNAL( doubleClicked( QListBoxItem * ) ),
				SLOT( onDoubleClicked( QListBoxItem * ) ) );
	m_boxer->addWidget( m_grouper );

	m_ladspaSubPluginDescriptionWidget = new ladspaSubPluginDescriptionWidget( this, _engine );
	connect( this, SIGNAL( highlighted( const ladspa_key_t & ) ),
			m_ladspaSubPluginDescriptionWidget, 
			SLOT( update( const ladspa_key_t & ) ) );
	m_boxer->addWidget( m_ladspaSubPluginDescriptionWidget );
	
	if( m_pluginList->numRows() > 0 )
	{
		m_pluginList->setSelected( 0, true );
		m_currentSelection = m_pluginKeys[0];
		emit( highlighted( m_currentSelection ) );
	}
}




ladspaDescription::~ladspaDescription()
{
	delete m_ladspaSubPluginDescriptionWidget;
	delete m_pluginList;
	delete m_grouper;
	delete m_boxer;
}




void ladspaDescription::onHighlighted( int _pluginIndex )
{
	m_currentSelection = m_pluginKeys[_pluginIndex];
	emit( highlighted( m_currentSelection ) );
}




void ladspaDescription::onDoubleClicked( QListBoxItem * _item )
{
	emit( doubleClicked( m_currentSelection ) );
}




void ladspaDescription::onAddButtonReleased()
{
	emit( addPlugin( m_currentSelection ) );
}


#include "ladspa_description.moc"

#endif

#endif

#endif
