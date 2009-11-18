/*
 * AlsaDeviceListModel - allows quick access to a list of alsa devices
 *
 * Copyright (c) 2009 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _ALSA_DEVICE_LIST_MODEL
#define _ALSA_DEVICE_LIST_MODEL

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

#include <alsa/asoundlib.h>
#include <QAbstractListModel>
#include <QPair>


class AlsaDeviceListModel : public QAbstractListModel
{
public:
	AlsaDeviceListModel(
		snd_rawmidi_stream_t _stream, QObject * _parent );

	AlsaDeviceListModel(
		snd_pcm_stream_t _stream, QObject * _parent );

	AlsaDeviceListModel( QObject * _parent );

protected:
	void init( const char * _iface, const char * _filter );

	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index,
				   int role = Qt::DisplayRole ) const;
private:
	typedef QPair<QString,QString> StringPair;
	typedef QList<StringPair> DeviceList;
	DeviceList m_devices;
};


#endif // LMMS_HAVE_ALSA

#endif
