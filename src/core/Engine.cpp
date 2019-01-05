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

#include <ctime>
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
#include "Knob.h"
#include "Note.h"
#include "BBTrack.h"
#include "InstrumentTrack.h"
#include "Pattern.h"
#include <iostream>
#include <QtCore/QTimer>
//#include <QtCore/QRandomGenerator>
//#include <QtQml/QQmlEngine>
#include <QtDeclarative/QtDeclarative>

#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_hints.h>

static SDL_Joystick* m_joystick = NULL;


float LmmsCore::s_framesPerTick;
Mixer* LmmsCore::s_mixer = NULL;
FxMixer * LmmsCore::s_fxMixer = NULL;
BBTrackContainer * LmmsCore::s_bbTrackContainer = NULL;
Song * LmmsCore::s_song = NULL;
ProjectJournal * LmmsCore::s_projectJournal = NULL;
Ladspa2LMMS * LmmsCore::s_ladspaManager = NULL;
DummyTrackContainer * LmmsCore::s_dummyTC = NULL;
QScriptEngine* LmmsCore::scriptEngine = NULL;
std::vector<SubprocessWrapper*> LmmsCore::s_processes = {};
double LmmsCore::s_gamepad_state[6] = {0.0};
bool LmmsCore::s_gamepad_buttons[11] = {false};


LmmsCore::LmmsCore() :m_rng(std::time(0)) {
	// default range 0.0 - 1.0
	this->m_uniform = std::uniform_real_distribution<double>();

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");  // required in SDL2 when initialized before display
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// Check for joystick
	if (SDL_NumJoysticks() > 0) {
		// Open joystick
		m_joystick = SDL_JoystickOpen(0);
		if (m_joystick) {
			printf("Opened Gamepad 0\n");
			printf("Name: %s\n", SDL_JoystickNameForIndex(0));  // SDL2
			printf("Number of Axes: %d\n", SDL_JoystickNumAxes(m_joystick));
			printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(m_joystick));
			printf("Number of Balls: %d\n", SDL_JoystickNumBalls(m_joystick));
			printf("Number of Hats: %d\n", SDL_JoystickNumHats(m_joystick));

			Knob::resetGamepads();

			this->m_sdlTimer = new QTimer(this);
			connect(m_sdlTimer, SIGNAL(timeout()), this, SLOT(updateSDL()));
			m_sdlTimer->start(1000/30);
		} else {
			printf("Couldn't open gamepad 0\n");
		}
	} else {
		printf("No gamepads are attached\n");	
	}

}


void LmmsCore::updateSDL() {
	SDL_Event event;
	SDL_PollEvent(&event);
	double x1 = ((double)SDL_JoystickGetAxis(m_joystick, 0)) / 32768.0;
	double y1 = ((double)SDL_JoystickGetAxis(m_joystick, 1)) / 32768.0;
	double z1 = (((double)SDL_JoystickGetAxis(m_joystick, 2)) / 32768.0) + 1.0;
	double x2 = ((double)SDL_JoystickGetAxis(m_joystick, 3)) / 32768.0;
	double y2 = ((double)SDL_JoystickGetAxis(m_joystick, 4)) / 32768.0;
	double z2 = (((double)SDL_JoystickGetAxis(m_joystick, 5)) / 32768.0) + 1.0;
	Knob::updateGamepad( x1,y1,z1, x2,y2,z2 );
	Engine::updateGamepad( x1,y1,z1, x2,y2,z2 );
	for (int i=0; i<11; i++) {
		if (SDL_JoystickGetButton(m_joystick, i)) {
			if (! Engine::s_gamepad_buttons[i]) {
				Engine::s_gamepad_buttons[i] = true;
				emit gamepadButtonPressed(i);
			}
		} else {
			if (Engine::s_gamepad_buttons[i]) {
				Engine::s_gamepad_buttons[i] = false;
				emit gamepadButtonReleased(i);
			}
		}
	}
	// because we are in a QTimer and not polling inside a while loop here, below has serious lag
	// the explicit Get functions above always respond in real time.
	//switch(event.type) {
	//	case SDL_JOYBUTTONDOWN:
	//		std::cout << "DOWN:" << event.jbutton.button << std::endl;
	//		break;
	//	case SDL_JOYBUTTONUP:
	//		std::cout << "UP:" << event.jbutton.button << std::endl;
	//		break;
	//	case SDL_KEYDOWN:
	//		std::cout << "KEYDOWN:" << event.key.keysym.scancode << std::endl;
	//		//std::cout << "unicode:" << event.key.keysym.unicode << std::endl;
	//		std::cout << "mod:" << event.key.keysym.mod << std::endl;
	//		std::cout << "name:" << event.key.keysym.sym << std::endl;
	//		break;
	//}
}


