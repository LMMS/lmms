/*
 * DirectoryScroller.h - class for selecting sounds in a given directory
 * This enalbes, for example, quickly trying out sounds in audio file processor
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

#ifndef LMMS_DIRECTORY_SCROLLER_H
#define LMMS_DIRECTORY_SCROLLER_H

#include <QString>
#include <QDir>

namespace lmms
{


class DirectoryScroller
{

public:

    DirectoryScroller(  );
    DirectoryScroller( const QString& path );

    virtual ~DirectoryScroller() = default;

    void setFile( const QString& path );
    void disable();
	QString next( );
	QString prev( );

private:
    int m_index; // -1 = disabled
    QString m_suffix;
    QDir m_dir;

    void findIndex(const QString& fileName);
    QString findNext(bool upDown);
} ;


} // namespace lmms

#endif // LMMS_DIRECTORY_SCROLLER_H
