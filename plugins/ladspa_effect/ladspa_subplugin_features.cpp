/*
 * ladspa_subplugin_features.cpp - derivation from
 *                                 plugin::descriptor::subPluginFeatures for
 *                                 hosting LADSPA-plugins
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>


#include "ladspa_subplugin_features.h"
#include "ladspa_2_lmms.h"
#include "mixer.h"
#include "audio_device.h"
#include "engine.h"


ladspaSubPluginFeatures::ladspaSubPluginFeatures( plugin::pluginTypes _type ) :
	subPluginFeatures( _type )
{
}




void ladspaSubPluginFeatures::fillDescriptionWidget( QWidget * _parent,
							const key * _key  )
{
	const ladspa_key_t & lkey = subPluginKeyToLadspaKey( _key );
	ladspa2LMMS * lm = engine::getLADSPAManager();

	QLabel * label = new QLabel( _parent );
	label->setText( QWidget::tr( "Name: " ) + lm->getName( lkey ) );

	QWidget * maker = new QWidget( _parent );
	QHBoxLayout * l = new QHBoxLayout( maker );
	l->setMargin( 0 );
	l->setSpacing( 0 );

	QLabel * maker_label = new QLabel( maker );
	maker_label->setText( QWidget::tr( "Maker: " ) );
	maker_label->setAlignment( Qt::AlignTop );
	QLabel * maker_content = new QLabel( maker );
	maker_content->setText( lm->getMaker( lkey ) );
	maker_content->setWordWrap( TRUE );
	l->addWidget( maker_label );
	l->addWidget( maker_content );
	l->setStretchFactor( maker_content, 100 );

	QWidget * copyright = new QWidget( _parent );
	l = new QHBoxLayout( copyright );
	l->setMargin( 0 );
	l->setSpacing( 0 );

	copyright->setMinimumWidth( _parent->minimumWidth() );
	QLabel * copyright_label = new QLabel( copyright );
	copyright_label->setText( QWidget::tr( "Copyright: " ) );
	copyright_label->setAlignment( Qt::AlignTop );

	QLabel * copyright_content = new QLabel( copyright );
	copyright_content->setText( lm->getCopyright( lkey ) );
	copyright_content->setWordWrap( TRUE );
	l->addWidget( copyright_label );
	l->addWidget( copyright_content );
	l->setStretchFactor( copyright_content, 100 );

	QLabel * requiresRealTime = new QLabel( _parent );
	requiresRealTime->setText( QWidget::tr( "Requires Real Time: " ) +
					( lm->hasRealTimeDependency( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );

	QLabel * realTimeCapable = new QLabel( _parent );
	realTimeCapable->setText( QWidget::tr( "Real Time Capable: " ) +
					( lm->isRealTimeCapable( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );

	QLabel * inplaceBroken = new QLabel( _parent );
	inplaceBroken->setText( QWidget::tr( "In Place Broken: " ) +
					( lm->isInplaceBroken( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );
	
	QLabel * channelsIn = new QLabel( _parent );
	channelsIn->setText( QWidget::tr( "Channels In: " ) +
		QString::number( lm->getDescription( lkey )->inputChannels ) );

	QLabel * channelsOut = new QLabel( _parent );
	channelsOut->setText( QWidget::tr( "Channels Out: " ) +
		QString::number( lm->getDescription( lkey )->outputChannels ) );	
}




void ladspaSubPluginFeatures::listSubPluginKeys( plugin::descriptor * _desc,
								keyList & _kl )
{
	ladspa2LMMS * lm = engine::getLADSPAManager();

	l_sortable_plugin_t plugins;
	switch( m_type )
	{
		case plugin::Instrument:
			plugins = lm->getInstruments();
			break;
		case plugin::Effect:
			plugins = lm->getValidEffects();
			//plugins += lm->getInvalidEffects();
			break;
		case plugin::Tool:
			plugins = lm->getAnalysisTools();
			break;
		case plugin::Other:
			plugins = lm->getOthers();
			break;
		default:
			break;
	}

	for( l_sortable_plugin_t::const_iterator it = plugins.begin();
						it != plugins.end(); ++it )
	{
		if( lm->getDescription( ( *it ).second )->inputChannels <= 
				  engine::getMixer()->audioDev()->channels() )
		{
			_kl.push_back( ladspaKeyToSubPluginKey(
							_desc,
							( *it ).first,
							( *it ).second ) );
		}
	}
}


