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

#include "RenderManager.h"

class QString;
class QLabel;
class QProgressBar;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QFormLayout;
class QGroupBox;

namespace lmms::gui {

class ExportProjectDialog : public QDialog
{
public:
	enum class Mode
	{
		ExportProject,
		ExportTracks,
		ExportTrack
	};

	ExportProjectDialog(const QString& path, Mode mode, QWidget* parent = nullptr);

	//! Assign a single track to export.
	//! The track is only used when the mode is set to `ExportTrack`.
	void setTrack(Track* track) { m_track = track; }

private:
	void accept() override;
	void reject() override;
	void onFileFormatChanged(int index);
	void onStartButtonClicked();
	void updateTitleBar(int prog);

	QLabel* m_fileFormatLabel = nullptr;
	QComboBox* m_fileFormatComboBox = nullptr;

	QLabel* m_sampleRateLabel = nullptr;
	QComboBox* m_sampleRateComboBox = nullptr;

	QLabel* m_bitRateLabel = nullptr;
	QComboBox* m_bitRateComboBox = nullptr;

	QLabel* m_bitDepthLabel = nullptr;
	QComboBox* m_bitDepthComboBox = nullptr;

	QLabel* m_stereoModeLabel = nullptr;
	QComboBox* m_stereoModeComboBox = nullptr;

	QLabel* m_compressionLevelLabel = nullptr;
	QComboBox* m_compressionLevelComboBox = nullptr;

	QGroupBox* m_fileFormatSettingsGroupBox = nullptr;
	QFormLayout* m_fileFormatSettingsLayout = nullptr;

	QCheckBox* m_exportAsLoopBox = nullptr;
	QCheckBox* m_exportBetweenLoopMarkersBox = nullptr;
	QCheckBox* m_importExportedTrackBox = nullptr;
	QLabel* m_loopRepeatLabel = nullptr;
	QSpinBox* m_loopRepeatBox = nullptr;
	QPushButton* m_startButton = nullptr;
	QPushButton* m_cancelButton = nullptr;
	QProgressBar* m_progressBar = nullptr;

	QString m_path;
	Mode m_mode;
	Track* m_track = nullptr;
	std::unique_ptr<RenderManager> m_renderManager;
};

} // namespace lmms::gui

#endif // LMMS_GUI_EXPORT_PROJECT_DIALOG_H
