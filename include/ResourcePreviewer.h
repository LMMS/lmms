/*
 * ResourcePreviewer.h - header file for ResourcePreviewer
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


#ifndef _RESOURCE_PREVIEWER_H
#define _RESOURCE_PREVIEWER_H

#include "track_container.h"
#include "mmp.h"

class Piano;
class ResourceItem;
class InstrumentTrack;


class ResourcePreviewer
{
public:
	ResourcePreviewer();
	~ResourcePreviewer();

	void preview( ResourceItem * _item );
	void stopPreview();

	Piano * pianoModel();


private:
	class PreviewTrackContainer : public trackContainer
	{
		virtual QString nodeName( void ) const
		{
			return "PreviewTrackContainer";
		}
	} ;

	PreviewTrackContainer m_previewTrackContainer;
	InstrumentTrack * m_previewTrack;

	multimediaProject m_defaultSettings;

} ;


#endif
