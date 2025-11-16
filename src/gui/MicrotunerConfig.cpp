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

#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

#include "ComboBox.h"
#include "embed.h"
#include "Engine.h"
#include "FileDialog.h"
#include "GuiApplication.h"
#include "Keymap.h"
#include "LcdFloatSpinBox.h"
#include "LcdSpinBox.h"
#include "lmms_constants.h"
#include "lmmsversion.h"
#include "MainWindow.h"
#include "Note.h"
#include "Scale.h"
#include "Song.h"
#include "SubWindow.h"

namespace lmms::gui
{


MicrotunerConfig::MicrotunerConfig() :
	QWidget(),
	m_scaleComboModel(nullptr, tr("Selected scale slot")),
	m_keymapComboModel(nullptr, tr("Selected keymap slot")),
	m_firstKeyModel(0, 0, NumKeys - 1, nullptr, tr("First key")),
	m_lastKeyModel(NumKeys - 1, 0, NumKeys - 1, nullptr, tr("Last key")),
	m_middleKeyModel(DefaultMiddleKey, 0, NumKeys - 1, nullptr, tr("Middle key")),
	m_baseKeyModel(DefaultBaseKey, 0, NumKeys - 1, nullptr, tr("Base key")),
	m_baseFreqModel(DefaultBaseFreq, 0.1f, 9999.999f, 0.001f, nullptr, tr("Base note frequency"))
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namespace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	setWindowIcon(embed::getIconPixmap("microtuner"));
	setWindowTitle(tr("Microtuner Configuration"));

	// Organize into 2 main columns: scales and keymaps
	auto microtunerLayout = new QGridLayout();
	microtunerLayout->setSpacing(2);

	// ----------------------------------
	// Scale sub-column
	//
	auto scaleLabel = new QLabel(tr("Scale slot to edit:"));
	microtunerLayout->addWidget(scaleLabel, 0, 0, 1, 2, Qt::AlignBottom);

	for (unsigned int i = 0; i < MaxScaleCount; i++)
	{
		m_scaleComboModel.addItem(QString::number(i) + ": " + Engine::getSong()->getScale(i)->getDescription());
	}
	auto scaleCombo = new ComboBox();
	scaleCombo->setModel(&m_scaleComboModel);
	microtunerLayout->addWidget(scaleCombo, 1, 0, 1, 2);
	connect(&m_scaleComboModel, &ComboBoxModel::dataChanged, this, &MicrotunerConfig::updateScaleForm);

	m_scaleNameEdit = new QLineEdit("12-TET");
	m_scaleNameEdit->setToolTip(tr("Scale description. Cannot start with \"!\" and cannot contain a newline character."));
	microtunerLayout->addWidget(m_scaleNameEdit, 2, 0, 1, 2);

	auto loadScaleButton = new QPushButton(tr("Load"));
	auto saveScaleButton = new QPushButton(tr("Save"));
	loadScaleButton->setToolTip(tr("Load scale definition from a file."));
	saveScaleButton->setToolTip(tr("Save scale definition to a file."));
	microtunerLayout->addWidget(loadScaleButton, 3, 0, 1, 1);
	microtunerLayout->addWidget(saveScaleButton, 3, 1, 1, 1);
	connect(loadScaleButton, &QPushButton::clicked, this, &MicrotunerConfig::loadScaleFromFile);
	connect(saveScaleButton, &QPushButton::clicked, this, &MicrotunerConfig::saveScaleToFile);

	m_scaleTextEdit = new QPlainTextEdit();
	m_scaleTextEdit->setPlainText("100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n1200.0");
	m_scaleTextEdit->setToolTip(tr("Enter intervals on separate lines. Numbers containing a decimal point are treated as cents.\nOther inputs are treated as integer ratios and must be in the form of \'a/b\' or \'a\'.\nUnity (0.0 cents or ratio 1/1) is always present as a hidden first value; do not enter it manually."));
	microtunerLayout->addWidget(m_scaleTextEdit, 4, 0, 2, 2);

