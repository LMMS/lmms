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
	void updateScaleList(int index);
	void updateKeymapList(int index);
	void updateScaleForm();
	void updateKeymapForm();

protected:
	void closeEvent(QCloseEvent *ce) override;

private slots:
	bool loadScaleFromFile();
	bool loadKeymapFromFile();
	bool saveScaleToFile();
	bool saveKeymapToFile();

private:
	bool validateScaleForm();
	bool validateKeymapForm();

	bool applyScale();
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
