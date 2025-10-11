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
#include <memory>
#include "ui_export_project.h"

#include "ProjectRenderer.h"
#include "RenderManager.h"

namespace lmms::gui
{


class ExportProjectDialog : public QDialog, public Ui::ExportProjectDialog
{
	Q_OBJECT
public:
	enum class ExportMode
	{
		Project, // export the project
		Track, // export a track
		MultipleTrack // export multiple track
	};

	ExportProjectDialog(const QString & _file_name, QWidget * _parent, bool multi_export);
	ExportProjectDialog(const QString & _file_name, QWidget * _parent, Track& trackToExport);

protected:
	void reject() override;
	void closeEvent( QCloseEvent * _ce ) override;


private slots:
	void startBtnClicked();
	void updateTitleBar( int );
	void accept() override;
	void startExport();

	void onFileFormatChanged(int);

private:
	ExportProjectDialog(const QString & _file_name, QWidget * _parent, ExportMode exportMode, Track* trackToExport);

	QString m_fileName;
	QString m_dirName;
	QString m_fileExtension;
	ExportMode m_exportMode;
	Track* m_trackToExport;

	ProjectRenderer::ExportFileFormat m_ft;
	std::unique_ptr<RenderManager> m_renderManager;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_EXPORT_PROJECT_DIALOG_H
