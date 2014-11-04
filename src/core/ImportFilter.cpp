/*
 * ImportFilter.cpp - base-class for all import-filters (MIDI, FLP etc)
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QMessageBox>

#include "ImportFilter.h"
#include "engine.h"
#include "TrackContainer.h"
#include "ProjectJournal.h"



ImportFilter::ImportFilter( const QString & _file_name,
							const Descriptor * _descriptor ) :
	Plugin( _descriptor, NULL ),
	m_file( _file_name )
{
}




ImportFilter::~ImportFilter()
{
}




void ImportFilter::import( const QString & _file_to_import,
							TrackContainer* tc )
{
	DescriptorList d;
	Plugin::getDescriptorsOfAvailPlugins( d );

	bool successful = false;

	char * s = qstrdup( _file_to_import.toUtf8().constData() );

	// do not record changes while importing files
	const bool j = engine::projectJournal()->isJournalling();
	engine::projectJournal()->setJournalling( false );

	for( Plugin::DescriptorList::ConstIterator it = d.begin();
												it != d.end(); ++it )
	{
		if( it->type == Plugin::ImportFilter )
		{
			Plugin * p = Plugin::instantiate( it->name, NULL, s );
			if( dynamic_cast<ImportFilter *>( p ) != NULL &&
				dynamic_cast<ImportFilter *>( p )->tryImport( tc ) == true )
			{
				delete p;
				successful = true;
				break;
			}
			delete p;
		}
	}

	engine::projectJournal()->setJournalling( j );

	delete[] s;

	if( successful == false )
	{
		QMessageBox::information( NULL,
			TrackContainer::tr( "Couldn't import file" ),
			TrackContainer::tr( "Couldn't find a filter for "
						"importing file %1.\n"
						"You should convert this file "
						"into a format supported by "
						"LMMS using another software."
						).arg( _file_to_import ),
					QMessageBox::Ok,
					QMessageBox::NoButton );
	}
}




bool ImportFilter::openFile()
{
	if( m_file.open( QFile::ReadOnly ) == false )
	{
		QMessageBox::critical( NULL,
			TrackContainer::tr( "Couldn't open file" ),
			TrackContainer::tr( "Couldn't open file %1 "
						"for reading.\nPlease make "
						"sure you have read-"
						"permission to the file and "
						"the directory containing the "
						"file and try again!" ).arg(
							m_file.fileName() ),
					QMessageBox::Ok,
					QMessageBox::NoButton );
		return false;
	}
	return true;
}



