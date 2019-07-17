/*
 * SpaControlBase.cpp - base class for spa instruments, effects, etc
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

#include "SpaControlBase.h"

#ifdef LMMS_HAVE_SPA

#include <QDebug>
#include <QFileInfo>
#include <QUrl>

#include <spa/spa.h>

#include "Engine.h"
#include "SpaManager.h"
#include "SpaProc.h"

SpaControlBase::SpaControlBase(Model* that, const QString& uniqueName,
								DataFile::Types settingsType) :
	m_spaDescriptor(Engine::getSPAManager()->getDescriptor(uniqueName)),
	m_that(that)
{
	if (m_spaDescriptor)
	{
			std::size_t procId = 0;
			int channelsLeft = DEFAULT_CHANNELS; // LMMS plugins are stereo
			while (channelsLeft > 0)
			{
					std::unique_ptr<SpaProc> newOne(
							new SpaProc(that, m_spaDescriptor, procId++,
										settingsType));
					if (newOne->isValid())
					{
						channelsLeft -= std::max(
								1 + static_cast<bool>(newOne->m_audioInCount),
								1 + static_cast<bool>(newOne->m_audioOutCount));
						Q_ASSERT(channelsLeft >= 0);
						if(newOne->netPort())
						{
							m_procsByPort.emplace(newOne->netPort(), newOne.get());
						}
						m_procs.push_back(std::move(newOne));
					}
					else
					{
							qCritical() << "Failed instantiating Spa processor";
							m_valid = false;
							channelsLeft = 0;
					}
			}
			if (m_valid)
			{
					m_channelsPerProc = DEFAULT_CHANNELS / m_procs.size();
					if (m_procs.size() > 1)
					{
							m_procs[0]->makeLinkingProc();
							createMultiChannelLinkModel();
					}

					// initially link all controls
					for (std::size_t i = 0; i < m_procs[0]->modelNum(); ++i)
					{
							linkModel(i, true);
					}
			}
	}
	else {
			qCritical() << "No SPA descriptor found for URI" << uniqueName;
			m_valid = false;
	}
	// TODO: error handling
}

void SpaControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
	LinkedModelGroups::saveSettings(doc, that);

	if (m_procs.empty()) { /* don't even add a "states" node */ }
	else
	{
		QDomElement states = doc.createElement("states");
		that.appendChild(states);

		char chanName[] = "state0";
		for (std::size_t chanIdx = 0; chanIdx < m_procs.size(); ++chanIdx)
		{
			chanName[5] = '0' + static_cast<char>(chanIdx);
			QDomElement channel = doc.createElement(
									QString::fromUtf8(chanName));
			states.appendChild(channel);
			m_procs[chanIdx]->saveState(doc, channel);
		}
	}
}

void SpaControlBase::loadSettings(const QDomElement &that)
{
	LinkedModelGroups::loadSettings(that);

	QDomElement states = that.firstChildElement("states");
	if (!states.isNull() && (!m_procs.empty()))
	{
		QDomElement lastChan;
		char chanName[] = "state0";
		for (std::size_t chanIdx = 0; chanIdx < m_procs.size(); ++chanIdx)
		{
			chanName[5] = '0' + static_cast<char>(chanIdx);
			QDomElement chan = states.firstChildElement(chanName);
			if (!chan.isNull()) { lastChan = chan; }

			m_procs[chanIdx]->loadState(lastChan);
		}
	}

}

SpaControlBase::~SpaControlBase() {}

void SpaControlBase::loadFile(const QString &file)
{
	// for now, only support loading one proc into all proc (duplicating)
	for(std::unique_ptr<SpaProc>& proc : m_procs)
		proc->loadFile(file);
	setNameFromFile(QFileInfo(file).baseName().replace(
		QRegExp("^[0-9]{4}-"), QString()));
}

bool SpaControlBase::hasUi() const
{
	// do not support external UI for mono effects yet
	// (how would that look??)
	return m_procs.size() == 1 && m_spaDescriptor->ui_ext();
}

void SpaControlBase::uiExtShow(bool doShow)
{
	if(m_procs.size() == 1)
		m_procs[0]->uiExtShow(doShow);
}

void SpaControlBase::writeOscToAll(
	const char *dest, const char *args, va_list va)
{
	for(std::unique_ptr<SpaProc>& proc : m_procs)
		proc->writeOsc(dest, args, va);
}

void SpaControlBase::writeOscToAll(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOscToAll(dest, args, va);
	va_end(va);
}

LinkedModelGroup *SpaControlBase::getGroup(std::size_t idx)
{
	return (idx < m_procs.size()) ? m_procs[idx].get() : nullptr;
}

const LinkedModelGroup *SpaControlBase::getGroup(std::size_t idx) const
{
	return (idx < m_procs.size()) ? m_procs[idx].get() : nullptr;
}




void SpaControlBase::copyModelsFromLmms() {
	for (std::unique_ptr<SpaProc>& c : m_procs) { c->copyModelsToPorts(); }
}




void SpaControlBase::copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames) {
	unsigned offset = 0;
	for (std::unique_ptr<SpaProc>& c : m_procs) {
		c->copyBuffersFromCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void SpaControlBase::copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const {
	unsigned offset = 0;
	for (const std::unique_ptr<SpaProc>& c : m_procs) {
		c->copyBuffersToCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void SpaControlBase::run(unsigned frames) {
	for (std::unique_ptr<SpaProc>& c : m_procs) { c->run(frames); }
}

AutomatableModel *SpaControlBase::modelAtPort(const QString &dest)
{
	if (m_procs.size() == 1)
	{
		QUrl url(dest);
		return m_procsByPort[static_cast<unsigned>(url.port())]->
			modelAtPort(dest);
	}
	else { return nullptr; /* TODO */ }
}


#endif // LMMS_HAVE_SPA
