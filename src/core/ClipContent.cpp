/*
 * ClipContent.h - Clip content base class & content linking infrastructure
 *
 * Copyright (c) 2026 Itreza <louis.bossy/at/etu.univ-tours.fr>
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
#include <qchar.h>
#include <qobject.h>
#include <stdexcept>
#include "ClipContent.h"


namespace lmms
{

QString SharedContentRepository::submit(
	std::weak_ptr<ClipContent> content, QString linkName
)
{
	// Pick an available name/key if none was provided
	int i = 65;
	while (linkName.isEmpty())
	{
		linkName = i <= 90 ? QString(QChar(i)) : QString::number(i - 65);
		if (auto it = s_contents.find(linkName); it != s_contents.end())
		{
			if (it->second.expired()) {s_contents.erase(it);}
			else {linkName.clear();}
		}
		i++;
	}

	if (s_contents.contains(linkName)) {return "";}

	s_contents[linkName] = std::move(content);
	return linkName;
}


std::shared_ptr<ClipContent> SharedContentRepository::get(QString linkName)
{
	try
	{
		return s_contents.at(linkName).lock();
	}
	catch (std::out_of_range)
	{
		return nullptr;
	}
}

} // namespace lmms
