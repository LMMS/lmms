/*
 * ExportProjectDialog.h - declaration of class ExportProjectDialog which is
 *                           responsible for exporting project
 *
 * Copyright (c) 2004-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_EXPORT_PROJECT_DIALOG_H
#define LMMS_GUI_EXPORT_PROJECT_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>

namespace lmms::gui
{

class ExportProjectDialog : public QDialog
{
public:
	ExportProjectDialog(const QString& path, QWidget* parent, bool multiExport = false);

private:
	class FileFormatSetting : public QWidget
	{
	public:
		FileFormatSetting(const QString& header);
		void addItem(const QString& text, const QVariant& userData);
	private:
		QLabel* m_label = nullptr;
		QComboBox* m_comboBox = nullptr;
	};

	FileFormatSetting* m_fileFormatSetting = nullptr;
	FileFormatSetting* m_sampleRateSetting = nullptr;
	FileFormatSetting* m_bitRateSetting = nullptr;
	FileFormatSetting* m_bitDepthSetting = nullptr;
	FileFormatSetting* m_stereoModeSetting = nullptr;
	FileFormatSetting* m_compressionLevelSetting = nullptr;
	QPushButton* m_startButton = nullptr;
	QPushButton* m_cancelButton = nullptr;
	QProgressBar* m_progressBar = nullptr;
};

} // namespace lmms::gui

#endif // LMMS_GUI_EXPORT_PROJECT_DIALOG_H
