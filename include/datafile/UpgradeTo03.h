/*
 * UpgradeTo03.h
 *   Functor for upgrading data files from pre lmms 0.3.0
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

#ifndef LMMS_UPGRADE_03_H
#define LMMS_UPGRADE_03_H

#include "datafile/DataFileUpgrade.h"


namespace lmms
{

/*
 * Upgrade to 0.3.0-rc2
 *
 * Upgrade to version 0.3.0-rc2 from some version greater than
 * or equal to 0.2.1-20070508
 */
class UpgradeTo0_3_0_RC2 : public DataFileUpgrade
{
public:
	UpgradeTo0_3_0_RC2(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.3.0
 *
 * Upgrade to version 0.3.0 (final) from some version greater than
 * or equal to 0.3.0-rc2
 */
class UpgradeTo0_3_0 : public DataFileUpgrade
{
public:
	UpgradeTo0_3_0(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


} // namespace lmms

#endif // LMMS_UPGRADE_03_H

