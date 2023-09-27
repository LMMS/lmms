/*
 * UpgradeRenameBBTCO.cpp
 *   Functor for upgrading data files after various tags were renamed
 *   to normalize on the "clip" suffix
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

#include "UpgradeRenameBBTCO.h"

#include <vector>

#include "Track.h"

namespace lmms
{


void UpgradeRenameBBTCO::upgrade()
{
	std::vector<std::pair<const char *, const char *>> names {
		{"automationpattern", "automationclip"},
		{"bbtco", "patternclip"},
		{"pattern", "midiclip"},
		{"sampletco", "sampleclip"},
		{"bbtrack", "patterntrack"},
		{"bbtrackcontainer", "patternstore"},
	};
	// Replace names of XML tags
	for (auto name : names)
	{
		QDomNodeList elements = m_document.elementsByTagName(name.first);
		renameElements(elements, name.second);
	}
	// Replace "Beat/Bassline" with "Pattern" in track names
	QDomNodeList elements = m_document.elementsByTagName("track");
	for (int i = 0; i < elements.length(); ++i)
	{
		auto e = elements.item(i).toElement();
		if (e.isNull()) { continue; }
		static_assert(Track::Type::Pattern == static_cast<Track::Type>(1), "Must be type=1 for backwards compatibility");
		if (static_cast<Track::Type>(e.attribute("type").toInt()) == Track::Type::Pattern)
		{
			e.setAttribute("name", e.attribute("name").replace("Beat/Bassline", "Pattern"));
		}
	}
}


} // namespace lmms
