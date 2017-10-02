/*
 * SongMetaDataDialog.cpp - dialog for setting song properties/tags/metadata
 *
 * Copyright (c) 2017
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

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

#include "lmmsversion.h"
#include "SongMetaDataDialog.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "OutputSettings.h"


SongMetaDataDialog::SongMetaDataDialog(QWidget * _parent) :
	QDialog( _parent ),
	Ui::SongMetaDataDialog()
{
	setupUi( this );
	setWindowTitle( tr( "Song Properties" ) );

	// tab 1
	Song * song=Engine::getSong();
	value_Tempo->setText(QString("%1 bpm").arg(song->getTempo()));
	value_TimeSignature->setText(QString("%1/%2")
				     .arg(song->getTimeSigModel().getNumerator())
				     .arg(song->getTimeSigModel().getDenominator()));
	value_Software->setText(QString("LMMS %1").arg(LMMS_VERSION));
	value_Structure->setText(song->songMetaData("Structure"));

	// tab 2
	value_SongTitle->setText(song->songMetaData("SongTitle"));
	value_Artist->setText(song->songMetaData("Artist"));
	value_AlbumTitle->setText(song->songMetaData("AlbumTitle"));
	value_TrackNumber->setText(song->songMetaData("TrackNumber"));
	value_ReleaseDate->setDate(QDate::fromString(song->songMetaData("ReleaseDate"),Qt::ISODate));
	value_IRCS->setText(song->songMetaData("IRCS"));
	value_Copyright->setText(song->songMetaData("Copyright"));
	value_License->lineEdit()->setText(song->songMetaData("License"));
	value_Genre->lineEdit()->setText(song->songMetaData("Genre"));
	value_Subgenre->setText(song->songMetaData("Subgenre"));

	// tab 3
	value_ArtistWebsite->setText(song->songMetaData("ArtistWebsite"));
	value_LabelWebsite->setText(song->songMetaData("LabelWebsite"));
	value_Amazon->setText(song->songMetaData("Amazon"));
	value_BandCamp->setText(song->songMetaData("BandCamp"));
	value_Clyp->setText(song->songMetaData("Clyp"));
	value_iTunes->setText(song->songMetaData("iTunes"));
	value_LMMS->setText(song->songMetaData("LMMS"));
	value_Orfium->setText(song->songMetaData("Orfium"));
	value_SoundCloud->setText(song->songMetaData("SoundCloud"));
	value_Spotify->setText(song->songMetaData("Spotify"));
	value_YouTube->setText(song->songMetaData("YouTube"));
}




SongMetaDataDialog::~SongMetaDataDialog()
{
}




void SongMetaDataDialog::reject()
{
	QDialog::reject();
}



void SongMetaDataDialog::accept()
{
	// tab 1
	Song * song=Engine::getSong();
	song->setSongMetaData("Structure",value_Structure->text());

	// tab 2
	song->setSongMetaData("SongTitle",value_SongTitle->text());
	song->setSongMetaData("Artist",value_Artist->text());
	song->setSongMetaData("AlbumTitle",value_AlbumTitle->text());
	song->setSongMetaData("TrackNumber",value_TrackNumber->text());
	song->setSongMetaData("ReleaseDate",value_ReleaseDate->date().toString(Qt::ISODate));
	song->setSongMetaData("IRCS",value_IRCS->text());
	song->setSongMetaData("Copyright",value_Copyright->text());
	song->setSongMetaData("License",value_License->currentText());
	song->setSongMetaData("Genre",value_Genre->currentText());
	song->setSongMetaData("Subgenre",value_Subgenre->text());

	// tab 3
	song->setSongMetaData("ArtistWebsite",value_ArtistWebsite->text());
	song->setSongMetaData("LabelWebsite",value_LabelWebsite->text());
	song->setSongMetaData("Amazon",value_Amazon->text());
	song->setSongMetaData("BandCamp",value_BandCamp->text());
	song->setSongMetaData("Clyp",value_Clyp->text());
	song->setSongMetaData("iTunes",value_iTunes->text());
	song->setSongMetaData("LMMS",value_LMMS->text());
	song->setSongMetaData("Orfium",value_Orfium->text());
	song->setSongMetaData("SoundCloud",value_SoundCloud->text());
	song->setSongMetaData("Spotify",value_Spotify->text());
	song->setSongMetaData("YouTube",value_YouTube->text());


	QDialog::accept();
}




void SongMetaDataDialog::closeEvent( QCloseEvent * _ce )
{
	QDialog::closeEvent( _ce );
}
