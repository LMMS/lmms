/*
 * UpgradeMixerRename.cpp
 *   Functor for upgrading data files after the fxmixer tag rename
 *   and fxch attribute rename.
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

#include "UpgradeMixerRename.h"

namespace lmms
{


void UpgradeMixerRename::upgrade()
{
	// Change nodename <fxmixer> to <mixer>
	QDomNodeList fxmixer = m_document.elementsByTagName("fxmixer");
	renameElements(fxmixer, "mixer");

	// Change nodename <fxchannel> to <mixerchannel>
	QDomNodeList fxchannel = m_document.elementsByTagName("fxchannel");
	renameElements(fxchannel, "mixerchannel");

	// Change the attribute fxch of element <instrumenttrack> to mixch
	QDomNodeList fxch = m_document.elementsByTagName("instrumenttrack");
	renameAttribute(fxch, "fxch", "mixch");

	// Change the attribute fxch of element <sampletrack> to mixch
	fxch = m_document.elementsByTagName("sampletrack");
	renameAttribute(fxch, "fxch", "mixch");
}


} // namespace lmms
