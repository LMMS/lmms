/*
 * UpgradeTo02.h - upgrades pre lmms 0.2.0 files
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

#ifndef LMMS_UPGRADE_02_H
#define LMMS_UPGRADE_02_H

#include "datafile/DataFileUpgrade.h"


namespace lmms
{

/*
 * Upgrade to 0.2.1-20070501
 *
 * Upgrade to version 0.2.1-20070501
 */
class UpgradeTo0_2_1_20070501 : public DataFileUpgrade
{
public:
	UpgradeTo0_2_1_20070501(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.2.1-20070508
 *
 * Upgrade to version 0.2.1-20070508 from some version greater than
 * or equal to 0.2.1-20070501
 */
class UpgradeTo0_2_1_20070508 : public DataFileUpgrade
{
public:
	UpgradeTo0_2_1_20070508(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


} // namespace lmms

#endif // LMMS_UPGRADE_02_H

