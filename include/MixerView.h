/*
 * MixerView.h - effect-mixer-view for LMMS
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MIXER_VIEW_H
#define MIXER_VIEW_H

#include <QMenu>
#include <QWidget>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QScrollArea>

#include "ModelView.h"
#include "Mixer.h"
#include "Engine.h"
#include "Fader.h"
#include "PixmapButton.h"
#include "embed.h"
#include "EffectRackView.h"
#include "Track.h"

class QButtonGroup;


namespace lmms::gui
{

class MixerLine;

class LMMS_EXPORT MixerView : public QWidget, public ModelView,
					public SerializingObjectHook
{
	Q_OBJECT
public:
	class MixerChannelView
	{
	public:
		MixerChannelView(QWidget * _parent, MixerView * _mv, int _chIndex );

		void setChannelIndex( int index );

		MixerLine * m_mixerLine;
		PixmapButton * m_muteBtn;
		PixmapButton * m_soloBtn;
		Fader * m_fader;
		EffectRackView * m_rackView;
	};


	MixerView();
	~MixerView() override;

	void keyPressEvent(QKeyEvent * e) override;

	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;

	inline MixerLine * currentMixerLine()
	{
		return m_currentMixerLine;
	}

	inline MixerChannelView * channelView(int index)
	{
		return m_mixerChannelViews[index];
	}


	void setCurrentMixerLine( MixerLine * _line );
	void setCurrentMixerLine( int _line );

	void clear();


	// display the send button and knob correctly
	void updateMixerLine(int index);

	// notify the view that a mixer channel was deleted
	void deleteChannel(int index);

	// delete all unused channels
	void deleteUnusedChannels();

	// move the channel to the left or right
	void moveChannelLeft(int index);
	void moveChannelRight(int index);

	void renameChannel(int index);

	// make sure the display syncs up with the mixer.
	// useful for loading projects
	void refreshDisplay();

	// Auto track link support	
	void updateAfterTrackAdd(Track * track, QString name = "");
	void updateAfterTrackStyleModify(Track * track);
	void trackMixerLineCreate(Track * track);
	void trackMixerLineAssign(Track * track, int channelIndex);
	void updateAfterTrackMove(Track * track);
	void updateBeforeTrackDelete(Track * track);
	void toggleAutoTrackLink(int index);


public slots:
	int addNewChannel();

protected:
	void closeEvent( QCloseEvent * _ce ) override;
	
private slots:
	void updateFaders();
	void toggledSolo();
	void updateAutoTrackLinkMenu();
	void toogleAutoLinkTrackConfig();

private:
	QVector<MixerChannelView *> m_mixerChannelViews;

	MixerLine * m_currentMixerLine;

	QScrollArea * channelArea;
	QHBoxLayout * chLayout;
	QWidget * m_channelAreaWidget;
	QStackedLayout * m_racksLayout;
	QWidget * m_racksWidget;
	QPushButton * m_toogleAutoLinkTrackConfigBtn;
	QPushButton * m_autoLinkTrackSettingsBtn;

	void updateMaxChannelSelector();
	void swapChannels(int indexA, int indexB);
	void updateAutoTrackSortOrder(bool autoSort = true);
	void deleteChannelInternal(int index);
	void setAutoLinkTrackConfig(bool enabled);
	void updateAutoLinkTrackConfigBtn(bool enabled);
	void setAutoTrackConstraints();
	void trackStyleToChannel(Track * track, MixerChannel * channel);
	void channelStyleToTrack(MixerChannel * channel, Track * track);
	void addAutoLinkTrackMenuEntry(QMenu* menu, const QString &text, bool state,
		std::function<void(Mixer::autoTrackLinkSettings* settings)> setValue,
		std::function<void()> after = nullptr);

	friend class MixerChannelView;
} ;


} // namespace lmms::gui

#endif