	auto applyScaleButton = new QPushButton(tr("Apply scale changes"));
	applyScaleButton->setToolTip(tr("Verify and apply changes made to the selected scale. To use the scale, select it in the settings of a supported instrument."));
	microtunerLayout->addWidget(applyScaleButton, 6, 0, 1, 2);
	connect(applyScaleButton, &QPushButton::clicked, this, &MicrotunerConfig::applyScale);

	// ----------------------------------
	// Mapping sub-column
	//
	auto keymapLabel = new QLabel(tr("Keymap slot to edit:"));
	microtunerLayout->addWidget(keymapLabel, 0, 2, 1, 2, Qt::AlignBottom);

	for (unsigned int i = 0; i < MaxKeymapCount; i++)
	{
		m_keymapComboModel.addItem(QString::number(i) + ": " + Engine::getSong()->getKeymap(i)->getDescription());
	}
	auto keymapCombo = new ComboBox();
	keymapCombo->setModel(&m_keymapComboModel);
	microtunerLayout->addWidget(keymapCombo, 1, 2, 1, 2);
	connect(&m_keymapComboModel, &ComboBoxModel::dataChanged, this, &MicrotunerConfig::updateKeymapForm);

	m_keymapNameEdit = new QLineEdit("default");
	m_keymapNameEdit->setToolTip(tr("Keymap description. Cannot start with \"!\" and cannot contain a newline character."));
	microtunerLayout->addWidget(m_keymapNameEdit, 2, 2, 1, 2);

	auto loadKeymapButton = new QPushButton(tr("Load"));
	auto saveKeymapButton = new QPushButton(tr("Save"));
	loadKeymapButton->setToolTip(tr("Load key mapping definition from a file."));
	saveKeymapButton->setToolTip(tr("Save key mapping definition to a file."));
	microtunerLayout->addWidget(loadKeymapButton, 3, 2, 1, 1);
	microtunerLayout->addWidget(saveKeymapButton, 3, 3, 1, 1);
	connect(loadKeymapButton, &QPushButton::clicked, this, &MicrotunerConfig::loadKeymapFromFile);
	connect(saveKeymapButton, &QPushButton::clicked, this, &MicrotunerConfig::saveKeymapToFile);

	m_keymapTextEdit = new QPlainTextEdit();
	m_keymapTextEdit->setPlainText("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11");
	m_keymapTextEdit->setToolTip(tr("Enter key mappings on separate lines. Each line assigns a scale degree to a MIDI key,\nstarting with the middle key and continuing in sequence.\nThe pattern repeats for keys outside of the explicit keymap range.\nMultiple keys can be mapped to the same scale degree.\nEnter \'x\' if you wish to leave the key disabled / not mapped."));
	microtunerLayout->addWidget(m_keymapTextEdit, 4, 2, 1, 2);

	// Mapping ranges
	auto keymapRangeLayout = new QGridLayout();
	microtunerLayout->addLayout(keymapRangeLayout, 5, 2, 1, 2, Qt::AlignCenter | Qt::AlignTop);

	auto firstKeySpin = new LcdSpinBox(3, nullptr, tr("First key"));
	firstKeySpin->setLabel(tr("FIRST"));
	firstKeySpin->setToolTip(tr("First MIDI key that will be mapped"));
	firstKeySpin->setModel(&m_firstKeyModel);
	keymapRangeLayout->addWidget(firstKeySpin, 0, 0);

	auto lastKeySpin = new LcdSpinBox(3, nullptr, tr("Last key"));
	lastKeySpin->setLabel(tr("LAST"));
	lastKeySpin->setToolTip(tr("Last MIDI key that will be mapped"));
	lastKeySpin->setModel(&m_lastKeyModel);
	keymapRangeLayout->addWidget(lastKeySpin, 0, 1);

	auto middleKeySpin = new LcdSpinBox(3, nullptr, tr("Middle key"));
	middleKeySpin->setLabel(tr("MIDDLE"));
	middleKeySpin->setToolTip(tr("First line in the keymap refers to this MIDI key"));
	middleKeySpin->setModel(&m_middleKeyModel);
	keymapRangeLayout->addWidget(middleKeySpin, 0, 2);

