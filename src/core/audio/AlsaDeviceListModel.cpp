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

#include <QtCore/QObject>

#include "AlsaDeviceListModel.h"

#ifdef LMMS_HAVE_ALSA

AlsaDeviceListModel::AlsaDeviceListModel(
		snd_rawmidi_stream_t _stream,
		QObject * _parent ) :
		QAbstractListModel( _parent )
{
	init( "rawmidi", SND_RAWMIDI_STREAM_INPUT ?
			"Input" :
			"Output" );
}



AlsaDeviceListModel::AlsaDeviceListModel(
		snd_pcm_stream_t _stream,
		QObject * _parent ) :
		QAbstractListModel( _parent )
{
	init( "pcm", SND_PCM_STREAM_CAPTURE ?
			"Input" :
			"Output" );
}


AlsaDeviceListModel::AlsaDeviceListModel(
		QObject * _parent ) :
		QAbstractListModel( _parent )
{
	init( "seq", NULL);
}


void AlsaDeviceListModel::init(
		const char * _iface,
		const char * _filter)
{
	void **hints, **n;
	char *name, *descr, *io;

	if (snd_device_name_hint(-1, _iface, &hints) < 0)
		return;
	
	n = hints;
	while (*n != NULL) {
		name = snd_device_name_get_hint(*n, "NAME");
		descr = snd_device_name_get_hint(*n, "DESC");
		io = snd_device_name_get_hint(*n, "IOID");
		
		// Filter out non-null or filtered items
		if (io == NULL || _filter == NULL || strcmp(io, _filter) == 0)
			m_devices.append(StringPair(name, descr));

		if (name != NULL)
			free(name);
		if (descr != NULL)
			free(descr);
		if (io != NULL)
			free(io);
		n++;
	}
	snd_device_name_free_hint(hints);
}



int AlsaDeviceListModel::rowCount(
		const QModelIndex & parent ) const
{
	return m_devices.count();
}



QVariant AlsaDeviceListModel::data(
		const QModelIndex & index,
		int role ) const
{
	switch( role )
	{
	case Qt::DisplayRole:
		return m_devices.at( index.row() ).first;
	case Qt::ToolTipRole:
	case Qt::StatusTipRole:
		return m_devices.at( index.row() ).second;
	default:
		return QVariant();
	};
}


#endif // LMMS_HAVE_ALSA
