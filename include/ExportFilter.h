/*
 * ExportFilter.h - declaration of class ExportFilter, the base-class for all
 *                  file export filters
 *
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

#ifndef EXPORT_FILTER_H
#define EXPORT_FILTER_H

#include <QtCore/QFile>

#include "TrackContainer.h"
#include "Plugin.h"


class LMMS_EXPORT ExportFilter : public Plugin
{
public:
	ExportFilter( const Descriptor * _descriptor ) : Plugin( _descriptor, NULL ) {}
	virtual ~ExportFilter() {}


	virtual bool tryExport(const TrackContainer::TrackList &tracks,
				const TrackContainer::TrackList &tracksBB,
				int tempo, int masterPitch, const QString &filename ) = 0;
protected:

	void saveSettings( QDomDocument &, QDomElement & ) override
	{
	}

	void loadSettings( const QDomElement & ) override
	{
	}

	QString nodeName() const override
	{
		return "import_filter";
	}


private:

} ;


#endif
