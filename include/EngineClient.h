/*
 * EngineClient.h - Interface to the engine-system of LMMS
 *
 * Copyright (c) 2016 Michael Gregorius
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


#ifndef ENGINECLIENT_H
#define ENGINECLIENT_H

#include "export.h"

#include "Engine.h"

class Song;
class Mixer;
class FxMixer;
class BBTrackContainer;
class ProjectJournal;

class EXPORT EngineClient
{
public:
	EngineClient( Engine * engine) : m_engine(engine) {}

	Engine * getEngine() const { return m_engine; }
	Song * getSong() const { return m_engine->getSong(); }
	Mixer * getMixer() const { return m_engine->mixer(); }
	FxMixer * getFxMixer() const { return m_engine->fxMixer(); }
	BBTrackContainer * getBBTrackContainer() const { return m_engine->getBBTrackContainer(); }
	ProjectJournal * getProjectJournal() const { return m_engine->projectJournal(); }

private:
	Engine * m_engine;
};


#endif

