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

#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>

#include "ComboBox.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "lmms_constants.h"
#include "MainWindow.h"
#include "Song.h"


MicrotunerConfig::MicrotunerConfig() :
	QWidget(),
	m_scaleComboModel(NULL, tr("Selected scale")),
	m_keymapComboModel(NULL, tr("Selected keymap")),
	m_firstKeyModel(0, 0, NumKeys - 1, NULL, tr("First key")),
	m_lastKeyModel(NumKeys - 1, 0, NumKeys - 1, NULL, tr("Last key")),
	m_middleKeyModel(60, 0, NumKeys - 1, NULL, tr("Middle key")),
	m_baseFreqModel(440.f, 0.1f, 9999.999f, 0.001f, NULL, tr("Base note frequency"))
{
	setWindowIcon(embed::getIconPixmap("microtuner"));
	setWindowTitle(tr("Microtuner"));

	int comboHeight = 22;	// using the same constant as zoom combo in SongEditor

	// Organize into 2 main columns: scales and keymaps
	QGridLayout *microtunerLayout = new QGridLayout();
	microtunerLayout->setSpacing(2);

	// ----------------------------------
	// Scale sub-column
	//
	QLabel *scaleLabel = new QLabel(tr("Scale:"));
	microtunerLayout->addWidget(scaleLabel, 0, 0, 1, 2, Qt::AlignBottom);

	for (unsigned int i = 0; i < MaxScaleCount; i++)
	{
		m_scaleComboModel.addItem(Engine::getSong()->getScale(i).getDescription());
	}
	ComboBox *scaleCombo = new ComboBox();
	scaleCombo->setFixedHeight(comboHeight);
	scaleCombo->setModel(&m_scaleComboModel);
	microtunerLayout->addWidget(scaleCombo, 1, 0, 1, 2);
	connect(&m_scaleComboModel, &ComboBoxModel::dataChanged, [=] {updateScaleForm();});

	m_scaleNameEdit = new QLineEdit("12-TET");
	m_scaleNameEdit->setToolTip(tr("Scale description. Cannot start with \"!\" and cannot contain a newline character."));
	microtunerLayout->addWidget(m_scaleNameEdit, 2, 0, 1, 2);

	QPushButton *scaleLoadButton = new QPushButton(tr("Load"));
	QPushButton *scaleSaveButton = new QPushButton(tr("Save"));
	microtunerLayout->addWidget(scaleLoadButton, 3, 0, 1, 1);
	microtunerLayout->addWidget(scaleSaveButton, 3, 1, 1, 1);

	m_scaleTextEdit = new QTextEdit();
	m_scaleTextEdit->setAcceptRichText(false);
	m_scaleTextEdit->setPlainText("100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n1200.0");
	m_scaleTextEdit->setToolTip(tr("Enter intervals on separate lines. Numbers containing a decimal point are treated as cents. Other input is treated as an integer ratio and must be in the form of \'a/b\' or \'a\'. Unity (0.0 cents or ratio 1/1) is always present as a hidden first value."));
	microtunerLayout->addWidget(m_scaleTextEdit, 4, 0, 5, 2, Qt::AlignLeft | Qt::AlignTop);

	QPushButton *applyScaleButton = new QPushButton(tr("Apply scale"));
	microtunerLayout->addWidget(applyScaleButton, 9, 0, 1, 2);
	connect(applyScaleButton, &QPushButton::clicked, [=] {applyScale();});

	// ----------------------------------
	// Mapping sub-column
	//
	QLabel *keymapLabel = new QLabel(tr("Keymap:"));
	microtunerLayout->addWidget(keymapLabel, 0, 2, 1, 2, Qt::AlignBottom);

	for (unsigned int i = 0; i < MaxKeymapCount; i++)
	{
		m_keymapComboModel.addItem(Engine::getSong()->getKeymap(i).getDescription());
	}
	ComboBox *keymapCombo = new ComboBox();
	keymapCombo->setFixedHeight(comboHeight);
	keymapCombo->setModel(&m_keymapComboModel);
	microtunerLayout->addWidget(keymapCombo, 1, 2, 1, 2);
	connect(&m_keymapComboModel, &ComboBoxModel::dataChanged, [=] {updateKeymapForm();});

	m_keymapNameEdit = new QLineEdit("default");
	m_keymapNameEdit->setToolTip(tr("Keymap description. Cannot start with \"!\" and cannot contain a newline character."));
	microtunerLayout->addWidget(m_keymapNameEdit, 2, 2, 1, 2);

	QPushButton *keymapLoadButton = new QPushButton(tr("Load"));
	QPushButton *keymapSaveButton = new QPushButton(tr("Save"));
	microtunerLayout->addWidget(keymapLoadButton, 3, 2, 1, 1);
	microtunerLayout->addWidget(keymapSaveButton, 3, 3, 1, 1);

	m_keymapTextEdit = new QTextEdit();
	m_keymapTextEdit->setAcceptRichText(false);
	m_keymapTextEdit->setPlainText("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11");
	m_keymapTextEdit->setToolTip(tr("Enter key mappings on separate lines. Each line assigns a scale degree to a MIDI key, starting with the middle key and continuing in sequence. The pattern repeats for keys outside of the explicit keymap range. Multiple keys can be mapped to the same scale degree. Enter \'x\' if you wish to leave the key disabled / not mapped."));
	microtunerLayout->addWidget(m_keymapTextEdit, 4, 2, 1, 2, Qt::AlignRight | Qt::AlignTop);

	// Mapping ranges
	QGridLayout *keymapRangeLayout = new QGridLayout();
	microtunerLayout->addLayout(keymapRangeLayout, 5, 2, 2, 2, Qt::AlignCenter | Qt::AlignTop);

	QLabel *firstKeyLabel = new QLabel(tr("First:"));
	keymapRangeLayout->addWidget(firstKeyLabel, 0, 0, Qt::AlignBottom);
	LcdSpinBox *firstKeySpin = new LcdSpinBox(3, NULL, tr("First key"));
	firstKeySpin->setToolTip(tr("First MIDI key that will be mapped"));
	firstKeySpin->setModel(&m_firstKeyModel);
	keymapRangeLayout->addWidget(firstKeySpin, 1, 0);

	QLabel *lastKeyLabel = new QLabel(tr("Last:"));
	keymapRangeLayout->addWidget(lastKeyLabel, 0, 1, Qt::AlignBottom);
	LcdSpinBox *lastKeySpin = new LcdSpinBox(3, NULL, tr("Last key"));
	lastKeySpin->setToolTip(tr("Last MIDI key that will be mapped"));
	lastKeySpin->setModel(&m_lastKeyModel);
	keymapRangeLayout->addWidget(lastKeySpin, 1, 1);

	QLabel *middleKeyLabel = new QLabel(tr("Middle:"));
	keymapRangeLayout->addWidget(middleKeyLabel, 0, 2, Qt::AlignBottom);
	LcdSpinBox *middleKeySpin = new LcdSpinBox(3, NULL, tr("Middle key"));
	middleKeySpin->setToolTip(tr("First line in the keymap refers to this MIDI key"));
	middleKeySpin->setModel(&m_middleKeyModel);
	keymapRangeLayout->addWidget(middleKeySpin, 1, 2);

	QLabel *baseFreqLabel = new QLabel(tr("Base note frequency:"));
	microtunerLayout->addWidget(baseFreqLabel, 7, 2, 1, 2, Qt::AlignBottom);
	LcdFloatSpinBox *baseFreqSpin = new LcdFloatSpinBox(4, 3, NULL, tr("Base note frequency"));
	baseFreqSpin->setModel(&m_baseFreqModel);
	baseFreqSpin->setToolTip(tr("Base note frequency"));
	microtunerLayout->addWidget(baseFreqSpin, 8, 2, 1, 2);

	QPushButton *applyKeymapButton = new QPushButton(tr("Apply keymap"));
	microtunerLayout->addWidget(applyKeymapButton, 9, 2, 1, 2);
	connect(applyKeymapButton, &QPushButton::clicked, [=] {applyKeymap();});

	updateScaleForm();
	updateKeymapForm();
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


/**
 * \brief Fill all the scale-related values based on currently selected scale
 */
void MicrotunerConfig::updateScaleForm()
{
	const unsigned int scaleID = m_scaleComboModel.value();

	m_scaleNameEdit->setText(Engine::getSong()->getScale(scaleID).getDescription());

	// fill in the intervals
	m_scaleTextEdit->setText("");
	const std::vector<Interval> &intervals = Engine::getSong()->getScale(scaleID).getIntervals();
	for (unsigned int i = 1; i < intervals.size(); i++)
	{
		m_scaleTextEdit->append(intervals[i].getString());
	}
	// scroll back to the top
	QTextCursor tmp = m_scaleTextEdit->textCursor();
	tmp.movePosition(QTextCursor::Start);
	m_scaleTextEdit->setTextCursor(tmp);
}


/**
 * \brief Fill all the keymap-related values based on currently selected keymap
 */
void MicrotunerConfig::updateKeymapForm()
{
	const unsigned int keymapID = m_keymapComboModel.value();

	m_keymapNameEdit->setText(Engine::getSong()->getKeymap(keymapID).getDescription());

	m_keymapTextEdit->setText("");
	const std::vector<int> &map = Engine::getSong()->getKeymap(keymapID).getMap();
	for (unsigned int i = 0; i < map.size(); i++)
	{
		m_keymapTextEdit->append(QString::number(map[i]));
	}
	QTextCursor tmp = m_keymapTextEdit->textCursor();
	tmp.movePosition(QTextCursor::Start);
	m_keymapTextEdit->setTextCursor(tmp);
}


/**
 * \brief Validate the entered interval definitions
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::validateScaleForm()
{
	auto fail = [=](QString message) {QMessageBox::critical(this, tr("Scale parsing error"), message);};

	// check name
	QString name = m_scaleNameEdit->text();
	if (name.length() > 0 && name[0] == '!') {fail(tr("Scale name cannot start with an exclamation mark")); return false;}
	if (name.contains('\n')) {fail(tr("Scale name cannot contain a new-line character")); return false;}

	// check intervals
	QStringList input = m_scaleTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
	for (auto &line: input)
	{
		if (line.size() == 0) {continue;}
		if (line[0] == '!') {continue;}		// comment
		QString firstSection = line.section(QRegExp("\\s+|/"), 0, 0, QString::SectionSkipEmpty);
		if (firstSection.contains('.'))		// cent mode
		{
			bool ok = true;
			firstSection.toFloat(&ok);
			if (!ok) {fail(tr("Interval defined in cents cannot be converted to a number")); return false;}
		}
		else								// ratio mode
		{
			bool ok = true;
			int num = 1, den = 1;
			den = firstSection.toInt(&ok);
			if (!ok) {fail(tr("Denominator of an interval defined as a ratio cannot be converted to a number")); return false;}
			if (line.contains('/'))
			{
				num = line.split('/').at(1).section(QRegExp("\\s+"), 0, 0, QString::SectionSkipEmpty).toInt(&ok);
			}
			if (!ok) {fail(tr("Numerator of an interval defined as a ratio cannot be converted to a number")); return false;}
			if (num * den < 0) {fail(tr("Interval defined as a ratio cannot be negative")); return false;}
		}
	}
	return true;
}


/**
 * \brief Validate the entered key mapping and other values
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::validateKeymapForm()
{
	auto fail = [=](QString message) {QMessageBox::critical(this, tr("Keymap parsing error"), message);};

	// check name
	QString name = m_keymapNameEdit->text();
	if (name.length() > 0 && name[0] == '!') {fail(tr("Keymap name cannot start with an exclamation mark")); return false;}
	if (name.contains('\n')) {fail(tr("Keymap name cannot contain a new-line character")); return false;}

	// check key mappings
	QStringList input = m_keymapTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
	for (auto &line: input)
	{
		if (line.size() == 0) {continue;}
		if (line[0] == '!') {continue;}			// comment
		QString firstSection = line.section(QRegExp("\\s+"), 0, 0, QString::SectionSkipEmpty);
		if (firstSection == "x") {continue;}	// not mapped
		// otherwise must contain a number
		bool ok = true;
		int deg = 0;
		deg = firstSection.toInt(&ok);
		if (!ok) {fail(tr("Scale degree cannot be converted to a whole number")); return false;}
		if (deg < 0) {fail(tr("Scale degree cannot be negative")); return false;}
	}

	return true;
}



bool MicrotunerConfig::applyScale()
{
	validateScaleForm();

	//TODO: update stored scale

	return true;
}


bool MicrotunerConfig::applyKeymap()
{
	validateKeymapForm();

	//TODO: update stored keymap and other values

	return true;
}


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
