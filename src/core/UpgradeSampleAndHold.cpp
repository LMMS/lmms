/*
 * UpgradeSampleAndHold.cpp
 *   Functor for upgrading data files for new sample
 *   and hold functionality, so freq > 0 won't change the project.
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

#include "UpgradeSampleAndHold.h"


namespace lmms
{


void UpgradeSampleAndHold::upgrade()
{
	QDomNodeList elements = m_document.elementsByTagName("lfocontroller");
	for (int i = 0; i < elements.length(); ++i)
	{
		if (elements.item(i).isNull()) { continue; }
		auto e = elements.item(i).toElement();
		// Correct old random wave LFO speeds
		if (e.attribute("wave").toInt() == 6)
		{
			e.setAttribute("speed",0.01f);
		}
	}
}


}
