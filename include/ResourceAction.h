/*
 * ResourceAction.h - header file for ResourceAction
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

#ifndef _RESOURCE_ACTION_H
#define _RESOURCE_ACTION_H

class InstrumentTrack;
class trackContainer;
class ResourceItem;


class ResourceAction
{
public:
	enum Actions
	{
		EditProperties,
		LoadProject,
		LoadInNewTrackSongEditor,
		LoadInNewTrackBBEditor,
		LoadInActiveInstrumentTrack,
		DownloadIntoCollection,
		UploadToWWW,
		DeleteLocalResource,
		ImportFile,
		NumActions
	} ;
	typedef Actions Action;

	ResourceAction( const ResourceItem * _item,
				Action _action = NumActions ) :
		m_action( _action ),
		m_item( _item )
	{
	}

	bool loadProject();
	bool loadByPlugin( InstrumentTrack * _target );
	bool loadPreset( InstrumentTrack * _target );
	bool importProject( trackContainer * _target );


private:
	Action m_action;
	const ResourceItem * m_item;

} ;


#endif
