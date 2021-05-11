/*
 * SampleTrackWindow.h
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
 
#ifndef SAMPLE_TRACK_WINDOW_H
#define SAMPLE_TRACK_WINDOW_H

#include "FxLineLcdSpinBox.h"

#include <QLineEdit>

#include "EffectRackView.h"
#include "SampleTrack.h"
 

class SampleTrackWindow : public QWidget, public ModelView, public SerializingObjectHook
{
	Q_OBJECT
public:
	SampleTrackWindow(SampleTrackView * tv);
	virtual ~SampleTrackWindow();

	SampleTrack * model()
	{
		return castModel<SampleTrack>();
	}

	const SampleTrack * model() const
	{
		return castModel<SampleTrack>();
	}

	void setSampleTrackView(SampleTrackView * tv);

	SampleTrackView *sampleTrackView()
	{
		return m_stv;
	}


public slots:
	void textChanged(const QString & new_name);
	void toggleVisibility(bool on);
	void updateName();


protected:
	// capture close-events for toggling sample-track-button
	void closeEvent(QCloseEvent * ce) override;

	void saveSettings(QDomDocument & doc, QDomElement & element) override;
	void loadSettings(const QDomElement & element) override;

private:
	void modelChanged() override;

	SampleTrack * m_track;
	SampleTrackView * m_stv;

	// widgets on the top of an sample-track-window
	QLineEdit * m_nameLineEdit;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	FxLineLcdSpinBox * m_effectChannelNumber;

	EffectRackView * m_effectRack;
} ;



#endif