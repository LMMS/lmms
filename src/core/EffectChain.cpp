/*
 * EffectChain.cpp - class for processing and effects chain
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <cassert>

#include "EffectChain.h"
#include "Effect.h"
#include "DummyEffect.h"
#include "MixHelpers.h"

namespace lmms
{


EffectChain::EffectChain( Model * _parent ) :
	Model( _parent ),
	SerializingObject(),
	m_enabledModel( false, nullptr, tr( "Effects enabled" ) )
{
}




EffectChain::~EffectChain()
{
	clear();
}




void EffectChain::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_enabledModel.saveSettings( _doc, _this, "enabled" );
	_this.setAttribute("numofeffects", static_cast<int>(m_effects.size()));

	for( Effect* effect : m_effects)
	{
		if (auto dummy = dynamic_cast<DummyEffect*>(effect)) { _this.appendChild(dummy->originalPluginData()); }
		else
		{
			QDomElement ef = effect->saveState( _doc, _this );
			ef.setAttribute( "name", QString::fromUtf8( effect->descriptor()->name ) );
			ef.appendChild( effect->key().saveXML( _doc ) );
		}
	}
}




void EffectChain::loadSettings( const QDomElement & _this )
{
	clear();

	// TODO This method should probably also lock the audio engine

	m_enabledModel.loadSettings( _this, "enabled" );

	const int plugin_cnt = _this.attribute( "numofeffects" ).toInt();

	QDomNode node = _this.firstChild();
	int fx_loaded = 0;
	while( !node.isNull() && fx_loaded < plugin_cnt )
	{
		if( node.isElement() && node.nodeName() == "effect" )
		{
			QDomElement effectData = node.toElement();

			const QString name = effectData.attribute( "name" );
			EffectKey key( effectData.elementsByTagName( "key" ).item( 0 ).toElement() );

			Effect* e = Effect::instantiate( name.toUtf8(), this, &key );

			if( e != nullptr && e->isOkay() && e->nodeName() == node.nodeName() )
			{
				e->restoreState( effectData );
			}
			else
			{
				delete e;
				e = new DummyEffect( parentModel(), effectData );
			}

			m_effects.push_back( e );
			++fx_loaded;
		}
		node = node.nextSibling();
	}

	emit dataChanged();
}




void EffectChain::appendEffect( Effect * _effect )
{
	Engine::audioEngine()->requestChangeInModel();
	m_effects.push_back(_effect);
	Engine::audioEngine()->doneChangeInModel();

	m_enabledModel.setValue( true );

	emit dataChanged();
}




void EffectChain::removeEffect( Effect * _effect )
{
	Engine::audioEngine()->requestChangeInModel();

	auto found = std::find(m_effects.begin(), m_effects.end(), _effect);
	if( found == m_effects.end() )
	{
		Engine::audioEngine()->doneChangeInModel();
		return;
	}
	m_effects.erase( found );

	Engine::audioEngine()->doneChangeInModel();

	if (m_effects.empty())
	{
		m_enabledModel.setValue( false );
	}

	emit dataChanged();
}




void EffectChain::moveDown( Effect * _effect )
{
	if (_effect != m_effects.back())
	{
		auto it = std::find(m_effects.begin(), m_effects.end(), _effect);
		assert(it != m_effects.end());
		std::swap(*std::next(it), *it);
	}
}




void EffectChain::moveUp( Effect * _effect )
{
	if (_effect != m_effects.front())
	{
		auto it = std::find(m_effects.begin(), m_effects.end(), _effect);
		assert(it != m_effects.end());
		std::swap(*std::prev(it), *it);
	}
}




bool EffectChain::processAudioBuffer( SampleFrame* _buf, const fpp_t _frames, bool hasInputNoise )
{
	if( m_enabledModel.value() == false )
	{
		return false;
	}

	MixHelpers::sanitize( _buf, _frames );

	bool moreEffects = false;
	for (const auto& effect : m_effects)
	{
		if (hasInputNoise || effect->isRunning())
		{
			moreEffects |= effect->processAudioBuffer(_buf, _frames);
			MixHelpers::sanitize(_buf, _frames);
		}
	}

	return moreEffects;
}




void EffectChain::startRunning()
{
	if( m_enabledModel.value() == false )
	{
		return;
	}

	for (const auto& effect : m_effects)
	{
		effect->startRunning();
	}
}




void EffectChain::clear()
{
	emit aboutToClear();

	Engine::audioEngine()->requestChangeInModel();

	while (m_effects.size())
	{
		auto e = m_effects[m_effects.size() - 1];
		m_effects.pop_back();
		delete e;
	}

	Engine::audioEngine()->doneChangeInModel();

	m_enabledModel.setValue( false );
}


} // namespace lmms