	auto baseKeySpin = new LcdSpinBox(3, nullptr, tr("Base key"));
	baseKeySpin->setLabel(tr("BASE N."));
	baseKeySpin->setToolTip(tr("Base note frequency will be assigned to this MIDI key"));
	baseKeySpin->setModel(&m_baseKeyModel);
	keymapRangeLayout->addWidget(baseKeySpin, 1, 0);

	auto baseFreqSpin = new LcdFloatSpinBox(4, 3, tr("Base note frequency"));
	baseFreqSpin->setLabel(tr("BASE NOTE FREQ"));
	baseFreqSpin->setModel(&m_baseFreqModel);
	baseFreqSpin->setToolTip(tr("Base note frequency"));
	keymapRangeLayout->addWidget(baseFreqSpin, 1, 1, 1, 2);

	auto applyKeymapButton = new QPushButton(tr("Apply keymap changes"));
	applyKeymapButton->setToolTip(tr("Verify and apply changes made to the selected key mapping. To use the mapping, select it in the settings of a supported instrument."));
	microtunerLayout->addWidget(applyKeymapButton, 6, 2, 1, 2);
	connect(applyKeymapButton, &QPushButton::clicked, this, &MicrotunerConfig::applyKeymap);

	updateScaleForm();
	updateKeymapForm();
	connect(Engine::getSong(), SIGNAL(scaleListChanged(int)), this, SLOT(updateScaleList(int)));
	connect(Engine::getSong(), SIGNAL(scaleListChanged(int)), this, SLOT(updateScaleForm()));
	connect(Engine::getSong(), SIGNAL(keymapListChanged(int)), this, SLOT(updateKeymapList(int)));
	connect(Engine::getSong(), SIGNAL(keymapListChanged(int)), this, SLOT(updateKeymapForm()));

	microtunerLayout->setRowStretch(4, 10);
	this->setLayout(microtunerLayout);

	// Add to the main window and setup fixed size etc.
	QMdiSubWindow *subWin = getGUI()->mainWindow()->addWindowedWidget(this);

	subWin->setAttribute(Qt::WA_DeleteOnClose, false);
	subWin->setMinimumWidth(300);
	subWin->setMinimumHeight(300);
	subWin->setMaximumWidth(500);
	subWin->setMaximumHeight(700);
	subWin->hide();

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);
}


/**
 * \brief Update list of available scales.
 * \param index Index of the scale to update; update all scales if -1 or out of range.
 */
void MicrotunerConfig::updateScaleList(int index)
{
	if (index >= 0 && static_cast<std::size_t>(index) < MaxScaleCount)
	{
		m_scaleComboModel.replaceItem(index,
			QString::number(index) + ": " + Engine::getSong()->getScale(index)->getDescription());
	}
	else
	{
		for (auto i = std::size_t{0}; i < MaxScaleCount; i++)
		{
			m_scaleComboModel.replaceItem(i,
				QString::number(i) + ": " + Engine::getSong()->getScale(i)->getDescription());
		}
	}
}


/**
 * \brief Update list of available keymaps.
 * \param index Index of the keymap to update; update all keymaps if -1 or out of range.
 */
void MicrotunerConfig::updateKeymapList(int index)
{
	if (index >= 0 && static_cast<std::size_t>(index) < MaxKeymapCount)
	{
		m_keymapComboModel.replaceItem(index,
			QString::number(index) + ": " + Engine::getSong()->getKeymap(index)->getDescription());
	}
	else
	{
		for (auto i = std::size_t{0}; i < MaxKeymapCount; i++)
		{
			m_keymapComboModel.replaceItem(i,
				QString::number(i) + ": " + Engine::getSong()->getKeymap(i)->getDescription());
		}
	}
}


/**
 * \brief Fill all the scale-related values based on currently selected scale
 */
