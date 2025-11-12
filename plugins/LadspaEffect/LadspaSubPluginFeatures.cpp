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
#include "AudioDevice.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "Ladspa2LMMS.h"
#include "LadspaBase.h"

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

	QString labelText =
		"<p><b>" + QWidget::tr("Name") + ":</b> " + lm->getName(lkey) + "</p>"
		"<p><b>" + QWidget::tr("File") + ":</b> <code>" + lkey.first + "</code></p>"
		"<p><b>" + QWidget::tr("Author") + ":</b> "
			+ lm->getMaker(lkey)
				.replace(" at ", "@")
				.replace(" dot ", ".")
				.toHtmlEscaped()
			+ "</p>"
		"<p><b>" + QWidget::tr("Copyright") + ":</b> " + lm->getCopyright(lkey) + "</p>"
		"<p><b>" + QWidget::tr("Channels") + ":</b> "
			+ QString::number(ldesc->inputChannels) + " " + QWidget::tr("in") + ", "
			+ QString::number(ldesc->outputChannels) + " " + QWidget::tr("out")
			+ "</p>";

	if (lm->hasRealTimeDependency(lkey))
	{
		labelText += "<p><b>" + QWidget::tr("Real-time Dependency")
			+ ":</b> " + QWidget::tr(
				"This plugin has a real-time dependency (e.g. listens "
				"to a MIDI device) so its output must not be cached or "
				"subject to significant latency."
			) + "</p>";
	}

	if (lm->isRealTimeCapable(lkey))
	{
		labelText += "<p><b>" + QWidget::tr("Real-time Capable")
			+ ":</b> " + QWidget::tr(
				"This plugin is capable of running in a &lsquo;hard "
				"real-time&rsquo; environment."
			) + "</p>";
	}

	if (lm->isInplaceBroken(lkey))
	{
		labelText += "<p><b>" + QWidget::tr("In-place Broken")
			+ ":</b> " + QWidget::tr(
				"This plugin cannot process audio &lsquo;in "
				"place&rsquo; and may cease to work correctly if the "
				"host elects to use the same data location for both "
				"input and output."
			) + "</p>";
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
