/*
 * SpaControlBase.h - implementation of SPA interface
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef SPA_CONTROL_BASE_H
#define SPA_CONTROL_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include <QMap>
#include <QMutex>
#include <QString>
#include <memory>
#include <vector>

// general LMMS includes
#include "DataFile.h"
#include "Plugin.h"

// includes from the spa library
#include <spa/audio_fwd.h>
#include <spa/spa_fwd.h>

class SpaControlBase
{
public:
	SpaControlBase(const QString &uniqueName);
	virtual ~SpaControlBase();

	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);

	void writeOsc(const char *dest, const char *args, va_list va);
	void writeOsc(const char *dest, const char *args, ...);

	void loadFile(const QString &file);

	const spa::descriptor *m_spaDescriptor = nullptr;
	spa::plugin *m_plugin = nullptr;

	QMap<QString, AutomatableModel *> m_connectedModels;
	uint64_t m_loadTicket = 0, m_saveTicket = 0, m_restoreTicket = 0;

protected:
	void reloadPlugin();

	class AutomatableModel *modelAtPort(const QString &dest);

private:
	virtual DataFile::Types settingsType() = 0;
	virtual void setNameFromFile(const QString &fname) = 0;

public:
	struct LmmsPorts
	{
		unsigned samplecount;
		unsigned buffersize;
		long samplerate; // TODO: use const?
		std::vector<float> m_lUnprocessed, m_rUnprocessed, m_lProcessed,
			m_rProcessed;
		// only for directly connected ports (not OSC)
		struct TypedPorts
		{
			char m_type;
			union
			{
				float m_f;
				int m_i;
				bool m_b;
			} m_val;
			union
			{
				class FloatModel *m_floatModel;
				class IntModel *m_intModel;
				class BoolModel *m_boolModel;
			} m_connectedModel;
			TypedPorts() = default;
			TypedPorts(char type) : m_type(type) {}
			//! Should not be called, but QVector can't guarantee it
			TypedPorts(const TypedPorts &) = delete;
		};

		//! these are forwarded to the user in the LMMS-internal GUI
		std::vector<TypedPorts> m_userPorts;
		LmmsPorts(int bufferSize);
		std::unique_ptr<spa::audio::osc_ringbuffer> rb;
	} m_ports;

protected:
	void copyModelsToPorts();

private:
	friend struct LmmsVisitor;
	friend struct TypeChecker;

protected:
	QMutex m_pluginMutex;

	bool initPlugin();
	void shutdownPlugin();

	bool m_hasGUI;
	bool m_loaded;

	QString nodeName() const { return "spacontrols"; }

private:
	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &file);
};

#endif // LMMS_HAVE_SPA

#endif // SPA_CONTROL_BASE_H
