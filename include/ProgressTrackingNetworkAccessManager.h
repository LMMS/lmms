/*
 * ProgressTrackingNetworkAccessManager.h - header file for
 *                       ProgressTrackingNetworkAccessManager class
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of RuckTrack - http://rucktrack.sourceforge.net
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

#ifndef _PROGRESS_TRACKING_NETWORK_ACCESS_MANAGER_H
#define _PROGRESS_TRACKING_NETWORK_ACCESS_MANAGER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>


class ProgressTrackingNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    ProgressTrackingNetworkAccessManager( QObject * _parent ) :
        QNetworkAccessManager( _parent )
    {
    }

    virtual QNetworkReply * createRequest( Operation op,
                                            const QNetworkRequest & req,
                                            QIODevice * outgoingData = 0 )
    {
        QNetworkReply * reply =
            QNetworkAccessManager::createRequest( op, req, outgoingData );
        if( op == GetOperation )
        {
            connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
                        this, SLOT( updateProgress( qint64, qint64 ) ) );
        }
        return reply;
    }


public slots:
    void updateProgress(qint64 done, qint64 total)
    {
        emit progressChanged( qBound<qint64>( 0, done * 100 /
                                        qMax<qint64>( 1, total ), 100 ) );
    }


signals:
    void progressChanged( int );

} ;


#endif // _PROGRESS_TRACKING_NETWORK_ACCESS_MANAGER_H
