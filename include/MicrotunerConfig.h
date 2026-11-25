/*
 * MicrotunerConfig.h - configuration widget for scales and keymaps
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

#ifndef LMMS_GUI_MICROTUNER_CONFIG_H
#define LMMS_GUI_MICROTUNER_CONFIG_H

#include <QWidget>

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "SerializingObject.h"

class QLineEdit;
class QPlainTextEdit;

namespace lmms::gui
{


class LMMS_EXPORT MicrotunerConfig : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	MicrotunerConfig();

	void saveSettings(QDomDocument &document, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;

	inline QString nodeName() const override
	{
		return "MicrotunerConfig";
	}
	QSize sizeHint() const override {return QSize(300, 400);}

public slots:
	//! @brief Update list of available scales.
	//! @param index Index of the scale to update; update all scales if -1 or out of range.
	void updateScaleList(int index);

	//! @brief Update list of available keymaps.
	//! @param index Index of the keymap to update; update all keymaps if -1 or out of range.
	void updateKeymapList(int index);

	//! @brief Fill all the scale-related values based on currently selected scale
	void updateScaleForm();

	//! @brief Fill all the keymap-related values based on currently selected keymap
	void updateKeymapForm();

private slots:
	//! @brief Parse an .scl file and apply the loaded scale if it is valid
	//! @return true if input is valid, false if problems were detected
	bool loadScaleFromFile();

	//! @brief Parse a .kbm file and apply the loaded keymap if it is valid
	//! @return true if input is valid, false if problems were detected
	bool loadKeymapFromFile();

	//! @brief Save currently entered scale definition as .scl file
	//! @return true if input is valid, false if problems were detected
	bool saveScaleToFile();

	//! @brief Save currently entered keymap definition as .kbm file
	//! @return true if input is valid, false if problems were detected
	bool saveKeymapToFile();

private:
	//! @brief Validate the scale name and entered interval definitions
	//! @return true if input is valid, false if problems were detected
	bool validateScaleForm();

	//! @brief Validate the entered key mapping and other values
	//! @return true if input is valid, false if problems were detected
	bool validateKeymapForm();

	//! @brief Parse and apply the entered scale definition
	//! @return true if input is valid, false if problems were detected
	bool applyScale();

	//! @brief Parse and apply the entered keymap definition
	//! @return true if input is valid, false if problems were detected
	bool applyKeymap();

	ComboBoxModel m_scaleComboModel;        //!< ID of scale currently selected for editing
	ComboBoxModel m_keymapComboModel;       //!< ID of keymap currently selected for editing

	QLineEdit *m_scaleNameEdit;             //!< edit field for the scale name or description
	QLineEdit *m_keymapNameEdit;            //!< edit field for the keymap name or description

	QPlainTextEdit *m_scaleTextEdit;        //!< text editor field for interval definitions
	QPlainTextEdit *m_keymapTextEdit;       //!< text editor field for key mappings

	IntModel m_firstKeyModel;               //!< model for spinbox of currently edited first key
	IntModel m_lastKeyModel;                //!< model for spinbox of currently edited last key
	IntModel m_middleKeyModel;              //!< model for spinbox of currently edited middle key

	IntModel m_baseKeyModel;                //!< model for spinbox of currently edited base key
	FloatModel m_baseFreqModel;             //!< model for spinbox of currently edited base note frequency
};


} // namespace lmms::gui

#endif // LMMS_GUI_MICROTUNER_CONFIG_H
