/*
 * ProjectVersion.cpp - compare versions in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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




#include "ProjectVersion.h"


int ProjectVersion::compare(const ProjectVersion & v1, const ProjectVersion & v2)
{
	if(v1.majorVersion() != v2.majorVersion())
	{
		return v1.majorVersion() - v2.majorVersion();
	}

	if(v1.minorVersion() != v2.minorVersion())
	{
		return v1.minorVersion() - v2.minorVersion();
	}

	if(v1.releaseVersion() != v2.releaseVersion())
	{
		return v1.releaseVersion() - v2.releaseVersion();
	}

	// make sure 0.x.y > 0.x.y-patch
	if(v1.buildVersion().isEmpty())
	{
		return 1;
	}
	if(v2.buildVersion().isEmpty())
	{
		return -1;
	}

	return QString::compare(v1.buildVersion(), v2.buildVersion());
}


