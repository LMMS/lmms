/*
 * UpgradeTo1_3_0.h
 *   Functor for upgrading data files to 1.3.0
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

#ifndef LMMS_UPGRADE_TO_13_H
#define LMMS_UPGRADE_TO_13_H

#include "datafile/DataFileUpgrade.h"

#include <map>


namespace lmms
{

// upgrade functor for 1.3.0
class UpgradeTo1_3_0 : public DataFileUpgrade
{
public:
	UpgradeTo1_3_0(DataFile& document) : DataFileUpgrade(document) {}

	void upgrade() override;

	void upgrade_effects();
	void upgrade_calf();
	void rename_plugins();
	void upgrade_ports();
	void upgrade_multiband_ports();
	void upgrade_pulsator_ports();
	void upgrade_vintagedelay_ports();
	void upgrade_equalizer_ports();
	void upgrade_saturator_ports();
	void upgrade_stereotools_ports();
	void upgrade_ampitchshift_ports();

// Functor state
private:
	QDomNodeList m_elements;

	QDomElement attribute;
	QDomElement effect;
	QString attrName;
	QString attrVal;
	QString plugin;

	using StringMap = std::map<QString, QString>;

	const StringMap pluginNames = {
		{"Sidechaincompressor", "SidechainCompressor"},
		{"Sidechaingate", "SidechainGate"},
		{"Multibandcompressor", "MultibandCompressor"},
		{"Multibandgate", "MultibandGate"},
		{"Multibandlimiter", "MultibandLimiter"},
	};
};


} // namespace lmms

#endif // LMMS_UPGRADE_TO_13_H
