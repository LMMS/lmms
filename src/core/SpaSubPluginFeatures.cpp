/*
 * SpaSubPluginFeatures.cpp - derivation from
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

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLibrary>
#include <spa/spa.h>

#include "SpaManager.h"
#include "SpaSubPluginFeatures.h"
#include "AudioDevice.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "Mixer.h"
#include "embed.h"
#include "PluginFactory.h"


spa::descriptor *SpaSubPluginFeatures::spaDescriptor(const Plugin::Descriptor::SubPluginFeatures::Key &k)
{
	return Engine::getSPAManager()->getDescriptor(k.attributes["plugin"]);
}

SpaSubPluginFeatures::SpaSubPluginFeatures( Plugin::PluginTypes _type ) :
	SubPluginFeatures( _type )
{
}




void SpaSubPluginFeatures::fillDescriptionWidget( QWidget * _parent,
						const Key * _key ) const
{
	QLabel * label = new QLabel( _parent );
	label->setText( QWidget::tr( "Name: " ) + "SPA (TODO)" );
	(void)_key;
#if 0
	const ladspa_key_t & lkey = subPluginKeyToLadspaKey( _key );
	Ladspa2LMMS * lm = Engine::getLADSPAManager();

	QLabel * label = new QLabel( _parent );
	label->setText( QWidget::tr( "Name: " ) + lm->getName( lkey ) );

	QLabel* fileInfo = new QLabel( _parent );
	fileInfo->setText( QWidget::tr( "File: %1" ).arg( lkey.first ) );

	QWidget * maker = new QWidget( _parent );
	QHBoxLayout * l = new QHBoxLayout( maker );
	l->setMargin( 0 );
	l->setSpacing( 0 );

	QLabel * maker_label = new QLabel( maker );
	maker_label->setText( QWidget::tr( "Maker: " ) );
	maker_label->setAlignment( Qt::AlignTop );
	QLabel * maker_content = new QLabel( maker );
	maker_content->setText( lm->getMaker( lkey ) );
	maker_content->setWordWrap( true );
	l->addWidget( maker_label );
	l->addWidget( maker_content, 1 );

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
	copyright_content->setWordWrap( true );
	l->addWidget( copyright_label );
	l->addWidget( copyright_content, 1 );

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
#endif
}

const char *SpaSubPluginFeatures::additionalFileExtensions(const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	return spaDescriptor(k)->save_formats();
}

const char *SpaSubPluginFeatures::displayName(const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	return spaDescriptor(k)->name();
}

const char *SpaSubPluginFeatures::description(const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	return spaDescriptor(k)->description_line();
}

const PixmapLoader *SpaSubPluginFeatures::logo(const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	spa::descriptor* spaDes = spaDescriptor(k);

	const char** xpm = spaDes->xpm_load();
	QString uniqueName = spa::unique_name(*spaDes).c_str();

	QString xpmKey = "spa-plugin:" + uniqueName;

	return xpm
		? new PixmapLoader(QString("xpm:" + xpmKey), xpm)
		: new PixmapLoader("plugins");
}

void SpaSubPluginFeatures::listSubPluginKeys(
						const Plugin::Descriptor * _desc, KeyList & _kl ) const
{
	SpaManager* spaMgr = Engine::getSPAManager();
	qDebug() << "listing sub plugin keys...";
	for(const std::pair<const std::string, SpaManager::SpaInfo>& pr : *spaMgr)
	{
		if(pr.second.m_type == m_type)
		{
			using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
			KeyType::AttributeMap atm;
			atm["file"] = pr.second.m_path; // TODO: remove path, remove so/dll
			atm["plugin"] = QString::fromUtf8(pr.first.c_str());
			const spa::descriptor& spaDes = *pr.second.m_descriptor;
			QString uniqueName = spa::unique_name(spaDes).c_str();

			_kl.push_back(KeyType(_desc, spaDes.name(), atm));
			qDebug() << "Found SPA sub plugin key of type" << m_type << ":" << _kl.back().name;
		}
	}
}




/*ladspa_key_t SpaSubPluginFeatures::subPluginKeyToLadspaKey(
							const Key * _key )
{
	QString file = _key->attributes["file"];
	return( ladspa_key_t( file.remove( QRegExp( "\\.so$" ) ).
				remove( QRegExp( "\\.dll$" ) ) +
#ifdef LMMS_BUILD_WIN32
						".dll"
#else
						".so"
#endif
					, _key->attributes["plugin"] ) );
}*/

