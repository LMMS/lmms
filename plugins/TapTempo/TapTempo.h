/*
 * TapTempo.h - plugin to count beats per minute
 *
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
 * 
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

#ifndef TAPTEMPO_H
#define TAPTEMPO_H

#include <QPushButton>
#include <QVector>
#include <QTime>

#include "ToolPlugin.h"
#include "ToolPluginView.h"

class TapTempoView : public ToolPluginView 
{
	Q_OBJECT
public:
	TapTempoView(ToolPlugin *);
	
	void onBpmClick();
	
	void keyPressEvent(QKeyEvent *) override;
	void keyReleaseEvent(QKeyEvent *) override;

private:
	QTime m_previousTime;
	QPushButton * m_bpmButton;
	int m_bpmAverage;
	int m_numTaps;
	bool m_keyDown;
};

class TapTempo : public ToolPlugin
{
	Q_OBJECT;
public:
	TapTempo();
	
	virtual PluginView * instantiateView(QWidget *)
	{
		return new TapTempoView(this);
	}

	virtual QString nodeName() const;

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}
};

#endif