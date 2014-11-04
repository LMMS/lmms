/*
 * VersionedSaveDialog.cpp - implementation of class VersionedSaveDialog
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QFontMetrics>
#include <QtGui/QLineEdit>

#include "VersionedSaveDialog.h"




VersionedSaveDialog::VersionedSaveDialog( QWidget *parent,
										  const QString &caption,
										  const QString &directory,
										  const QString &filter ) :
	FileDialog(parent, caption, directory, filter)
{
	setAcceptMode( QFileDialog::AcceptSave );
	setFileMode( QFileDialog::AnyFile );

	// Create + and - buttons
	QPushButton *plusButton( new QPushButton( "+", this) );
	plusButton->setToolTip( tr( "Increment version number" ) );
	QPushButton *minusButton( new QPushButton( "-", this ) );
	minusButton->setToolTip( tr( "Decrement version number" ) );
	plusButton->setFixedWidth( plusButton->fontMetrics().width( "+" ) + 30 );
	minusButton->setFixedWidth( minusButton->fontMetrics().width( "+" ) + 30 );

	// Add buttons to grid layout. For doing this, remove the lineEdit and
	// replace it with a HBox containing lineEdit and the buttons.
	QGridLayout *layout = dynamic_cast<QGridLayout*>( this->layout() );
	QWidget *lineEdit = findChild<QLineEdit*>();
	layout->removeWidget( lineEdit );

	QHBoxLayout* hLayout( new QHBoxLayout() );
	hLayout->addWidget( lineEdit );
	hLayout->addWidget( plusButton );
	hLayout->addWidget( minusButton );
	layout->addLayout( hLayout, 2, 1 );

	// Connect + and - buttons
	connect( plusButton, SIGNAL( clicked() ), this, SLOT( incrementVersion() ));
	connect( minusButton, SIGNAL( clicked() ), this, SLOT( decrementVersion() ));
}




bool VersionedSaveDialog::changeFileNameVersion(QString &fileName, bool increment )
{
	static QRegExp regexp( "-\\d+(\\.\\w+)?$" );

	int idx = regexp.indexIn( fileName );
	// For file names without extension (no ".mmpz")
	int insertIndex = fileName.lastIndexOf( '.' );
	if ( insertIndex < idx+1 )
		insertIndex = fileName.size();

	if ( idx == -1 )
	{
		// Can't decrement if there is no version number
		if ( increment == false )
			return false;
		else
			fileName.insert( insertIndex, "-01" );
	}
	else
	{
		// Find current version number
		QString number = fileName.mid( idx+1, insertIndex - idx - 1 );
		bool ok;
		ushort version = number.toUShort( &ok );
		Q_ASSERT( ok );

		// Can't decrement 0
		if ( !increment and version == 0 )
			return false;
		// Replace version number
		version = increment ? version + 1 : version - 1;
		QString newnumber = QString( "%1" ).arg( version, 2, 10, QChar( '0' ) );

		fileName.replace( idx+1, number.length(), newnumber );
	}
	return true;
}




void VersionedSaveDialog::incrementVersion()
{
	const QStringList& selected = selectedFiles();
	if ( selected.size() != 1 )
		return;
	QString file = selected[0];
	changeFileNameVersion( file, true );
	clearSelection();
	selectFile( file );
}




void VersionedSaveDialog::decrementVersion()
{
	const QStringList& selected = selectedFiles();
	if ( selected.size() != 1 )
		return;
	QString file = selected[0];
	changeFileNameVersion( file, false );
	clearSelection();
	selectFile( file );
}

#include "moc_VersionedSaveDialog.cxx"
