/*
 * GuiApplication.h
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#ifndef GUIAPPLICATION_H
#define GUIAPPLICATION_H

#include <QObject>
#include <QString>

#include "export.h"
#include "OscMsgListener.h"

class QLabel;
class QSplashScreen;

class AutomationEditorWindow;
class BBEditor;
class ControllerRackView;
class FxMixerView;
class MainWindow;
class PianoRollWindow;
class ProjectNotes;
class SongEditorWindow;

class EXPORT GuiApplication : public QObject, public OscMsgListener
{
	Q_OBJECT;
public:
	explicit GuiApplication(const QString &fileToLoad, const QString &fileToImport,
		bool fullscreen, bool exitAfterImport);
	~GuiApplication();

	static GuiApplication* instance();

	MainWindow* mainWindow() { return m_mainWindow; }
	FxMixerView* fxMixerView() { return m_fxMixerView; }
	SongEditorWindow* songEditor() { return m_songEditor; }
	BBEditor* getBBEditor() { return m_bbEditor; }
	PianoRollWindow* pianoRoll() { return m_pianoRoll; }
	ProjectNotes* getProjectNotes() { return m_projectNotes; }
	AutomationEditorWindow* automationEditor() { return m_automationEditor; }
	ControllerRackView* getControllerRackView() { return m_controllerRackView; }
	
	// override of callback defined by OscMsgListener
	void processMessage(const QByteArray &msg);

public slots:
	void displayInitProgress(const QString &msg);

private slots:
	void childDestroyed(QObject *obj);
	// this slot should always be connected via Qt::QueuedConnection so that it operates in the gui thread
	void processOscMsgInGuiThread(QByteArray msg);
	// staged initialization (in order to avoid blocking the gui thread)
	void initEngine();
	void initMainWindow();
	void initSongEditorWindow();
	void initFxMixerView();
	void initControllerRackView();
	void initProjectNotes();
	void initBbEditor();
	void initPianoRoll();
	void initAutomationEditor();
	void handleCtorOptions();
signals:
	void postInitEngine();
	void postInitMainWindow();
	void postInitSongEditorWindow();
	void postInitFxMixerView();
	void postInitControllerRackView();
	void postInitProjectNotes();
	void postInitBbEditor();
	void postInitPianoRoll();
	void postInitAutomationEditor();
	// sent whenever we receive an Open Sound Control message 
	//   (presumably from the core or another section of the UI)
	void receivedOscMessage(QByteArray msg);

private:
	static GuiApplication* s_instance;

	MainWindow* m_mainWindow;
	FxMixerView* m_fxMixerView;
	SongEditorWindow* m_songEditor;
	AutomationEditorWindow* m_automationEditor;
	BBEditor* m_bbEditor;
	PianoRollWindow* m_pianoRoll;
	ProjectNotes* m_projectNotes;
	ControllerRackView* m_controllerRackView;
	QLabel* m_loadingProgressLabel;
	QSplashScreen* m_splashScreen;

	// initialization arguments:
	QString m_fileToLoad;
	QString m_fileToImport;
	bool m_fullscreen;
	bool m_exitAfterImport;
};

#define gui GuiApplication::instance()

#endif // GUIAPPLICATION_H