template <typename T> void addType(QScriptEngine* engine) {
	auto constructor = engine->newFunction([](QScriptContext*, QScriptEngine* engine){
		return engine->newQObject(new T());
	});
	auto value = engine->newQMetaObject(&T::staticMetaObject, constructor);
	engine->globalObject().setProperty(T::staticMetaObject.className(), value);
}

void LmmsCore::scriptEnable() {
	//qmlRegisterType<Mixer>("lmms.core", 1,0, "Mixer");
	qmlRegisterType<Song>("lmms.core", 1,0, "Song");
	//qmlRegisterType<BBTrackContainer>("lmms.core", 1,0, "BBTrackContainer");
	qmlRegisterType<BBTrack>("lmms.core", 1,0, "BBTrack");
	qmlRegisterType<InstrumentTrack>("lmms.core", 1,0, "InstrumentTrack");
	qmlRegisterType<Pattern>("lmms.core", 1,0, "Pattern");
	qmlRegisterType<NoteScriptWrapper>("lmms.core", 1,0, "Note");
	qmlRegisterType<SubprocessWrapper>("lmms.core", 1,0, "SubprocessWrapper");

	//qRegisterMetaType<SubprocessWrapper>("SubprocessWrapper");

	gui->mainWindow()->enableScriptTools();

	LmmsCore::scriptEngine = new QScriptEngine();
	addType<QTimer>(LmmsCore::scriptEngine);

	QScriptValue fun = LmmsCore::scriptEngine->newFunction(LmmsCore::scriptPrint);
	LmmsCore::scriptEngine->globalObject().setProperty("print", fun);

	QScriptValue fun2 = LmmsCore::scriptEngine->newFunction(LmmsCore::generateRandom);
	LmmsCore::scriptEngine->globalObject().setProperty("random", fun2);

	LmmsCore *engine = inst();  // the singleton instance of LmmsCore
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
	gui->mainWindow()->setScriptDebug( txt.toString() );
	return txt;
}
QScriptValue LmmsCore::generateRandom(QScriptContext *context, QScriptEngine *engine) {
	//QScriptValue r(engine, QRandomGenerator::global()->generateDouble());
	QScriptValue r(engine, inst()->m_uniform(inst()->m_rng));
	return r;
}



void LmmsCore::init( bool renderOnly )
{
	LmmsCore *engine = inst();
	engine->setObjectName("LmmsCore");

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

void LmmsCore::shutdownSDL() {
	std::cout << "shutdown SDL..." << std::endl;
	inst()->m_sdlTimer->stop();
	if (m_joystick) SDL_JoystickClose(m_joystick);
	SDL_Quit();
	std::cout << "shutdown SDL OK" << std::endl;
}


void LmmsCore::destroy()
{
	std::cout << "LmmsCore::destroy..." << std::endl;
	scriptEngine->abortEvaluation();
	std::cout << "scriptEngine halted" << std::endl;
	s_projectJournal->stopAllJournalling();
	std::cout << "ProjectJournal halted" << std::endl;
	s_mixer->stopProcessing();
	std::cout << "Mixer halted" << std::endl;
	PresetPreviewPlayHandle::cleanup();
	std::cout << "PresetPreviewPlayHandle cleanup OK" << std::endl;
	s_song->clearProject();
	std::cout << "song cleared OK" << std::endl;
	deleteHelper( &s_bbTrackContainer );
	deleteHelper( &s_dummyTC );

	deleteHelper( &s_fxMixer );
	deleteHelper( &s_mixer );

	deleteHelper( &s_ladspaManager );

	//delete ConfigManager::inst();
	deleteHelper( &s_projectJournal );

	deleteHelper( &s_song );
	std::cout << "free memory OK" << std::endl;
	delete ConfigManager::inst();
	std::cout << "LmmsCore::destroy OK" << std::endl;

}




void LmmsCore::updateFramesPerTick()
{
	s_framesPerTick = s_mixer->processingSampleRate() * 60.0f * 4 /
				DefaultTicksPerTact / s_song->getTempo();
}

LmmsCore * LmmsCore::s_instanceOfMe = NULL;
