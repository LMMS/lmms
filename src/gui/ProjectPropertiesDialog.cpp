/*
 * ProjectPropertiesDialog.cpp - Configuration widget for project-specific settings
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ProjectPropertiesDialog.h"

#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>

#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectProperties.h"
#include "TabBar.h"
#include "TabButton.h"
#include "Song.h"

namespace lmms::gui
{


inline void labelWidget(QWidget * w, const QString & txt)
{
	auto title = new QLabel(txt, w);
	QFont f = title->font();
	f.setBold(true);
	title->setFont(f);

	QBoxLayout * boxLayout = dynamic_cast<QBoxLayout *>(w->layout());
	assert(boxLayout);

	boxLayout->addWidget(title);
}



ProjectPropertiesDialog::ProjectPropertiesDialog():
	m_metaTitle(ProjectProperties::inst()->value("meta","title", QFileInfo(Engine::getSong()->projectFileName()).baseName())),
	m_metaArtist(ProjectProperties::inst()->value("meta","artist", "")),
	m_metaAlbum(ProjectProperties::inst()->value("meta","album", "")),
	m_metaYear(ProjectProperties::inst()->value("meta","year", "1970")),
	m_metaComment(ProjectProperties::inst()->value("meta","comment", "")),
	m_metaGenre(ProjectProperties::inst()->value("meta","genre", "")),
	m_metaSoftware(ProjectProperties::inst()->value("meta","software", "LMMS"))
{
	setWindowIcon(embed::getIconPixmap("setup_general"));
	setWindowTitle(tr("Project Properties"));
	setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	setModal(true);


	// Main widget.
	auto main_w = new QWidget(this);

	// Vertical layout.
	auto vlayout = new QVBoxLayout(this);
	vlayout->setSpacing(0);
	vlayout->setContentsMargins(0, 0, 0, 0);

	// Horizontal layout.
	auto hlayout = new QHBoxLayout(main_w);
	hlayout->setSpacing(0);
	hlayout->setContentsMargins(0, 0, 0, 0);

	// Tab bar for the main tabs.
	m_tabBar = new TabBar(main_w, QBoxLayout::TopToBottom);
	m_tabBar->setExclusive(true);
	m_tabBar->setFixedWidth(72);

	// Settings widget.
	auto settings_w = new QWidget(main_w);

	QVBoxLayout * settingsLayout = new QVBoxLayout(settings_w);

	// General widget.
	auto general_w = new QWidget(settings_w);
	auto general_layout = new QVBoxLayout(general_w);
	general_layout->setSpacing(10);
	general_layout->setContentsMargins(0, 0, 0, 0);
	labelWidget(general_w, tr("General"));

	// General scroll area.
	auto generalScroll = new QScrollArea(general_w);
	generalScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	generalScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// General controls widget.
	auto generalControls = new QWidget(general_w);

	// Path selectors layout.
	auto generalControlsLayout = new QVBoxLayout;
	generalControlsLayout->setSpacing(10);
	generalControlsLayout->setContentsMargins(0, 0, 0, 0);


	auto metaDataGroupBox = new QGroupBox(tr("Metadata"), generalControls);

	auto metaDataVBox = new QVBoxLayout(metaDataGroupBox);
	metaDataVBox->setSpacing(5);
	metaDataVBox->setContentsMargins(0, 0, 0, 0);

	auto addMetaDataLine = [metaDataVBox, metaDataGroupBox](QString name, QLineEdit* lineEdit, QString* value)
	{
		auto line_w = new QWidget(metaDataGroupBox);
		auto lineLayout = new QVBoxLayout(line_w);
		lineLayout->setSpacing(2);
		lineLayout->setContentsMargins(0, 0, 0, 0);
		lineLayout->addWidget(new QLabel{name, line_w});
		lineEdit = new QLineEdit(*value, line_w);
		connect(lineEdit, &QLineEdit::textChanged, [value](const QString& text){ *value = text; });
		lineLayout->addWidget(lineEdit);
		metaDataVBox->addWidget(line_w);
	};

	addMetaDataLine(tr("Title"), m_metaTitleLineEdit, &m_metaTitle);
	addMetaDataLine(tr("Artist"), m_metaArtistLineEdit, &m_metaArtist);
	addMetaDataLine(tr("Album"), m_metaAlbumLineEdit, &m_metaAlbum);
	addMetaDataLine(tr("Year"), m_metaYearLineEdit, &m_metaYear);
	addMetaDataLine(tr("Comment"), m_metaCommentLineEdit, &m_metaComment);
	addMetaDataLine(tr("Genre"), m_metaGenreLineEdit, &m_metaGenre);
	addMetaDataLine(tr("Software"), m_metaSoftwareLineEdit, &m_metaSoftware);

	generalControlsLayout->addWidget(metaDataGroupBox);
	generalControlsLayout->addSpacing(10);



	// General layout ordering.
	generalControlsLayout->addStretch();
	generalControls->setLayout(generalControlsLayout);
	generalScroll->setWidget(generalControls);
	generalScroll->setWidgetResizable(true);
	general_layout->addWidget(generalScroll, 1);



	// Add all main widgets to the layout of the settings widget
	// This is needed so that we automatically get the correct sizes.
	settingsLayout->addWidget(general_w);

	// Major tabs ordering.
	m_tabBar->addTab(general_w,
			tr("General"), 0, false, true, false)->setIcon(
					embed::getIconPixmap("setup_general"));

	m_tabBar->setActiveTab(0);

	// Horizontal layout ordering.
	hlayout->addSpacing(2);
	hlayout->addWidget(m_tabBar);
	hlayout->addSpacing(10);
	hlayout->addWidget(settings_w);
	hlayout->addSpacing(10);

	// Extras widget and layout.
	auto extras_w = new QWidget(this);
	auto extras_layout = new QHBoxLayout(extras_w);
	extras_layout->setSpacing(0);
	extras_layout->setContentsMargins(0, 0, 0, 0);

	// OK button.
	auto ok_btn = new QPushButton(embed::getIconPixmap("apply"), tr("OK"), extras_w);
	connect(ok_btn, SIGNAL(clicked()),
			this, SLOT(accept()));

	// Cancel button.
	auto cancel_btn = new QPushButton(embed::getIconPixmap("cancel"), tr("Cancel"), extras_w);
	connect(cancel_btn, SIGNAL(clicked()),
			this, SLOT(reject()));

	// Extras layout ordering.
	extras_layout->addStretch();
	extras_layout->addWidget(ok_btn);
	extras_layout->addSpacing(10);
	extras_layout->addWidget(cancel_btn);
	extras_layout->addSpacing(10);

	// Vertical layout ordering.
	vlayout->addWidget(main_w, 1);
	vlayout->addSpacing(10);
	vlayout->addWidget(extras_w);
	vlayout->addSpacing(10);

	// Ensure that we cannot make the dialog smaller than it wants to be
	setMinimumWidth(width());

	show();
}


void ProjectPropertiesDialog::accept()
{
	QDialog::accept();

	ProjectProperties::inst()->setValue("meta", "title", m_metaTitle);
	ProjectProperties::inst()->setValue("meta", "artist", m_metaArtist);
	ProjectProperties::inst()->setValue("meta", "album", m_metaAlbum);
	ProjectProperties::inst()->setValue("meta", "year", m_metaYear);
	ProjectProperties::inst()->setValue("meta", "comment", m_metaComment);
	ProjectProperties::inst()->setValue("meta", "genre", m_metaGenre);
	ProjectProperties::inst()->setValue("meta", "software", m_metaSoftware);
}


} // namespace lmms::gui
