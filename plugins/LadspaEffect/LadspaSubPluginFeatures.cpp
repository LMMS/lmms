/*
 * LadspaSubPluginFeatures.cpp - derivation from
 *                               Plugin::Descriptor::SubPluginFeatures for
 *                               hosting LADSPA-plugins
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QHBoxLayout>
#include <QLabel>

#include "LadspaSubPluginFeatures.h"

#include "Engine.h"
#include "Ladspa2LMMS.h"
#include "LadspaBase.h"
#include "lmms_constants.h"

namespace lmms
{


LadspaSubPluginFeatures::LadspaSubPluginFeatures( Plugin::Type _type ) :
	SubPluginFeatures( _type )
{
}




QString LadspaSubPluginFeatures::displayName(const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	const ladspa_key_t & lkey = subPluginKeyToLadspaKey(&k);
	Ladspa2LMMS * lm = Engine::getLADSPAManager();
	return lm->getName(lkey);
}




void LadspaSubPluginFeatures::fillDescriptionWidget( QWidget * _parent,
													const Key * _key  ) const
{
	const ladspa_key_t & lkey = subPluginKeyToLadspaKey( _key );
	Ladspa2LMMS * lm = Engine::getLADSPAManager();

	auto label = new QLabel(_parent);
	label->setText( QWidget::tr( "Name: " ) + lm->getName( lkey ) );

	auto fileInfo = new QLabel(_parent);
	fileInfo->setText( QWidget::tr( "File: %1" ).arg( lkey.first ) );

	auto maker = new QWidget(_parent);
	auto l = new QHBoxLayout(maker);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing( 0 );

	auto maker_label = new QLabel(maker);
	maker_label->setText( QWidget::tr( "Maker: " ) );
	maker_label->setAlignment( Qt::AlignTop );
	auto maker_content = new QLabel(maker);
	maker_content->setText( lm->getMaker( lkey ) );
	maker_content->setWordWrap( true );
	l->addWidget( maker_label );
	l->addWidget( maker_content, 1 );

	auto copyright = new QWidget(_parent);
	l = new QHBoxLayout( copyright );
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing( 0 );

	copyright->setMinimumWidth( _parent->minimumWidth() );
	auto copyright_label = new QLabel(copyright);
	copyright_label->setText( QWidget::tr( "Copyright: " ) );
	copyright_label->setAlignment( Qt::AlignTop );

	auto copyright_content = new QLabel(copyright);
	copyright_content->setText( lm->getCopyright( lkey ) );
	copyright_content->setWordWrap( true );
	l->addWidget( copyright_label );
	l->addWidget( copyright_content, 1 );

	auto requiresRealTime = new QLabel(_parent);
	requiresRealTime->setText( QWidget::tr( "Requires Real Time: " ) +
					( lm->hasRealTimeDependency( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );

	auto realTimeCapable = new QLabel(_parent);
	realTimeCapable->setText( QWidget::tr( "Real Time Capable: " ) +
					( lm->isRealTimeCapable( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );

	auto inplaceBroken = new QLabel(_parent);
	inplaceBroken->setText( QWidget::tr( "In Place Broken: " ) +
					( lm->isInplaceBroken( lkey ) ?
							QWidget::tr( "Yes" ) :
							QWidget::tr( "No" ) ) );

	auto channelsIn = new QLabel(_parent);
	channelsIn->setText( QWidget::tr( "Channels In: " ) +
		QString::number( lm->getDescription( lkey )->inputChannels ) );

	auto channelsOut = new QLabel(_parent);
	channelsOut->setText( QWidget::tr( "Channels Out: " ) +
		QString::number( lm->getDescription( lkey )->outputChannels ) );	
}




void LadspaSubPluginFeatures::listSubPluginKeys(
						const Plugin::Descriptor * _desc, KeyList & _kl ) const
{
	Ladspa2LMMS * lm = Engine::getLADSPAManager();

	l_sortable_plugin_t plugins;
	switch( m_type )
	{
		case Plugin::Type::Instrument:
			plugins = lm->getInstruments();
			break;
		case Plugin::Type::Effect:
			plugins = lm->getValidEffects();
			//plugins += lm->getInvalidEffects();
			break;
		case Plugin::Type::Tool:
			plugins = lm->getAnalysisTools();
			break;
		case Plugin::Type::Other:
			plugins = lm->getOthers();
			break;
		default:
			break;
	}

	for( l_sortable_plugin_t::const_iterator it = plugins.begin();
						it != plugins.end(); ++it )
	{
		if (lm->getDescription((*it).second)->inputChannels <= DEFAULT_CHANNELS)
		{
			_kl.push_back( ladspaKeyToSubPluginKey( _desc, ( *it ).first, ( *it ).second ) );
		}
	}
}




ladspa_key_t LadspaSubPluginFeatures::subPluginKeyToLadspaKey(
							const Key * _key )
{
	QString file = _key->attributes["file"];
	return(ladspa_key_t(file.remove(QRegularExpression("\\.so$")).remove(QRegularExpression("\\.dll$")) +
#ifdef LMMS_BUILD_WIN32
						".dll"
#else
						".so"
#endif
					, _key->attributes["plugin"] ) );
}


} // namespace lmms
