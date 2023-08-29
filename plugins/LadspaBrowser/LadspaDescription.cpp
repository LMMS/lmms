/*
 * LadspaDescription.cpp - LADSPA plugin description
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "LadspaDescription.h"

#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QVBoxLayout>

#include "AudioDevice.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "Ladspa2LMMS.h"


namespace lmms::gui
{


LadspaDescription::LadspaDescription( QWidget * _parent,
						LadspaPluginType _type ) :
	QWidget( _parent )
{
	Ladspa2LMMS * manager = Engine::getLADSPAManager();

	l_sortable_plugin_t plugins;
	switch( _type )
	{
		case LadspaPluginType::Source:
			plugins = manager->getInstruments();
			break;
		case LadspaPluginType::Transfer:
			plugins = manager->getValidEffects();
			break;
		case LadspaPluginType::Valid:
			plugins = manager->getValidEffects();
			break;
		case LadspaPluginType::Invalid:
			plugins = manager->getInvalidEffects();
			break;
		case LadspaPluginType::Sink:
			plugins = manager->getAnalysisTools();
			break;
		case LadspaPluginType::Other:
			plugins = manager->getOthers();
			break;
		default:
			break;
	}

	QList<QString> pluginNames;
	for (const auto& plugin : plugins)
	{
		ch_cnt_t audioDeviceChannels = Engine::audioEngine()->audioDev()->channels();
		if (_type != LadspaPluginType::Valid || manager->getDescription(plugin.second)->inputChannels <= audioDeviceChannels)
		{
			pluginNames.push_back(plugin.first);
			m_pluginKeys.push_back(plugin.second);
		}
	}

	auto pluginsBox = new QGroupBox(tr("Plugins"), this);
	auto pluginList = new QListWidget(pluginsBox);
	pluginList->addItems( pluginNames );
	connect( pluginList, SIGNAL( currentRowChanged( int ) ),
						SLOT( rowChanged( int ) ) );
	connect( pluginList, SIGNAL( itemDoubleClicked( QListWidgetItem * ) ),
				SLOT( onDoubleClicked( QListWidgetItem * ) ) );
	( new QVBoxLayout( pluginsBox ) )->addWidget( pluginList );

	auto descriptionBox = new QGroupBox(tr("Description"), this);
	auto descriptionLayout = new QVBoxLayout(descriptionBox);
	descriptionLayout->setSpacing( 0 );
	descriptionLayout->setContentsMargins(0, 0, 0, 0);

	m_scrollArea = new QScrollArea( descriptionBox );
	descriptionLayout->addWidget( m_scrollArea );

	auto layout = new QVBoxLayout(this);
	layout->addWidget( pluginsBox );
	layout->addWidget( descriptionBox );

	if( pluginList->count() > 0 )
	{
		pluginList->setCurrentRow( 0 );
		m_currentSelection = m_pluginKeys[0];
		update( m_currentSelection );
	}
}




void LadspaDescription::update( const ladspa_key_t & _key )
{
	auto description = new QWidget;
	m_scrollArea->setWidget( description );

	auto layout = new QVBoxLayout(description);
	layout->setSizeConstraint( QLayout::SetFixedSize );

	Ladspa2LMMS * manager = Engine::getLADSPAManager();

	auto name = new QLabel(description);
	name->setText( QWidget::tr( "Name: " ) + manager->getName( _key ) );
	layout->addWidget( name );

	auto maker = new QWidget(description);
	auto makerLayout = new QHBoxLayout(maker);
	makerLayout->setContentsMargins(0, 0, 0, 0);
	makerLayout->setSpacing( 0 );
	layout->addWidget( maker );

	auto maker_label = new QLabel(maker);
	maker_label->setText( QWidget::tr( "Maker: " ) );
	maker_label->setAlignment( Qt::AlignTop );
	auto maker_content = new QLabel(maker);
	maker_content->setText( manager->getMaker( _key ) );
	maker_content->setWordWrap( true );
	makerLayout->addWidget( maker_label );
	makerLayout->addWidget( maker_content, 1 );

	auto copyright = new QWidget(description);
	auto copyrightLayout = new QHBoxLayout(copyright);
	copyrightLayout->setContentsMargins(0, 0, 0, 0);
	copyrightLayout->setSpacing( 0 );
	layout->addWidget( copyright );

	auto copyright_label = new QLabel(copyright);
	copyright_label->setText( QWidget::tr( "Copyright: " ) );
	copyright_label->setAlignment( Qt::AlignTop );

	auto copyright_content = new QLabel(copyright);
	copyright_content->setText( manager->getCopyright( _key ) );
	copyright_content->setWordWrap( true );
	copyrightLayout->addWidget( copyright_label );
	copyrightLayout->addWidget( copyright_content, 1 );

	auto requiresRealTime = new QLabel(description);
	requiresRealTime->setText( QWidget::tr( "Requires Real Time: " ) +
				( manager->hasRealTimeDependency( _key ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );
	layout->addWidget( requiresRealTime );

	auto realTimeCapable = new QLabel(description);
	realTimeCapable->setText( QWidget::tr( "Real Time Capable: " ) +
					( manager->isRealTimeCapable( _key ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );
	layout->addWidget( realTimeCapable );

	auto inplaceBroken = new QLabel(description);
	inplaceBroken->setText( QWidget::tr( "In Place Broken: " ) +
					( manager->isInplaceBroken( _key ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );
	layout->addWidget( inplaceBroken );

	auto channelsIn = new QLabel(description);
	channelsIn->setText( QWidget::tr( "Channels In: " ) + QString::number(
			manager->getDescription( _key )->inputChannels ) );
	layout->addWidget( channelsIn );

	auto channelsOut = new QLabel(description);
	channelsOut->setText( QWidget::tr( "Channels Out: " ) + QString::number(
			manager->getDescription( _key )->outputChannels ) );
	layout->addWidget( channelsOut );
}




void LadspaDescription::rowChanged( int _pluginIndex )
{
	m_currentSelection = m_pluginKeys[_pluginIndex];
	update( m_currentSelection );
}




void LadspaDescription::onDoubleClicked( QListWidgetItem * _item )
{
	emit( doubleClicked( m_currentSelection ) );
}


} // namespace lmms::gui
