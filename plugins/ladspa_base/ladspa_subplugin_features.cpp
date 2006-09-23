/*
 * ladspa_subplugin_features.cpp - derivation from
 *                                 plugin::descriptor::subPluginFeatures for
 *                                 hosting LADSPA-plugins
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#ifdef QT4

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

#else

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>

#endif


#include "ladspa_subplugin_features.h"
#include "ladspa_2_lmms.h"
#include "mixer.h"
#include "audio_device.h"


ladspaSubPluginDescriptionWidget::ladspaSubPluginDescriptionWidget(
					QWidget * _parent, engine * _engine,
					const ladspa_key_t & _key ) :
	QWidget( _parent
#ifdef QT3
			, "ladspaSubPluginDescriptionWidget"
#endif
						)
{
	ladspa2LMMS * lm = _engine->getLADSPAManager();
	QVBoxLayout * l = new QVBoxLayout( this );

#ifndef QT3
	QGroupBox * groupbox = new QGroupBox( tr( "Description" ), this );
#else
	QGroupBox * groupbox = new QGroupBox( 9, Qt::Vertical, 
						tr( "Description" ), this );
#endif

	QLabel * label = new QLabel( groupbox );
	label->setText( tr( "Name: " ) + lm->getName( _key ) );

	QLabel * maker = new QLabel( groupbox );
	maker->setText( tr( "Maker: " ) + lm->getMaker( _key ) );

	QLabel * copyright = new QLabel( groupbox );
	copyright->setText( tr( "Copyright: " ) + lm->getCopyright( _key ) );

	QLabel * requiresRealTime = new QLabel( groupbox );
	if( lm->hasRealTimeDependency( _key ) )
	{
		requiresRealTime->setText( tr( "Requires Real Time: Yes" ) );
	}
	else
	{
		requiresRealTime->setText( tr( "Requires Real Time: No" ) );
	}

	QLabel * realTimeCapable = new QLabel( groupbox );
	if( lm->isRealTimeCapable( _key ) )
	{
		realTimeCapable->setText( tr( "Real Time Capable: Yes" ) );
	}
	else
	{
		realTimeCapable->setText( tr( "Real Time Capable: No" ) );
	}		

	QLabel * inplaceBroken = new QLabel( groupbox );
	if( lm->isInplaceBroken( _key ) )
	{
		inplaceBroken->setText( tr( "In Place Broken: Yes" ) );
	}
	else
	{
		inplaceBroken->setText( tr( "In Place Broken: No" ) );
	}
	
	QLabel * channelsIn = new QLabel( groupbox );
	channelsIn->setText( tr( "Channels In: " ) +
		QString::number( lm->getDescription( _key )->inputChannels ) );

	QLabel * channelsOut = new QLabel( groupbox );
	channelsOut->setText( tr( "Channels Out: " ) +
		QString::number( lm->getDescription( _key )->outputChannels ) );	
	l->addWidget( groupbox );
}




ladspaSubPluginFeatures::ladspaSubPluginFeatures( plugin::pluginTypes _type ) :
	subPluginFeatures( _type )
{
}




QWidget * ladspaSubPluginFeatures::createDescriptionWidget(
			QWidget * _parent, engine * _eng, const key & _key  )
{
	return( new ladspaSubPluginDescriptionWidget( _parent, _eng,
					subPluginKeyToLadspaKey( _key )  ) );
}




void ladspaSubPluginFeatures::listSubPluginKeys( engine * _eng,
				plugin::descriptor * _desc, keyList & _kl )
{
	ladspa2LMMS * lm = _eng->getLADSPAManager();

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
		case plugin::AnalysisTools:
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
				  _eng->getMixer()->audioDev()->channels() )
		{
			_kl.push_back( ladspaKeyToSubPluginKey(
							_desc,
							( *it ).first,
							( *it ).second ) );
		}
	}
}



#include "ladspa_subplugin_features.moc"
