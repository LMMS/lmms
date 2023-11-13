/*
 * UpgradeTo04.h
 *   Functor for upgrading data files from 0.3.0 through 0.4.0
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

#ifndef LMMS_UPGRADE_04_H
#define LMMS_UPGRADE_04_H

#include "datafile/DataFileUpgrade.h"


namespace lmms
{

/*
 * Upgrade to 0.4.0-20080104
 *
 * Upgrade to version 0.4.0-20080104 from some version greater than
 * or equal to 0.3.0 (final)
 */
class UpgradeTo0_4_0_20080104 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080104(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-20080118
 *
 * Upgrade to version 0.4.0-20080118 from some version greater than
 * or equal to 0.4.0-20080104
 */
class UpgradeTo0_4_0_20080118 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080118(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-20080129
 *
 * Upgrade to version 0.4.0-20080129 from some version greater than
 * or equal to 0.4.0-20080118
 */
class UpgradeTo0_4_0_20080129 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080129(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-20080409
 *
 * Upgrade to version 0.4.0-20080409 from some version greater than
 * or equal to 0.4.0-20080129
 */
class UpgradeTo0_4_0_20080409 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080409(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-20080607
 *
 * Upgrade to version 0.4.0-20080607 from some version greater than
 * or equal to 0.3.0-20080409
 */
class UpgradeTo0_4_0_20080607 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080607(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-20080622
 *
 * Upgrade to version 0.4.0-20080622 from some version greater than
 * or equal to 0.3.0-20080607
 */
class UpgradeTo0_4_0_20080622 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_20080622(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-beta1
 *
 * Upgrade to version 0.4.0-beta1 from some version greater than
 * or equal to 0.4.0-20080622
 * convert binary effect-key-blobs to XML
 */
class UpgradeTo0_4_0_beta1 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_beta1(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};

/*
 * Upgrade to 0.4.0-rc2
 *
 * Upgrade to version 0.4.0-rc2 from some version greater than
 * or equal to 0.4.0-beta1
 */
class UpgradeTo0_4_0_rc2 : public DataFileUpgrade
{
public:
	UpgradeTo0_4_0_rc2(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;
};


} // namespace lmms

#endif // LMMS_UPGRADE_04_H

