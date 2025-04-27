/*
 * ProjectPropertiesDialog.h - Configuration widget for project-specific settings
 *
 * Copyright (c) 2025 regulus79
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

#ifndef LMMS_GUI_PROJECT_PROPERTIES_DIALOG_H
#define LMMS_GUI_PROJECT_PROPERTIES_DIALOG_H

#include <QDialog>


class QLineEdit;

namespace lmms::gui
{

class TabBar;

class ProjectPropertiesDialog : public QDialog
{
	Q_OBJECT
public:
	ProjectPropertiesDialog();

protected slots:
	void accept() override;

private:
	TabBar * m_tabBar;

	QString m_metaTitle;
	QString m_metaArtist;
	QString m_metaAlbum;
	QString m_metaYear;
	QString m_metaComment;
	QString m_metaGenre;
	QString m_metaSoftware;

	QLineEdit * m_metaTitleLineEdit;
	QLineEdit * m_metaArtistLineEdit;
	QLineEdit * m_metaAlbumLineEdit;
	QLineEdit * m_metaYearLineEdit;
	QLineEdit * m_metaCommentLineEdit;
	QLineEdit * m_metaGenreLineEdit;
	QLineEdit * m_metaSoftwareLineEdit;

};


} // namespace lmms::gui

#endif // LMMS_GUI_PROJECT_PROPERTIES_DIALOG_H
