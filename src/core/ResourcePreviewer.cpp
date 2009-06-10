/*
 * ResourcePreviewer.cpp - implementation of ResourcePreviewer
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "ResourcePreviewer.h"
#include "ResourceItem.h"
#include "engine.h"
#include "instrument_track.h"
#include "mmp.h"
#include "project_journal.h"


ResourcePreviewer::ResourcePreviewer() :
	m_previewTrackContainer(),
	m_previewTrack( NULL )
{
	// do not clutter global journal with items due to changing settings
	// in preview classes
	m_previewTrackContainer.setJournalling( false );
	m_previewTrack = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack,
						&m_previewTrackContainer ) );
	m_previewTrack->setJournalling( false );

}




void ResourcePreviewer::preview( ResourceItem * _item )
{
	const bool j = engine::getProjectJournal()->isJournalling();
	engine::getProjectJournal()->setJournalling( false );
	engine::setSuppressMessages( true );

	switch( _item->type() )
	{
		case ResourceItem::TypePreset:
			// fetch data, load into multimedia project and
			// load it as preset
			m_previewTrack->loadTrackSpecificSettings(
				multimediaProject( _item->fetchData() ).
					content().firstChild().toElement() );
			m_previewTrack->getMidiPort()->setMode(
							midiPort::Disabled );
			break;
	}

	engine::setSuppressMessages( false );
	engine::getProjectJournal()->setJournalling( j );
}


#include "moc_ResourcePreviewer.cxx"

