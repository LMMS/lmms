/*
 * Engine.h - engine-system of LMMS
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


#ifndef ENGINE_H
#define ENGINE_H

#include <random>
#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtCore/QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "SubprocessWrapper.h"
#include "GuiApplication.h"
#include "MainWindow.h"
//#include "lmms_export.h"
#include "Song.h"

class BBTrackContainer;
class DummyTrackContainer;
class FxMixer;
class ProjectJournal;
class Mixer;
//class Song;
class Ladspa2LMMS;


class QsWidget : public QWidget {
	Q_OBJECT
	public slots:
		void setLayout(QLayout * layout) {
			QWidget::setLayout(layout);
		}
};

class QsHBoxLayout : public QHBoxLayout {
	Q_OBJECT
	public slots:
		void addWidget(QWidget * layout) {
			QHBoxLayout::addWidget(layout);
		}
};

class QsVBoxLayout : public QVBoxLayout {
	Q_OBJECT
	public slots:
		void addWidget(QWidget * layout) {
			QVBoxLayout::addWidget(layout);
		}
};

// Note: This class is called 'LmmsCore' instead of 'Engine' because of naming
// conflicts caused by ZynAddSubFX. See https://github.com/LMMS/lmms/issues/2269
// and https://github.com/LMMS/lmms/pull/2118 for more details.
//
// The workaround was to rename Lmms' Engine so that it has a different symbol
// name in the object files, but typedef it back to 'Engine' and keep it inside
// of Engine.h so that the rest of the codebase can be oblivious to this issue
// (and it could be fixed without changing every single file).

class LmmsCore;
typedef LmmsCore Engine;

class LmmsCore : public QObject
{
	Q_OBJECT
	//Q_PROPERTY(Mixer* mixer MEMBER s_mixer)
	//Q_PROPERTY(FxMixer* fxMixer MEMBER s_fxMixer)
	//Q_PROPERTY(Song* song READ getSong)  // not Qt4 QtScript compatible
	//Q_PROPERTY(BBTrackContainer* bbTrackContainer MEMBER s_bbTrackContainer)
	Q_PROPERTY(QScriptValue song READ __getSong__)
	Q_PROPERTY(QScriptValue toolbar READ __getToolBar__)
	Q_PROPERTY(QScriptValue workspace READ __getWorkspace__)
	Q_PROPERTY(QScriptValue mainwindow READ __getMainWindow__)

	static std::vector<SubprocessWrapper*> s_processes;

	// QScriptValue property getter wrappers were not required in Qt5 QtScript
	// Here in Qt4 we must directly call newQObject to wrap the property.
	QScriptValue __getSong__() {
		return this->scriptEngine->newQObject((QObject*)getSong());
	}
	QScriptValue __getToolBar__() {
		return this->scriptEngine->newQObject((QObject*)getToolBar());
	}
	QScriptValue __getWorkspace__() {
		return this->scriptEngine->newQObject((QObject*)getWorkspace());
	}
	QScriptValue __getMainWindow__() {
		return this->scriptEngine->newQObject((QObject*)gui->mainWindow());
	}

public slots:
	inline SubprocessWrapper* newProcess(QString exe, QStringList args, bool capture=false, int width=320, int height=240) {
		auto parent = new SubWindow(gui->mainWindow()); // seems to also break mplayer with native xembed support?
		auto sw = new SubprocessWrapper(exe,args, capture, parent, width, height);
		//auto sw = new SubprocessWrapper(exe,args, capture, NULL, width, height);
		s_processes.push_back( sw );
		return sw;
	}
	inline QMdiArea* getWorkspace() {
		return gui->mainWindow()->workspace();
	}
	inline QWidget* getToolBar() {
		return gui->mainWindow()->toolBar();
	}
	inline QString getScript() {
		return gui->mainWindow()->getScript();
	}
	inline void setScript(QString txt) {
		gui->mainWindow()->setScript(txt);
	}
	inline double gamepadAxis(int index) {
		return LmmsCore::s_gamepad_state[index];
	}

	void updateSDL();

public:
	LmmsCore();
	static void init( bool renderOnly );
	static void destroy();

	// core
	static Mixer *mixer()
	{
		return s_mixer;
	}

	static FxMixer * fxMixer()
	{
		return s_fxMixer;
	}

	static Song * getSong()
	{
		return s_song;
	}

	static BBTrackContainer * getBBTrackContainer()
	{
		return s_bbTrackContainer;
	}

	static ProjectJournal * projectJournal()
	{
		return s_projectJournal;
	}

	static Ladspa2LMMS * getLADSPAManager()
	{
		return s_ladspaManager;
	}

	static DummyTrackContainer * dummyTrackContainer()
	{
		return s_dummyTC;
	}

	static float framesPerTick()
	{
		return s_framesPerTick;
	}
	static void updateFramesPerTick();

	static inline LmmsCore * inst()
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new LmmsCore();
		}
		return s_instanceOfMe;
	}

	static QScriptEngine* scriptEngine;
	static void scriptEnable();
	static QScriptValue scriptPrint(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue generateRandom(QScriptContext *context, QScriptEngine *engine);
	static void scriptEval(std::string script, std::string fileName="");
	static void scriptEval(QString script, QString fileName="");
	static void updateGamepad(double x1,double y1,double z1,  double x2,double y2,double z2) {
		LmmsCore::s_gamepad_state[0] = x1;
		LmmsCore::s_gamepad_state[1] = y1;
		LmmsCore::s_gamepad_state[2] = z1;
		LmmsCore::s_gamepad_state[3] = x2;
		LmmsCore::s_gamepad_state[4] = y2;
		LmmsCore::s_gamepad_state[5] = z2;
	}
	static void shutdownSDL();

signals:
	void initProgress(const QString &msg);
	void gamepadButtonPressed(int i);
	void gamepadButtonReleased(int i);


private:
	// small helper function which sets the pointer to NULL before actually deleting
	// the object it refers to
	template<class T>
	static inline void deleteHelper( T * * ptr )
	{
		T * tmp = *ptr;
		*ptr = NULL;
		delete tmp;
	}

	static float s_framesPerTick;

	// core
	static Mixer *s_mixer;
	static FxMixer * s_fxMixer;
	static Song * s_song;
	static BBTrackContainer * s_bbTrackContainer;
	static ProjectJournal * s_projectJournal;
	static DummyTrackContainer * s_dummyTC;
	static Ladspa2LMMS * s_ladspaManager;

	// Mersenne Twister 19937 (64 bits)
	std::mt19937_64 m_rng;
	std::uniform_real_distribution<double> m_uniform;

	static double s_gamepad_state[6];
	static bool   s_gamepad_buttons[11];

	//SDL_Joystick *m_joystick; moved to a private global in Engine.cpp (so we do not have to include SDL2 here)
	QTimer* m_sdlTimer;

	// even though most methods are static, an instance is needed for Qt slots/signals
	static LmmsCore * s_instanceOfMe;
	friend class GuiApplication;
};

#endif