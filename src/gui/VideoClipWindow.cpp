/*
 * VideoClipWindow.cpp
 *
 * Copyright (c) 2024 regulus79
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
#include <QVBoxLayout>
#include <QSizePolicy>

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "VideoClip.h"
#include "VideoClipWindow.h"
#include "Song.h"
#include "SubWindow.h"

namespace lmms::gui
{

VideoClipWindow::VideoClipWindow(VideoClip * vclip):
	QWidget(),
	m_clip(vclip)
{
	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget(this);
	subWin->resize(640,480);
	subWin->hide();

	QVBoxLayout * layout = new QVBoxLayout(this);

	m_mediaPlayer = new QMediaPlayer(this);
	m_videoWidget = new QVideoWidget(this);

	layout->addWidget(m_videoWidget);
	m_mediaPlayer->setVideoOutput(m_videoWidget);

	connect(Engine::getSong(), &Song::playbackStateChanged, this, &VideoClipWindow::playbackStateChanged);
	connect(Engine::getSong(), &Song::playbackPositionChanged, this, &VideoClipWindow::playbackPositionChanged);

	connect(m_clip, &VideoClip::videoChanged, this, &VideoClipWindow::videoChanged);

	connect(Engine::getSong(), &Song::tempoChanged, this, &VideoClipWindow::durationChanged);
	connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &VideoClipWindow::durationChanged);

	// TODO: Checking for updates 60 times per second is not optimal; it would be better to connect this to
	// a positionChanged signal from the timeline, but unfortunately the Song's getTimeline function returns
	// a Timeline object, while the object which gives the signal is TimeLineWidget (which we cannot access).
	
	// Copied from TimeLineWidget.cpp
	auto updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &VideoClipWindow::playbackStateChanged);
	updateTimer->start( 1000 / 60 );  // 60 fps
}

void VideoClipWindow::videoChanged()
{
	m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_clip->videoFile()));
}

void VideoClipWindow::durationChanged()
{
	float millisecondsPerTick = TimePos::ticksToMilliseconds(1, Engine::getSong()->getTempo());
	TimePos length = m_mediaPlayer->duration() / millisecondsPerTick;
	m_clip->changeLength(length + m_clip->startTimeOffset());
}

void VideoClipWindow::playbackStateChanged()
{
	int currentTime = Engine::getSong()->getPlayPos().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipStartTime = m_clip->startPosition().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipStartOffset = m_clip->startTimeOffset().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipLengthMilliseconds = m_clip->length().getTimeInMilliseconds(Engine::getSong()->getTempo());

	int videoStartTime = clipStartTime + (clipStartOffset > 0? clipStartOffset : 0);

	if (Engine::getSong()->playMode() == Song::PlayMode::Song)
	{
		if (Engine::getSong()->isPlaying() && currentTime >= videoStartTime && currentTime < clipStartTime + clipLengthMilliseconds
			&& m_mediaPlayer->state() != QMediaPlayer::PlayingState)
		{
			m_mediaPlayer->play();
			m_mediaPlayer->setPosition(std::clamp(currentTime - clipStartTime - clipStartOffset, 0, static_cast<int>(m_mediaPlayer->duration())));
		}
		else if (!Engine::getSong()->isPlaying() || (Engine::getSong()->isPlaying() && (currentTime < videoStartTime || currentTime >= clipStartTime + clipLengthMilliseconds)
					&& m_mediaPlayer->state() == QMediaPlayer::PlayingState))
		{
			m_mediaPlayer->pause();
			m_mediaPlayer->setPosition(std::clamp(currentTime - clipStartTime - clipStartOffset, 0, static_cast<int>(m_mediaPlayer->duration())));
		}
	}
	else if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
	{
		m_mediaPlayer->pause();
		m_mediaPlayer->setPosition(std::clamp(currentTime - clipStartTime - clipStartOffset, 0, static_cast<int>(m_mediaPlayer->duration())));
	}
}

void VideoClipWindow::playbackPositionChanged()
{
	int currentTime = Engine::getSong()->getPlayPos().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipStartTime = m_clip->startPosition().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipStartOffset = m_clip->startTimeOffset().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int clipLengthMilliseconds = m_clip->length().getTimeInMilliseconds(Engine::getSong()->getTempo());
	int videoStartTime = clipStartTime + (clipStartOffset > 0? clipStartOffset : 0);

	if (currentTime >= videoStartTime && currentTime < clipStartTime + clipLengthMilliseconds)
	{
		m_mediaPlayer->setPosition(std::clamp(currentTime - clipStartTime - clipStartOffset, 0, static_cast<int>(m_mediaPlayer->duration())));
	}
}

// Copied from SampleTrackWindow.cpp
void VideoClipWindow::toggleVisibility(bool on)
{
	if(on)
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}

// Copied from SampleTrackWindow.cpp
void VideoClipWindow::closeEvent(QCloseEvent* ce)
{
	ce->ignore();

	if(getGUI()->mainWindow()->workspace())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
}

} // namespace lmms::gui
