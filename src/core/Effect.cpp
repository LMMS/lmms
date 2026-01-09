/*
 * Effect.cpp - base-class for effects
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

#include <QDomElement>

#include "AudioBus.h"
#include "Effect.h"
#include "EffectChain.h"
#include "EffectControls.h"
#include "EffectView.h"
#include "MixHelpers.h"

#include "ConfigManager.h"
#include "SampleFrame.h"

namespace lmms
{


Effect::Effect( const Plugin::Descriptor * _desc,
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Plugin( _desc, _parent, _key ),
	m_parent( nullptr ),
	m_okay( true ),
	m_noRun( false ),
	m_awake(false),
	m_enabledModel( true, this, tr( "Effect enabled" ) ),
	m_wetDryModel( 1.0f, -1.0f, 1.0f, 0.01f, this, tr( "Wet/Dry mix" ) ),
	m_autoQuitModel( 1.0f, 1.0f, 8000.0f, 100.0f, 1.0f, this, tr( "Decay" ) ),
	m_autoQuitEnabled(ConfigManager::inst()->value("ui", "disableautoquit", "1").toInt() == 0)
{
	m_wetDryModel.setCenterValue(0);

	// Call the virtual method onEnabledChanged so that effects can react to changes,
	// e.g. by resetting state.
	connect(&m_enabledModel, &BoolModel::dataChanged, [this] { onEnabledChanged(); });
}

void Effect::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_enabledModel.saveSettings( _doc, _this, "on" );
	m_wetDryModel.saveSettings( _doc, _this, "wet" );
	m_autoQuitModel.saveSettings( _doc, _this, "autoquit" );
	controls()->saveState( _doc, _this );
}




void Effect::loadSettings( const QDomElement & _this )
{
	m_enabledModel.loadSettings( _this, "on" );
	m_wetDryModel.loadSettings( _this, "wet" );
	m_autoQuitModel.loadSettings( _this, "autoquit" );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( controls()->nodeName() == node.nodeName() )
			{
				controls()->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}




bool Effect::processAudioBuffer(AudioBus& inOut)
{
	if (!isAwake())
	{
		if (!inOut.hasInputNoise(0b11))
		{
			// Sleeping plugins need to zero any track channels their output is routed to in order to
			// prevent sudden track channel passthrough behavior when the plugin is put to sleep.
			// Otherwise auto-quit could become audibly noticeable, which is not intended.

			inOut.silenceChannels(0b11);

			return false;
		}

		startRunning();
	}

	if (!isRunning())
	{
		// Plugin is awake but not running
		processBypassedImpl();
		return false;
	}

	const auto status = processImpl(inOut.interleavedBuffer(0).asSampleFrames().data(), inOut.frames());

	// Copy interleaved plugin output to planar
	MixHelpers::copy(inOut.buffers(0), inOut.interleavedBuffer(0));

	inOut.sanitize(0b11);

	// Update silence status for track channels the processor wrote to
	const bool silentOutput = inOut.update(0b11);

	switch (status)
	{
		case ProcessStatus::Continue:
			break;
		case ProcessStatus::ContinueIfNotQuiet:
			handleAutoQuit(silentOutput);
			break;
		case ProcessStatus::Sleep:
			stopRunning();
			return false;
		default:
			break;
	}

	return isAwake();
}




Effect * Effect::instantiate( const QString& pluginName,
				Model * _parent,
				Descriptor::SubPluginFeatures::Key * _key )
{
	Plugin * p = Plugin::instantiateWithKey( pluginName, _parent, _key );
	// check whether instantiated plugin is an effect
	if( dynamic_cast<Effect *>( p ) != nullptr )
	{
		// everything ok, so return pointer
		auto effect = dynamic_cast<Effect*>(p);
		effect->m_parent = dynamic_cast<EffectChain *>(_parent);
		return effect;
	}

	// not quite... so delete plugin and leave it up to the caller to instantiate a DummyEffect
	delete p;

	return nullptr;
}




void Effect::handleAutoQuit(bool silentOutput)
{
	if (!m_autoQuitEnabled)
	{
		return;
	}

	// Check whether we need to continue processing input. Restart the
	// counter if the threshold has been exceeded.

	if (silentOutput)
	{
		// The output buffer is quiet, so check if auto-quit should be activated yet
		if (++m_quietBufferCount > timeout())
		{
			// Activate auto-quit
			stopRunning();
		}
	}
	else
	{
		// The output buffer is not quiet
		m_quietBufferCount = 0;
	}
}




gui::PluginView * Effect::instantiateView( QWidget * _parent )
{
	return new gui::EffectView( this, _parent );
}

} // namespace lmms
