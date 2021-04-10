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

#include "AutomatableModel.h"
#include "SpaControlBase.h"

#ifdef LMMS_HAVE_SPA

#include <QDebug>
#include <QFileInfo>
#include <QThread>
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
			int channelsLeft = DEFAULT_CHANNELS; // LMMS plugins are stereo
			while (channelsLeft > 0)
			{
					std::unique_ptr<SpaProc> newOne(
							new SpaProc(that, m_spaDescriptor, settingsType));
					if (newOne->isValid())
					{
						channelsLeft -= std::max(
								1 + static_cast<bool>(newOne->m_audioInCount),
								1 + static_cast<bool>(newOne->m_audioOutCount));
						Q_ASSERT(channelsLeft >= 0);
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
					linkAllModels();
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

	LinkedModelGroups::saveSettings(doc, that);
}

void SpaControlBase::loadSettings(const QDomElement &that)
{
	// first load state, then the ports ("settings")
	// if we first load the ports, those ports might not exist in current state

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

	// this will load only initial models and ignore added models
	LinkedModelGroups::loadSettings(that);

	bool slept = false;
	QDomElement models = that.firstChildElement("models");
	for(QDomElement el = models.firstChildElement(); !el.isNull();
		el = el.nextSiblingElement())
	{
		QString nodename = el.hasAttribute("nodename")	? el.attribute("nodename")
														: el.nodeName();
		if(!m_procs[0]->containsModel(nodename))
		{
			if (!slept)
			{
				QThread::sleep(3);
				slept = true;
			}
			AutomatableModel* newModel = modelAtPort(nodename); // create model in all processes
			newModel->loadSettings(models, nodename);
			for(std::unique_ptr<SpaProc>& proc : m_procs)
			{
				proc->addModel(newModel, nodename);
			}
		}
	}
}

SpaControlBase::~SpaControlBase() {}

void SpaControlBase::loadFile(const QString &file, bool user)
{
	// for now, only support loading one proc into all proc (duplicating)
	for(std::unique_ptr<SpaProc>& proc : m_procs)
		proc->loadFile(file, user);
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
	QUrl url(dest);
	// create model at all ports, if it does not yet exist
	for (std::unique_ptr<SpaProc>& proc : m_procs)
	{
		proc->modelAtPort(url.path());
	}
	linkAllModels(); // link the newly created models
	// now return the right model
	// always return the first proc's model, since this function is used for
	// automation (all other proc's equivalent models are linked, and thus don't
	// require automation)
	return m_procs[0]->modelAtPort(url.path());
}

void SpaControlBase::handleMidiInputEvent(const MidiEvent &event,
	const TimePos &time, f_cnt_t offset)
{
	for (auto& c : m_procs) { c->handleMidiInputEvent(event, time, offset); }
}


#endif // LMMS_HAVE_SPA
