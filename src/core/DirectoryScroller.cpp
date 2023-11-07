/*
 * DirectoryScroller.cpp - Util to flip through all the sound files in a directory
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


#include "PathUtil.h"
#include "DirectoryScroller.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "lmmsversion.h"

namespace lmms
{

DirectoryScroller::DirectoryScroller( const QString& path )
{
	if ( !path.isEmpty() )
	{
		findIndex(path);
	}
	else disable();
}

DirectoryScroller::DirectoryScroller(  )
{
	disable();
}

QString DirectoryScroller::next()
{
	return findNext(true);
}

QString DirectoryScroller::prev()
{
	return findNext(false);
}

void DirectoryScroller::disable()
{
	m_index = -1;
}

void DirectoryScroller::setFile( const QString& path )
{
	if ( !path.isEmpty() )
	{
		findIndex(path);
	}
	else disable();
}

void DirectoryScroller::findIndex(const QString& path)
{
	QFileInfo fInfo(PathUtil::toAbsolute(path));
	if ( ! fInfo.exists() )
	{
		disable();
		return;
	}

	m_dir = fInfo.dir();
	m_suffix = fInfo.suffix();
	QString fileName = fInfo.fileName();
	QStringList files = m_dir.entryList(QStringList("*." + m_suffix), QDir::Files, QDir::Name);
	for( int i = 0 ; i < files.length() ; i++ )
	{
		if ( files.at(i) == fileName )
		{
			m_index = i;
			return;
		}
	}
	// should not be possible to get here since path exists
	disable();
}

/**
 * @return filename (no path) of the next file to be used from the same directory or an empty string
 */
QString DirectoryScroller::findNext(bool upDown)
{
	if (m_index < 0)
	{
		return QString();
	}

	QStringList files = m_dir.entryList(QStringList("*." + m_suffix), QDir::Files, QDir::Name);
	if (upDown)
	{
		if (m_index < files.length() - 1)
		{
			return files.at(++m_index);
		}
	}
	else if (m_index > 0)
	{
		return files.at(--m_index);
	}

	return QString();
}

} // namespace lmms
