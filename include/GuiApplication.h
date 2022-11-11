/*
 * GuiApplication.h
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#ifndef GUIAPPLICATION_H
#define GUIAPPLICATION_H

#include <QObject>

#include "lmms_export.h"
#include "lmmsconfig.h"

class QLabel;

namespace lmms::gui
{

class AutomationEditorWindow;
class ControllerRackView;
class MixerView;
class MainWindow;
class MicrotunerConfig;
class PatternEditorWindow;
class PianoRollWindow;
class ProjectNotes;
class SongEditorWindow;

class LMMS_EXPORT GuiApplication : public QObject
{
	Q_OBJECT;
public:
	explicit GuiApplication();
	~GuiApplication() override;

	static GuiApplication* instance();
#ifdef LMMS_BUILD_WIN32
	static QFont getWin32SystemFont();
#endif

	MainWindow* mainWindow() { return m_mainWindow; }
	MixerView* mixerView() { return m_mixerView; }
	SongEditorWindow* songEditor() { return m_songEditor; }
	PatternEditorWindow* patternEditor() { return m_patternEditor; }
	PianoRollWindow* pianoRoll() { return m_pianoRoll; }
	ProjectNotes* getProjectNotes() { return m_projectNotes; }
	MicrotunerConfig* getMicrotunerConfig() { return m_microtunerConfig; }
	AutomationEditorWindow* automationEditor() { return m_automationEditor; }
	ControllerRackView* getControllerRackView() { return m_controllerRackView; }

public slots:
	void displayInitProgress(const QString &msg);

private slots:
	void childDestroyed(QObject *obj);

private:
	static GuiApplication* s_instance;

	MainWindow* m_mainWindow;
	MixerView* m_mixerView;
	SongEditorWindow* m_songEditor;
	AutomationEditorWindow* m_automationEditor;
	PatternEditorWindow* m_patternEditor;
	PianoRollWindow* m_pianoRoll;
	ProjectNotes* m_projectNotes;
	MicrotunerConfig* m_microtunerConfig;
	ControllerRackView* m_controllerRackView;
	QLabel* m_loadingProgressLabel;
};

// Short-hand function
LMMS_EXPORT GuiApplication* getGUI();

} // namespace lmms::gui

#endif // GUIAPPLICATION_H
