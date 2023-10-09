/*
 * UpgradeTo13Misc.h
 *   Functor for upgrading data files for BLURB
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

#pragma once

#ifndef LMMS_CLASS_NAME_GUARD_H
#define LMMS_CLASS_NAME_GUARD_H

#include "datafile/DataFileUpgrade.h"


namespace lmms
{


class UpgradeTo13NoHiddenClipNames : public DataFileUpgrade
{
public:
	UpgradeTo13NoHiddenClipNames(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


class UpgradeTo13AutomationNodes : public DataFileUpgrade
{
public:
	UpgradeTo13AutomationNodes(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/** \brief TripleOscillator switched to using high-quality, alias-free oscillators by default
 *
 * Older projects were made without this feature and would sound differently if loaded
 * with the new default setting. This upgrade routine preserves their old behavior.
 */
class UpgradeTo13DefaultTripleOscillatorHQ : public DataFileUpgrade
{
public:
	UpgradeTo13DefaultTripleOscillatorHQ(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


class UpgradeTo13SampleAndHold : public DataFileUpgrade
{
public:
	UpgradeTo13SampleAndHold(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


} // namespace lmms

#endif // LMMS_CLASS_NAME_GUARD_H

