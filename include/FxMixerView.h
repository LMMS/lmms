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

#include "FxMixer.h"
#include "ModelView.h"

class QStackedLayout;
class QButtonGroup;
class fader;
class FxLine;
class EffectRackView;
class pixmapButton;


class FxMixerView : public QWidget, public ModelView,
					public SerializingObjectHook
{
	Q_OBJECT
public:
	FxMixerView();
	virtual ~FxMixerView();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	FxLine * currentFxLine()
	{
		return m_currentFxLine;
	}
	void setCurrentFxLine( FxLine * _line );
	void setCurrentFxLine( int _line );

	void clear();


private slots:
	void updateFaders();
	void addNewChannel();

private:
	struct FxChannelView
	{
		FxLine * m_fxLine;
		EffectRackView * m_rackView;
		pixmapButton * m_muteBtn;
		fader * m_fader;
	} ;

	QVector<FxChannelView> m_fxChannelViews;

	QStackedLayout * m_fxRacksLayout;
	FxLine * m_currentFxLine;

	QScrollArea * channelArea;
	QHBoxLayout * chLayout;

	void addFxLine(int i, QWidget * parent, QLayout * layout);
} ;

#endif
