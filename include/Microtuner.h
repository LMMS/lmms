/*
 * Microtuner.h - manage tuning and scale information of an instrument
 *
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#ifndef LMMS_MICROTUNER_H
#define LMMS_MICROTUNER_H

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "JournallingObject.h"

namespace lmms
{

class LMMS_EXPORT Microtuner : public Model, public JournallingObject
{
	Q_OBJECT
public:
	explicit Microtuner();

	bool enabled() const {return m_enabledModel.value();}
	bool keyRangeImport() const {return enabled() && m_keyRangeImportModel.value();}
	int currentScale() const {return m_scaleModel.value();}
	int currentKeymap() const {return m_keymapModel.value();}

	BoolModel *enabledModel() {return &m_enabledModel;}
	ComboBoxModel *scaleModel() {return &m_scaleModel;}
	ComboBoxModel *keymapModel() {return &m_keymapModel;}
	BoolModel *keyRangeImportModel() {return &m_keyRangeImportModel;}

	int firstKey() const;
	int lastKey() const;
	int baseKey() const;
	float baseFreq() const;

	float keyToFreq(int key, int userBaseNote) const;
	int octaveSize() const;

	QString nodeName() const override {return "microtuner";}
	void saveSettings(QDomDocument & document, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;

protected slots:
	void updateScaleList(int index);
	void updateKeymapList(int index);

private:
	BoolModel m_enabledModel;               //!< Enable microtuner (otherwise using 12-TET @440 Hz)
	ComboBoxModel m_scaleModel;
	ComboBoxModel m_keymapModel;
	BoolModel m_keyRangeImportModel;

};

} // namespace lmms

#endif // LMMS_MICROTUNER_H
