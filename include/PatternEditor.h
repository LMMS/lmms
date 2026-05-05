/*
 * PatternEditor.h - basic main-window for editing patterns
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_PATTERN_EDITOR_H
#define LMMS_GUI_PATTERN_EDITOR_H

#include "Editor.h"
#include "TrackContainerView.h"

namespace lmms
{

class PatternStore;

namespace gui
{

class ComboBox;
class TimeLineWidget;


class PatternEditor : public TrackContainerView
{
	Q_OBJECT
public:
	PatternEditor(PatternStore* ps);

	bool fixedClips() const override
	{
		return true;
	}

	void removeViewsForPattern(int pattern);

	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void addSampleTrack();
	void addAutomationTrack();
	void cloneClip();
	void updateMaxSteps();

protected slots:
	void dropEvent(QDropEvent * de ) override;
	void resizeEvent(QResizeEvent* de) override;
	void updatePosition();
	void updatePixelsPerBar();

private:
	PatternStore* m_ps;
	TimeLineWidget* m_timeLine;
	int m_trackHeadWidth;
	tick_t m_maxClipLength;
	void makeSteps( bool clone );
};


class PatternEditorWindow : public Editor
{
Q_OBJECT
public:
	PatternEditorWindow(PatternStore* ps);
	~PatternEditorWindow() = default;

	QSize sizeHint() const override;

	PatternEditor* m_editor;

public slots:
	void play() override;
	void stop() override;

private:
	ComboBox* m_patternComboBox;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PATTERN_EDITOR_H
