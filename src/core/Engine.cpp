/*
 * Engine.cpp - implementation of LMMS' engine-system
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "Engine.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "FxMixer.h"
#include "Ladspa2LMMS.h"
#include "Mixer.h"
#include "PresetPreviewPlayHandle.h"
#include "ProjectJournal.h"
#include "Song.h"
#include "BandLimitedWave.h"

#include "Note.h"
#include "BBTrack.h"
#include "InstrumentTrack.h"
#include "Pattern.h"
#include <iostream>
#include <QtCore/QTimer>
#include <QtCore/QRandomGenerator>
#include <QtQml/QQmlEngine>

float LmmsCore::s_framesPerTick;
Mixer* LmmsCore::s_mixer = NULL;
FxMixer * LmmsCore::s_fxMixer = NULL;
BBTrackContainer * LmmsCore::s_bbTrackContainer = NULL;
Song * LmmsCore::s_song = NULL;
ProjectJournal * LmmsCore::s_projectJournal = NULL;
Ladspa2LMMS * LmmsCore::s_ladspaManager = NULL;
DummyTrackContainer * LmmsCore::s_dummyTC = NULL;
QScriptEngine* LmmsCore::scriptEngine = NULL;

template <typename T> void addType(QScriptEngine* engine) {
	auto constructor = engine->newFunction([](QScriptContext*, QScriptEngine* engine){
		return engine->newQObject(new T());
	});
	auto value = engine->newQMetaObject(&T::staticMetaObject, constructor);
	engine->globalObject().setProperty(T::staticMetaObject.className(), value);
}

void LmmsCore::scriptEnable() {
	qmlRegisterType<Mixer>("lmms.core", 1,0, "Mixer");
	qmlRegisterType<Song>("lmms.core", 1,0, "Song");
	qmlRegisterType<BBTrackContainer>("lmms.core", 1,0, "BBTrackContainer");
	qmlRegisterType<BBTrack>("lmms.core", 1,0, "BBTrack");
	qmlRegisterType<InstrumentTrack>("lmms.core", 1,0, "InstrumentTrack");
	qmlRegisterType<Pattern>("lmms.core", 1,0, "Pattern");
	qmlRegisterType<NoteScriptWrapper>("lmms.core", 1,0, "Note");

	LmmsCore::scriptEngine = new QScriptEngine();
	addType<QTimer>(LmmsCore::scriptEngine);

	QScriptValue fun = LmmsCore::scriptEngine->newFunction(LmmsCore::scriptPrint);
	LmmsCore::scriptEngine->globalObject().setProperty("print", fun);

	QScriptValue fun2 = LmmsCore::scriptEngine->newFunction(LmmsCore::generateRandom);
	LmmsCore::scriptEngine->globalObject().setProperty("random", fun2);

	LmmsCore *engine = inst();  // the singleton instance of LmmsCore
	//auto engine = new LmmsCoreScriptWrapper();
	QScriptValue ewrapper = LmmsCore::scriptEngine->newQObject(engine);
	LmmsCore::scriptEngine->globalObject().setProperty("lmms", ewrapper);

	LmmsCore::scriptEngine->evaluate(R"HEADER(
	function dir(object) {
		var names = [];
		for (s in object) {
			names.push(s);
		}
		names.sort();
		return names;
	}
	function setTimeout(fn, ms) {
		var timer = new QTimer();
		timer.interval = ms;
		timer.singleShot = true;
		var conn = timer.timeout.connect(fn);
		timer.start();
	}
	function setInterval(fn, ms) {
		var timer = new QTimer();
		timer.interval = ms;
		timer.singleShot = false;
		var conn = timer.timeout.connect(fn);
		timer.start();
	}
	)HEADER");
}

void LmmsCore::scriptEval( std::string script, std::string fileName) {
	LmmsCore::scriptEval(QString(script.c_str()), QString(fileName.c_str()));
}
void LmmsCore::scriptEval( QString script, QString fileName) {
	QScriptValue result = LmmsCore::scriptEngine->evaluate(script, fileName);
	if (LmmsCore::scriptEngine->hasUncaughtException()) {
		int line = LmmsCore::scriptEngine->uncaughtExceptionLineNumber();
		//std::cout << "uncaught exception at line" << line << ":" << result.toString() << std::endl;
		std::cout << "uncaught exception at line" << line << ":" << result.toString().toUtf8().constData() << std::endl;
	} else {
		std::cout << "script result: " << result.toString().toUtf8().constData() << std::endl;		
	}
}
QScriptValue LmmsCore::scriptPrint(QScriptContext *context, QScriptEngine *engine) {
	QScriptValue txt = context->argument(0);
	std::cout << txt.toString().toUtf8().constData() << std::endl;		
	return txt;
}
QScriptValue LmmsCore::generateRandom(QScriptContext *context, QScriptEngine *engine) {
	QScriptValue r(engine, QRandomGenerator::global()->generateDouble());
	return r;
}



void LmmsCore::init( bool renderOnly )
{
	LmmsCore *engine = inst();

	emit engine->initProgress(tr("Generating wavetables"));
	// generate (load from file) bandlimited wavetables
	BandLimitedWave::generateWaves();

	emit engine->initProgress(tr("Initializing data structures"));
	s_projectJournal = new ProjectJournal;
	s_mixer = new Mixer( renderOnly );
	s_song = new Song;
	s_fxMixer = new FxMixer;
	s_bbTrackContainer = new BBTrackContainer;

	s_ladspaManager = new Ladspa2LMMS;

	s_projectJournal->setJournalling( true );

	emit engine->initProgress(tr("Opening audio and midi devices"));
	s_mixer->initDevices();

	PresetPreviewPlayHandle::init();
	s_dummyTC = new DummyTrackContainer;

	emit engine->initProgress(tr("Launching mixer threads"));
	s_mixer->startProcessing();
}




void LmmsCore::destroy()
{
	s_projectJournal->stopAllJournalling();
	s_mixer->stopProcessing();

	PresetPreviewPlayHandle::cleanup();

	s_song->clearProject();

	deleteHelper( &s_bbTrackContainer );
	deleteHelper( &s_dummyTC );

	deleteHelper( &s_fxMixer );
	deleteHelper( &s_mixer );

	deleteHelper( &s_ladspaManager );

	//delete ConfigManager::inst();
	deleteHelper( &s_projectJournal );

	deleteHelper( &s_song );

	delete ConfigManager::inst();
}




void LmmsCore::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}

LmmsCore * LmmsCore::s_instanceOfMe = NULL;
