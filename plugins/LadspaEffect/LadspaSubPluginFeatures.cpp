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
	const auto ldesc = lm->getDescription(lkey);

	auto labelText = QString{
		"<p><b>%1</b>%2</p>" // Name
		"<p><b>%3</b><code>%4</code></p>" // File
		"<p><b>%5</b>%6</p>" // Author
		"<p><b>%7</b>%8</p>" // Copyright
		"<p><b>%9</b>%10</p>" // Channels
	}.arg(
		QWidget::tr("Name: "), lm->getName(lkey),
		QWidget::tr("File: "), lkey.first,
		QWidget::tr("Author: "), lm->getMaker(lkey).replace(" at ", "@").replace(" dot ", ".").toHtmlEscaped(),
		QWidget::tr("Copyright: "), lm->getCopyright(lkey),
		QWidget::tr("Channels: "), QWidget::tr("%1 in, %2 out").arg(ldesc->inputChannels).arg(ldesc->outputChannels)
	);

	if (lm->hasRealTimeDependency(lkey))
	{
		labelText += QString{"<p><b>%1</b>%2</p>"}.arg(
			QWidget::tr("Real-time Dependency: "),
			QWidget::tr("This plugin has a real-time dependency (e.g. listens to a MIDI device) so its output must "
				"not be cached or subject to significant latency.")
		);
	}

	if (!lm->isRealTimeCapable(lkey))
	{
		labelText += QString{"<p><b>%1</b>%2</p>"}.arg(
			QWidget::tr("Not Real-time Capable: "),
			QWidget::tr("This plugin is not suitable for use in a &lsquo;hard real-time&rsquo; environment.")
		);
	}

	auto label = new QLabel(labelText, _parent);
	label->setWordWrap(true);
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
