/*
 * UpgradeTo1Pre3.h
 *   Functor for upgrading data files for from 1.0 release up to 1.2.2 stable
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

#ifndef LMMS_UPGRADE_1PRE3_H
#define LMMS_UPGRADE_1PRE3_H

#include "datafile/DataFileUpgrade.h"


namespace lmms
{

/*
 * Upgrade to 1.0.99
 *
 * Upgrade to version 1.0.99 from some version less than 1.0.99
 */
class UpgradeTo1_0_99 : public DataFileUpgrade
{
public:
	UpgradeTo1_0_99(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


/*
 * Upgrade to 1.1.0
 *
 * Upgrade to version 1.1.0 from some version less than 1.1.0
 */
class UpgradeTo1_1_0 : public DataFileUpgrade
{
public:
	UpgradeTo1_1_0(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


/*
 * Upgrade to 1.1.19
 *
 * Upgrade to version 1.1.91 from some version less than 1.1.91
 */
class UpgradeTo1_1_91 : public DataFileUpgrade
{
public:
	UpgradeTo1_1_91(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


/*
 * Upgrade to 1.2.0 rc3
 *
 * Upgrade from earlier bbtrack beat note behaviour of adding steps if
 * a note is placed after the last step.
 */
class UpgradeTo1_2_0_RC3 : public DataFileUpgrade
{
public:
	UpgradeTo1_2_0_RC3(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;

	void upgradeElementRc2_42(QDomElement& el);
};


} // namespace lmms

#endif // LMMS_UPGRADE_1PRE3_H

