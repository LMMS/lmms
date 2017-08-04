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

	Song * song=Engine::getSong();
	value_Tempo->setText(QString("%1 bpm").arg(song->getTempo()));
	value_TimeSignature->setText(QString("%1/%2")
				     .arg(song->getTimeSigModel().getNumerator())
				     .arg(song->getTimeSigModel().getDenominator()));
	value_Software->setText(QString("LMMS %1").arg(LMMS_VERSION));
	value_Structure->setText(song->songMetaData("Structure"));
	value_SongTitle->setText(song->songMetaData("SongTitle"));
	value_AlbumTitle->setText(song->songMetaData("AlbumTitle"));
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
	Song * song=Engine::getSong();
	song->setSongMetaData("Structure",value_Structure->text());
	song->setSongMetaData("SongTitle",value_SongTitle->text());
	song->setSongMetaData("AlbumTitle",value_AlbumTitle->text());
	song->setSongMetaData("YouTube",value_YouTube->text());
	QDialog::accept();
}




void SongMetaDataDialog::closeEvent( QCloseEvent * _ce )
{
	QDialog::closeEvent( _ce );
}
