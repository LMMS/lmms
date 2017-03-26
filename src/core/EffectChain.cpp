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

#include "EffectChain.h"
#include "Effect.h"
#include "DummyEffect.h"
#include "MixHelpers.h"
#include "Song.h"


EffectChain::EffectChain( Model * _parent ) :
	Model( _parent ),
	SerializingObject(),
	m_enabledModel( false, NULL, tr( "Effects enabled" ) )
{
}




EffectChain::~EffectChain()
{
	clear();
}




void EffectChain::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "enabled", m_enabledModel.value() );
	_this.setAttribute( "numofeffects", m_effects.count() );

	for( Effect* effect : m_effects)
	{
		if( DummyEffect* dummy = dynamic_cast<DummyEffect*>(effect) )
		{
			_this.appendChild( dummy->originalPluginData() );
		}
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

	// TODO This method should probably also lock the mixer

	m_enabledModel.setValue( _this.attribute( "enabled" ).toInt() );

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

			if( e != NULL && e->isOkay() && e->nodeName() == node.nodeName() )
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
	Engine::mixer()->requestChangeInModel();
	m_effects.append( _effect );
	Engine::mixer()->doneChangeInModel();

	emit dataChanged();
}




void EffectChain::removeEffect( Effect * _effect )
{
	Engine::mixer()->requestChangeInModel();

	Effect ** found = qFind( m_effects.begin(), m_effects.end(), _effect );
	if( found == m_effects.end() )
	{
		Engine::mixer()->doneChangeInModel();
		return;
	}
	m_effects.erase( found );

	Engine::mixer()->doneChangeInModel();
	emit dataChanged();
}




void EffectChain::moveDown( Effect * _effect )
{
	if( _effect != m_effects.last() )
	{
		int i = 0;
		for( EffectList::Iterator it = m_effects.begin();
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}

		Effect * temp = m_effects[i + 1];
		m_effects[i + 1] = _effect;
		m_effects[i] = temp;
	}
}




void EffectChain::moveUp( Effect * _effect )
{
	if( _effect != m_effects.first() )
	{
		int i = 0;
		for( EffectList::Iterator it = m_effects.begin();
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}

		Effect * temp = m_effects[i - 1];
		m_effects[i - 1] = _effect;
		m_effects[i] = temp;
	}
}




bool EffectChain::processAudioBuffer( sampleFrame * _buf, const fpp_t _frames, bool hasInputNoise )
{
	if( m_enabledModel.value() == false )
	{
		return false;
	}
	const bool exporting = Engine::getSong()->isExporting();
	if( exporting ) // strip infs/nans if exporting
	{
		MixHelpers::sanitize( _buf, _frames );
	}

	bool moreEffects = false;
	for( EffectList::Iterator it = m_effects.begin(); it != m_effects.end(); ++it )
	{
		if( hasInputNoise || ( *it )->isRunning() )
		{
			moreEffects |= ( *it )->processAudioBuffer( _buf, _frames );
			if( exporting ) // strip infs/nans if exporting
			{
				MixHelpers::sanitize( _buf, _frames );
			}
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

	for( EffectList::Iterator it = m_effects.begin();
						it != m_effects.end(); it++ )
	{
		( *it )->startRunning();
	}
}




void EffectChain::clear()
{
	emit aboutToClear();

	Engine::mixer()->requestChangeInModel();

	m_enabledModel.setValue( false );
	while( m_effects.count() )
	{
		Effect * e = m_effects[m_effects.count() - 1];
		m_effects.pop_back();
		delete e;
	}

	Engine::mixer()->doneChangeInModel();
}
