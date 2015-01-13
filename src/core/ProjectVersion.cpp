/*
 * ProjectVersion.cpp - compare versions in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2015 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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

int ProjectVersion::compare(ProjectVersion v1, ProjectVersion v2)
{
	if(v1.getMajor() != v2.getMajor())
	{
		return v1.getMajor() - v2.getMajor();
	}
	
	// return prematurely for Major comparison
	if(v1.getCompareType() == CompareType::Major || 
		v2.getCompareType() == CompareType::Major)
	{
		return 0;
	}

	if(v1.getMinor() != v2.getMinor())
	{
		return v1.getMinor() - v2.getMinor();
	}

	// return prematurely for Minor comparison
	if(v1.getCompareType() == CompareType::Minor || 
		v2.getCompareType() == CompareType::Minor)
	{
		return 0;
	}

	if(v1.getRelease() != v2.getRelease())
	{
		return v1.getRelease() - v2.getRelease();
	}

	if(v1.getCompareType() == CompareType::Release || 
		v2.getCompareType() == CompareType::Release) {
		return 0;
	}

	// make sure 0.x.y > 0.x.y-patch
	if(v1.getBuild().isEmpty())
	{
		return 1;
	}
	if(v2.getBuild().isEmpty())
	{
		return -1;
	}

	return QString::compare(v1.getBuild(), v2.getBuild());
}

