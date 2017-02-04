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

#include <QtCore/QObject>

#include "export.h"

class QLabel;

class AutomationEditorWindow;
class BBEditor;
class ControllerRackView;
class FxMixerView;
class GrooveView;
class StudioControllerView;
class MainWindow;
class PianoRollWindow;
class ProjectNotes;
class SongEditorWindow;

class EXPORT GuiApplication : public QObject
{
	Q_OBJECT;
public:
	explicit GuiApplication();
	~GuiApplication();

	static GuiApplication* instance();

	MainWindow* mainWindow() { return m_mainWindow; }
	FxMixerView* fxMixerView() { return m_fxMixerView; }
	GrooveView* grooveView() { return m_grooveView; }
	StudioControllerView* studioControllerView() { return m_studioControllerView; }
	SongEditorWindow* songEditor() { return m_songEditor; }
	BBEditor* getBBEditor() { return m_bbEditor; }
	PianoRollWindow* pianoRoll() { return m_pianoRoll; }
	ProjectNotes* getProjectNotes() { return m_projectNotes; }
	AutomationEditorWindow* automationEditor() { return m_automationEditor; }
	ControllerRackView* getControllerRackView() { return m_controllerRackView; }

public slots:
	void displayInitProgress(const QString &msg);

private slots:
	void childDestroyed(QObject *obj);

private:
	static GuiApplication* s_instance;

	MainWindow* m_mainWindow;
	FxMixerView* m_fxMixerView;
	GrooveView* m_grooveView;
	StudioControllerView* m_studioControllerView;
	SongEditorWindow* m_songEditor;
	AutomationEditorWindow* m_automationEditor;
	BBEditor* m_bbEditor;
	PianoRollWindow* m_pianoRoll;
	ProjectNotes* m_projectNotes;
	ControllerRackView* m_controllerRackView;
	QLabel* m_loadingProgressLabel;
};

#define gui GuiApplication::instance()

#endif // GUIAPPLICATION_H
