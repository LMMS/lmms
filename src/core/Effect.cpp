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

#include "Effect.h"
#include "EffectChain.h"
#include "EffectControls.h"
#include "EffectView.h"

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
	m_running( false ),
	m_enabledModel( true, this, tr( "Effect enabled" ) ),
	m_wetDryModel( 1.0f, -1.0f, 1.0f, 0.01f, this, tr( "Wet/Dry mix" ) ),
	m_autoQuitModel( 1.0f, 1.0f, 8000.0f, 100.0f, 1.0f, this, tr( "Decay" ) ),
	m_autoQuitEnabled(ConfigManager::inst()->value("ui", "disableautoquit", "1").toInt() == 0)
{
	m_wetDryModel.setCenterValue(0);

	m_srcState[0] = m_srcState[1] = nullptr;
	reinitSRC();

	// Call the virtual method onEnabledChanged so that effects can react to changes,
	// e.g. by resetting state.
	connect(&m_enabledModel, &BoolModel::dataChanged, [this] { onEnabledChanged(); });
}




Effect::~Effect()
{
	for (const auto& state : m_srcState)
	{
		if (state != nullptr)
		{
			src_delete(state);
		}
	}
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




bool Effect::processAudioBuffer(SampleFrame* buf, const fpp_t frames)
{
	if (!isOkay() || dontRun() || !isEnabled() || !isRunning())
	{
		processBypassedImpl();
		return false;
	}

	const auto status = processImpl(buf, frames);
	switch (status)
	{
		case ProcessStatus::Continue:
			break;
		case ProcessStatus::ContinueIfNotQuiet:
			handleAutoQuit({buf, frames});
			break;
		case ProcessStatus::Sleep:
			return false;
		default:
			break;
	}

	return isRunning();
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




void Effect::handleAutoQuit(std::span<const SampleFrame> output)
{
	if (!m_autoQuitEnabled)
	{
		return;
	}

	/*
	 * In the past, the RMS was calculated then compared with a threshold of 10^(-10).
	 * Now we use a different algorithm to determine whether a buffer is non-quiet, so
	 * a new threshold is needed for the best compatibility. The following is how it's derived.
	 *
	 * Old method:
	 * RMS = average (L^2 + R^2) across stereo buffer.
	 * RMS threshold = 10^(-10)
	 *
	 * So for a single channel, it would be:
	 * RMS/2 = average M^2 across single channel buffer.
	 * RMS/2 threshold = 5^(-11)
	 *
	 * The new algorithm for determining whether a buffer is non-silent compares M with the threshold,
	 * not M^2, so the square root of M^2's threshold should give us the most compatible threshold for
	 * the new algorithm:
	 *
	 * (RMS/2)^0.5 = (5^(-11))^0.5 = 0.0001431 (approx.)
	 *
	 * In practice though, the exact value shouldn't really matter so long as it's sufficiently small.
	 */
	static constexpr auto threshold = 0.0001431f;

	// Check whether we need to continue processing input. Restart the
	// counter if the threshold has been exceeded.

	for (const SampleFrame& frame : output)
	{
		const auto abs = frame.abs();
		if (abs.left() >= threshold || abs.right() >= threshold)
		{
			// The output buffer is not quiet
			m_quietBufferCount = 0;
			return;
		}
	}

	// The output buffer is quiet, so check if auto-quit should be activated yet
	if (++m_quietBufferCount > timeout())
	{
		// Activate auto-quit
		stopRunning();
	}
}




gui::PluginView * Effect::instantiateView( QWidget * _parent )
{
	return new gui::EffectView( this, _parent );
}

	


void Effect::reinitSRC()
{
	for (auto& state : m_srcState)
	{
		if (state != nullptr)
		{
			src_delete(state);
		}
		int error;
		const int currentInterpolation = Engine::audioEngine()->currentQualitySettings().libsrcInterpolation();
		if((state = src_new(currentInterpolation, DEFAULT_CHANNELS, &error)) == nullptr)
		{
			qFatal( "Error: src_new() failed in effect.cpp!\n" );
		}
	}
}




void Effect::resample( int _i, const SampleFrame* _src_buf,
							sample_rate_t _src_sr,
				SampleFrame* _dst_buf, sample_rate_t _dst_sr,
								f_cnt_t _frames )
{
	if( m_srcState[_i] == nullptr )
	{
		return;
	}
	m_srcData[_i].input_frames = _frames;
	m_srcData[_i].output_frames = Engine::audioEngine()->framesPerPeriod();
	m_srcData[_i].data_in = const_cast<float*>(_src_buf[0].data());
	m_srcData[_i].data_out = _dst_buf[0].data ();
	m_srcData[_i].src_ratio = (double) _dst_sr / _src_sr;
	m_srcData[_i].end_of_input = 0;

	if (int error = src_process(m_srcState[_i], &m_srcData[_i]))
	{
		qFatal( "Effect::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
}

} // namespace lmms
