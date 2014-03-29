/*
 * FxMixerView.h - effect-mixer-view for LMMS
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _FX_MIXER_VIEW_H
#define _FX_MIXER_VIEW_H

#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QScrollArea>

#include "FxLine.h"
#include "FxMixer.h"
#include "ModelView.h"
#include "engine.h"
#include "fader.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "embed.h"
#include "EffectRackView.h"

class QButtonGroup;
class FxLine;

class EXPORT FxMixerView : public QWidget, public ModelView,
					public SerializingObjectHook
{
	Q_OBJECT
public:
	struct FxChannelView
	{
		FxChannelView(QWidget * _parent, FxMixerView * _mv, int _chIndex );

		FxLine * m_fxLine;
		pixmapButton * m_muteBtn;
		fader * m_fader;
	};


	FxMixerView();
	virtual ~FxMixerView();

	virtual void keyPressEvent(QKeyEvent * e);

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	inline FxLine * currentFxLine()
	{
		return m_currentFxLine;
	}

	inline FxChannelView * channelView(int index)
	{
		return m_fxChannelViews[index];
	}

	void setCurrentFxLine( FxLine * _line );
	void setCurrentFxLine( int _line );

	void clear();


	// display the send button and knob correctly
	void updateFxLine(int index);

	// notify the view that an fx channel was deleted
	void deleteChannel(int index);

	// move the channel to the left or right
	void moveChannelLeft(int index);
	void moveChannelRight(int index);

	// make sure the display syncs up with the fx mixer.
	// useful for loading projects
	void refreshDisplay();

private slots:
	void updateFaders();
	void addNewChannel();

private:

	QVector<FxChannelView *> m_fxChannelViews;

	FxLine * m_currentFxLine;

	QScrollArea * channelArea;
	QHBoxLayout * chLayout;
	QWidget * m_channelAreaWidget;
	EffectRackView * m_rackView;

	void updateMaxChannelSelector();
} ;

#endif
