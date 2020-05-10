/*
 * MicrotunerConfig.cpp - configuration widget for scales and keymaps
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

#include "MicrotunerConfig.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

#include "ComboBox.h"
#include "embed.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "MainWindow.h"


MicrotunerConfig::MicrotunerConfig() :
	QWidget()
{
	setWindowIcon(embed::getIconPixmap("microtuner"));
	setWindowTitle(tr("Microtuner"));

	int comboHeight = 22;	// using the same constant as zoom combo in SongEditor

	// Organize into 2 main columns: scales and keymaps
	QGridLayout *microtunerLayout = new QGridLayout();
	microtunerLayout->setSpacing(2);

	// Scale sub-column
	QLabel *scaleLabel = new QLabel(tr("Scale:"));
	microtunerLayout->addWidget(scaleLabel, 0, 0, 1, 2, Qt::AlignBottom);

	ComboBox *scaleCombo = new ComboBox();
	scaleCombo->setFixedHeight(comboHeight);
	microtunerLayout->addWidget(scaleCombo, 1, 0, 1, 2);

	QLineEdit *scaleNameEdit = new QLineEdit("12-TET");
	scaleNameEdit->setToolTip(tr("Scale description"));
	microtunerLayout->addWidget(scaleNameEdit, 2, 0, 1, 2);

	QPushButton *scaleLoadButton = new QPushButton(tr("Load"));
	QPushButton *scaleSaveButton = new QPushButton(tr("Save"));
	microtunerLayout->addWidget(scaleLoadButton, 3, 0, 1, 1);
	microtunerLayout->addWidget(scaleSaveButton, 3, 1, 1, 1);

	QTextEdit *scaleTextEdit = new QTextEdit();
	scaleTextEdit->setAcceptRichText(false);
	scaleTextEdit->setPlainText("100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n1200.0");
	microtunerLayout->addWidget(scaleTextEdit, 4, 0, 4, 2, Qt::AlignLeft | Qt::AlignTop);

	QPushButton *applyScaleButton = new QPushButton(tr("Apply scale"));
	microtunerLayout->addWidget(applyScaleButton, 8, 0, 1, 2);

	// Mapping sub-column
	QLabel *mappingLabel = new QLabel(tr("Keymap:"));
	microtunerLayout->addWidget(mappingLabel, 0, 2, 1, 2, Qt::AlignBottom);

	ComboBox *keymapCombo = new ComboBox();
	keymapCombo->setFixedHeight(comboHeight);
	microtunerLayout->addWidget(keymapCombo, 1, 2, 1, 2);

	QLineEdit *keymapNameEdit = new QLineEdit("default");
	keymapNameEdit->setToolTip(tr("Keymap description"));
	microtunerLayout->addWidget(keymapNameEdit, 2, 2, 1, 2);

	QPushButton *mappingLoadButton = new QPushButton(tr("Load"));
	QPushButton *mappingSaveButton = new QPushButton(tr("Save"));
	microtunerLayout->addWidget(mappingLoadButton, 3, 2, 1, 1);
	microtunerLayout->addWidget(mappingSaveButton, 3, 3, 1, 1);

	QTextEdit *mappingTextEdit = new QTextEdit();
	mappingTextEdit->setAcceptRichText(false);
	mappingTextEdit->setPlainText("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11");
	microtunerLayout->addWidget(mappingTextEdit, 4, 2, 1, 2, Qt::AlignRight | Qt::AlignTop);

	// Mapping ranges
	QGridLayout *keymapRangeLayout = new QGridLayout();
	microtunerLayout->addLayout(keymapRangeLayout, 5, 2, 2, 2, Qt::AlignCenter | Qt::AlignTop);

	QLabel *firstNoteLabel = new QLabel(tr("First:"));
	keymapRangeLayout->addWidget(firstNoteLabel, 0, 0, Qt::AlignBottom);
	LcdSpinBox *firstNoteSpin = new LcdSpinBox(3, NULL, tr("First note"));
	firstNoteSpin->setToolTip(tr("First MIDI note that will be tuned"));
	keymapRangeLayout->addWidget(firstNoteSpin, 1, 0);

	QLabel *lastNoteLabel = new QLabel(tr("Last:"));
	keymapRangeLayout->addWidget(lastNoteLabel, 0, 1, Qt::AlignBottom);
	LcdSpinBox *lastNoteSpin = new LcdSpinBox(3, NULL, tr("Last note"));
	lastNoteSpin->setToolTip(tr("Last MIDI note that will be tuned"));
	keymapRangeLayout->addWidget(lastNoteSpin, 1, 1);

	QLabel *middleNoteLabel = new QLabel(tr("Middle:"));
	keymapRangeLayout->addWidget(middleNoteLabel, 0, 2, Qt::AlignBottom);
	LcdSpinBox *middleNoteSpin = new LcdSpinBox(3, NULL, tr("Middle note"));
	middleNoteSpin->setToolTip(tr("First key in the keymap is mapped to this MIDI note"));
	keymapRangeLayout->addWidget(middleNoteSpin, 1, 2);

	Knob *baseFreqKnob = new Knob(knobBright_26);
//	baseFreqKnob->setModel(it->m_microtuner.baseFreqModel());
	baseFreqKnob->setLabel(tr("BASE FREQ"));
	baseFreqKnob->setHintText(tr("Base note frequency:"), " " + tr("Hz"));
	baseFreqKnob->setToolTip(tr("Base note frequency"));
	microtunerLayout->addWidget(baseFreqKnob, 7, 2, 1, 2);


	QPushButton *applyKeymapButton = new QPushButton(tr("Apply keymap"));
	microtunerLayout->addWidget(applyKeymapButton, 8, 2, 1, 2);


	this->setLayout(microtunerLayout);

	// Add to the main window and setup fixed size etc.
	QMdiSubWindow *subWin = gui->mainWindow()->addWindowedWidget(this);

	subWin->setAttribute(Qt::WA_DeleteOnClose, false);
	subWin->setFixedWidth(300);
	subWin->setMinimumHeight(300);
	subWin->hide();

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);
}

/*
void MicrotunerConfig::applyScale()
{
	//TODO: validate entries, update stored scale, update instrument combo boxes and LUTs
}
*/

void MicrotunerConfig::saveSettings(QDomDocument &document, QDomElement &element)
{
	MainWindow::saveWidgetState(this, element);
}


void MicrotunerConfig::loadSettings(const QDomElement &element)
{
	MainWindow::restoreWidgetState(this, element);
}


void MicrotunerConfig::closeEvent(QCloseEvent *ce)
{
	if (parentWidget())	{parentWidget()->hide();}
	else {hide();}
	ce->ignore();
}
