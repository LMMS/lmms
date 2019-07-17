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

//#include <QMutex>
#include <QString>
#include <memory>
#include <vector>

// general LMMS includes
#include "DataFile.h"
#include "LinkedModelGroups.h"
#include "lmms_basics.h"

// includes from the spa library
#include <spa/spa_fwd.h>

class SpaProc;

class SpaControlBase : public LinkedModelGroups
{
	friend class SpaViewBase;
public:
	SpaControlBase(Model *that, const QString &uniqueName,
				DataFile::Types settingsType);
	~SpaControlBase() override;

	std::vector<std::unique_ptr<SpaProc>>& controls() { return m_procs; }

	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);

//	void writeOsc(const char *dest, const char *args, va_list va) {}
//	void writeOsc(const char *dest, const char *args, ...) {}

	void loadFile(const QString &file);

	const spa::descriptor *m_spaDescriptor = nullptr;
	bool hasUi() const;
	void uiExtShow(bool doShow);
	void copyModelsFromLmms();
	void copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames);
	void copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const;
	void run(unsigned frames);

	AutomatableModel *modelAtPort(const QString &dest);
	void writeOscToAll(const char *dest, const char *args, va_list va);
	void writeOscToAll(const char *dest, const char *args...);
protected:
	void reloadPlugin() { /* TODO */ }
	bool isValid() { return m_valid; }

private:
	bool m_valid = true;

	virtual void setNameFromFile(const QString &fname) = 0;

	Model* m_that;

protected:

	LinkedModelGroup* getGroup(std::size_t idx) override;
	const LinkedModelGroup* getGroup(std::size_t idx) const override;

/*	bool initPlugin() {}
	void shutdownPlugin() {}*/

	bool m_hasGUI = false;
	bool m_loaded;

	QString nodeName() const { return "spacontrols"; }

	std::vector<std::unique_ptr<SpaProc>> m_procs;
	std::map<unsigned, SpaProc*> m_procsByPort;

private:
	unsigned m_channelsPerProc;
};

#endif // LMMS_HAVE_SPA

#endif // SPA_CONTROL_BASE_H
