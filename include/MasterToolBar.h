/*
 * MasterToolBar.h - declaration of class MasterToolBar, a window where you can
 *                 setup your songs
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


#ifndef MASTER_TOOL_BAR_H
#define MASTER_TOOL_BAR_H

#include <QWidget>

class AutomatableSlider;
class IntegerSpinBox;
class MeterDialog;
class Song;
class TextFloat;

class MasterToolBar : public QWidget
{
Q_OBJECT
public:
	MasterToolBar( Song * song );
	~MasterToolBar();
	
	//TODO: Save settings & Load settings!

protected:
	virtual void closeEvent( QCloseEvent * _ce );
	
private slots:
	void setMasterVolume( int new_val );
	void showMasterVolumeFloat();
	void updateMasterVolumeFloat( int new_val );
	void hideMasterVolumeFloat();

	void setMasterPitch( int new_val );
	void showMasterPitchFloat();
	void updateMasterPitchFloat( int new_val );
	void hideMasterPitchFloat();

private:
	Song * m_song;
	
	IntegerSpinBox * m_tempoSpinBox;
	MeterDialog * m_timeSigDisplay;
	AutomatableSlider * m_masterVolumeSlider;
	AutomatableSlider * m_masterPitchSlider;
	
	TextFloat * m_mvsStatus;
	TextFloat * m_mpsStatus;
};

#endif