void MicrotunerConfig::updateScaleForm()
{
	Song *song = Engine::getSong();
	if (song == nullptr) {return;}

	auto newScale = song->getScale(m_scaleComboModel.value());

	m_scaleNameEdit->setText(newScale->getDescription());

	// fill in the intervals
	m_scaleTextEdit->setPlainText("");
	const std::vector<Interval> &intervals = newScale->getIntervals();
	for (std::size_t i = 1; i < intervals.size(); i++)
	{
		m_scaleTextEdit->appendPlainText(intervals[i].getString());
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
	Song *song = Engine::getSong();
	if (song == nullptr) {return;}

	auto newMap = song->getKeymap(m_keymapComboModel.value());

	m_keymapNameEdit->setText(newMap->getDescription());

	m_keymapTextEdit->setPlainText("");
	const std::vector<int> &map = newMap->getMap();
	for (int value : map)
	{
		if (value >= 0) {m_keymapTextEdit->appendPlainText(QString::number(value));}
		else {m_keymapTextEdit->appendPlainText("x");}
	}
	QTextCursor tmp = m_keymapTextEdit->textCursor();
	tmp.movePosition(QTextCursor::Start);
	m_keymapTextEdit->setTextCursor(tmp);

	m_firstKeyModel.setValue(newMap->getFirstKey());
	m_lastKeyModel.setValue(newMap->getLastKey());
	m_middleKeyModel.setValue(newMap->getMiddleKey());
	m_baseKeyModel.setValue(newMap->getBaseKey());
	m_baseFreqModel.setValue(newMap->getBaseFreq());
}


/**
 * \brief Validate the scale name and entered interval definitions
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::validateScaleForm()
{
	auto fail = [this](const QString& message){ QMessageBox::critical(this, tr("Scale parsing error"), message); };

	// check name
	QString name = m_scaleNameEdit->text();
	if (name.length() > 0 && name[0] == '!') {fail(tr("Scale name cannot start with an exclamation mark")); return false;}
	if (name.contains('\n')) {fail(tr("Scale name cannot contain a new-line character")); return false;}

	// check intervals
	#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
		QStringList input = m_scaleTextEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
	#else
		QStringList input = m_scaleTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
	#endif
	for (auto &line: input)
	{
		if (line.isEmpty()) {continue;}
		if (line[0] == '!') {continue;}		// comment
		QString firstSection = line.section(QRegularExpression("\\s+|/"), 0, 0, QString::SectionSkipEmpty);
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
			num = firstSection.toInt(&ok);
			if (!ok) {fail(tr("Numerator of an interval defined as a ratio cannot be converted to a number")); return false;}
			if (line.contains('/'))
			{
				den = line.split('/').at(1).section(QRegularExpression("\\s+"), 0, 0, QString::SectionSkipEmpty).toInt(&ok);
			}
			if (!ok) {fail(tr("Denominator of an interval defined as a ratio cannot be converted to a number")); return false;}
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
	auto fail = [this](const QString& message){ QMessageBox::critical(this, tr("Keymap parsing error"), message); };

	// check name
	QString name = m_keymapNameEdit->text();
	if (name.length() > 0 && name[0] == '!') {fail(tr("Keymap name cannot start with an exclamation mark")); return false;}
	if (name.contains('\n')) {fail(tr("Keymap name cannot contain a new-line character")); return false;}

	// check key mappings
	#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
		QStringList input = m_keymapTextEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
	#else
		QStringList input = m_keymapTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
	#endif
	for (auto &line: input)
	{
		if (line.isEmpty()) {continue;}
		if (line[0] == '!') {continue;}			// comment
		QString firstSection = line.section(QRegularExpression("\\s+"), 0, 0, QString::SectionSkipEmpty);
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


/**
 * \brief Parse and apply the entered scale definition
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::applyScale()
{
	if (!validateScaleForm()) {return false;};

	std::vector<Interval> newIntervals;
	newIntervals.emplace_back(1, 1);

#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	QStringList input = m_scaleTextEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
#else
	QStringList input = m_scaleTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
#endif
	for (auto &line: input)
	{
		if (line.isEmpty()) {continue;}
		if (line[0] == '!') {continue;}		// comment
		QString firstSection = line.section(QRegularExpression("\\s+|/"), 0, 0, QString::SectionSkipEmpty);
		if (firstSection.contains('.'))		// cent mode
		{
			newIntervals.emplace_back(firstSection.toFloat());
		}
		else								// ratio mode
		{
			int num = 1, den = 1;
			num = firstSection.toInt();
			if (line.contains('/'))
			{
				den = line.split('/').at(1).section(QRegularExpression("\\s+"), 0, 0, QString::SectionSkipEmpty).toInt();
			}
			newIntervals.emplace_back(num, den);
		}
	}

	Song *song = Engine::getSong();
	if (song == nullptr) {return false;}

	auto newScale = std::make_shared<Scale>(m_scaleNameEdit->text(), std::move(newIntervals));
	song->setScale(m_scaleComboModel.value(), newScale);

	return true;
}


/**
 * \brief Parse and apply the entered keymap definition
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::applyKeymap()
{
	if (!validateKeymapForm()) {return false;}

	std::vector<int> newMap;

#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	QStringList input = m_keymapTextEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
#else
	QStringList input = m_keymapTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
#endif
	for (auto &line: input)
	{
		if (line.isEmpty()) {continue;}
		if (line[0] == '!') {continue;}			// comment
		QString firstSection = line.section(QRegularExpression("\\s+"), 0, 0, QString::SectionSkipEmpty);
		if (firstSection == "x")
		{
			newMap.push_back(-1);				// not mapped
			continue;
		}
		newMap.push_back(firstSection.toInt());
	}

	Song *song = Engine::getSong();
	if (song == nullptr) {return false;}

	auto newKeymap = std::make_shared<Keymap>(
		m_keymapNameEdit->text(),
		std::move(newMap),
		m_firstKeyModel.value(),
		m_lastKeyModel.value(),
		m_middleKeyModel.value(),
		m_baseKeyModel.value(),
		m_baseFreqModel.value()
	);
	song->setKeymap(m_keymapComboModel.value(), newKeymap);

	if (newKeymap->getDegree(newKeymap->getBaseKey()) == -1) {
		QMessageBox::warning(this, tr("Invalid keymap"), tr("Base key is not mapped to any scale degree. No sound will be produced as there is no way to assign reference frequency to any note."));}

	return true;
}


/**
 * \brief Parse an .scl file and apply the loaded scale if it is valid
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::loadScaleFromFile()
{
	QString fileName = FileDialog::getOpenFileName(this, tr("Open scale"), "", tr("Scala scale definition (*.scl)"));
	if (fileName == "") {return false;}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Scale load failure"), tr("Unable to open selected file."));
		return false;
	}
	QTextStream stream(&file);
	int i = -2, limit = 0;

	m_scaleNameEdit->setText("");
	m_scaleTextEdit->clear();
	while (!stream.atEnd() && i < limit)
	{
		QString line = stream.readLine();
		if (line != "" && line[0] == '!') {continue;}					// comment
		switch(i) {
			case -2:	m_scaleNameEdit->setText(line); break;			// first non-comment line = name or description
			case -1:	limit = line.toInt(); break;					// second non-comment line = degree count
			default:	m_scaleTextEdit->appendPlainText(line); break;	// all other lines = interval definitions
		}
		i++;
	}

	return applyScale();
}


/**
 * \brief Parse a .kbm file and apply the loaded keymap if it is valid
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::loadKeymapFromFile()
{
	QString fileName = FileDialog::getOpenFileName(this, tr("Open keymap"), "", tr("Scala keymap definition (*.kbm)"));
	if (fileName == "") {return false;}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Keymap load failure"), tr("Unable to open selected file."));
		return false;
	}
	QTextStream stream(&file);
	int i = -7, limit = 0;

	m_keymapNameEdit->setText(QFileInfo(fileName).baseName());		// .kbm does not store description, use file name
	m_keymapTextEdit->clear();

	while (!stream.atEnd() && i < limit)
	{
		QString line = stream.readLine();
		if (line != "" && line[0] == '!')
		{
			if (line.length() > 1 && line[1] == '!' && i == -7)		// LMMS extension: double "!" occurring before any
			{														// value is loaded marks a description field.
				m_keymapNameEdit->setText(line.mid(2));
			}
			continue;
		}
		switch(i) {
			case -7:	limit = line.toInt(); break;						// first non-comment line = keymap size
			case -6:	m_firstKeyModel.setValue(line.toInt()); break;		// second non-comment line = first key
			case -5:	m_lastKeyModel.setValue(line.toInt()); break;		// third non-comment line = last key
			case -4:	m_middleKeyModel.setValue(line.toInt()); break;		// fourth non-comment line = middle key
			case -3:	m_baseKeyModel.setValue(line.toInt()); break;		// fifth non-comment line = base key
			case -2:	m_baseFreqModel.setValue(line.toDouble()); break;	// sixth non-comment line = base freq
			case -1:	break;	// ignored									// seventh non-comment line = octave degree
			default:	m_keymapTextEdit->appendPlainText(line); break;		// all other lines = mapping definitions
		}
		i++;
	}

	return applyKeymap();
}


/**
 * \brief Save currently entered scale definition as .scl file
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::saveScaleToFile()
{
	if (!applyScale()) {return false;}
	QString fileName = FileDialog::getSaveFileName(this, tr("Save scale"), "", tr("Scala scale definition (*.scl)"));
	if (fileName == "") {return false;}
	if (QFileInfo(fileName).suffix() != "scl") {fileName = fileName + ".scl";}
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this, tr("Scale save failure"), tr("Unable to open selected file for writing."));
		return false;
	}
	Song *song = Engine::getSong();
	if (song == nullptr) {return false;}

	QTextStream stream(&file);
	stream << "! " << QFileInfo(fileName).fileName() << "\n";
	stream << "! Exported from LMMS " LMMS_VERSION "\n";
	stream << "!\n";
	stream << "! Scale description:\n";
	stream << m_scaleNameEdit->text() << "\n";
	stream << "!\n";
	stream << "! Number of degrees:\n";
	stream << song->getScale(m_scaleComboModel.value())->getIntervals().size() - 1 << "\n";
	stream << "!\n";
	stream << "! Intervals:\n";
	stream << m_scaleTextEdit->toPlainText() << "\n";

	return true;
}


/**
 * \brief Save currently entered keymap definition as .kbm file
 * \return true if input is valid, false if problems were detected
 */
bool MicrotunerConfig::saveKeymapToFile()
{
	if (!applyKeymap()) {return false;}
	QString fileName = FileDialog::getSaveFileName(this, tr("Save keymap"), "", tr("Scala keymap definition (*.kbm)"));
	if (fileName == "") {return false;}
	if (QFileInfo(fileName).suffix() != "kbm") {fileName = fileName + ".kbm";}
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this, tr("Keymap save failure"), tr("Unable to open selected file for writing."));
		return false;
	}
	Song *song = Engine::getSong();
	if (song == nullptr) {return false;}

	QTextStream stream(&file);
	stream << "! " << QFileInfo(fileName).fileName() << "\n";
	stream << "! Exported from LMMS " LMMS_VERSION "\n";
	stream << "!\n";
	stream << "! Keymap description:\n";
	stream << "!!" << m_keymapNameEdit->text() << "\n";
	stream << "!\n";
	stream << "! Keymap size:\n";
	stream << song->getKeymap(m_keymapComboModel.value())->getMap().size() << "\n";
	stream << "!\n";
	stream << "! First key:\n";
	stream << m_firstKeyModel.value() << "\n";
	stream << "! Last key:\n";
	stream << m_lastKeyModel.value() << "\n";
	stream << "! Middle key:\n";
	stream << m_middleKeyModel.value() << "\n";
	stream << "! Base key:\n";
	stream << m_baseKeyModel.value() << "\n";
	stream << "! Base frequency:\n";
	stream << m_baseFreqModel.value() << "\n";
	stream << "! Octave degree (always using the last scale degree):\n";
	stream << "0\n";
	stream << "!\n";
	stream << "! Key mappings:\n";
	stream << m_keymapTextEdit->toPlainText() << "\n";

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
	if (parentWidget()) {parentWidget()->hide();}
	else {hide();}
	ce->ignore();
}


} // namespace lmms::gui
