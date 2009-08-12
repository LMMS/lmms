/*
 * setup_dialog_mcl.h - dialog for setting up MIDI Control Listener
 *
 * Copyright (c) 2009 Achim Settelmeier <lmms/at/m1.sirlab.de>
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

#ifndef _SETUP_DIALOG_MCL_H
#define _SETUP_DIALOG_MCL_H

#include <QtGui/QDialog>

#include "DummyMidiEventProcessor.h"
#include "piano.h"

class setupDialog;

class setupDialogMCL : public QDialog
{
	Q_OBJECT
public:
	setupDialogMCL( setupDialog * _parent );
	virtual ~setupDialogMCL();


protected slots:
	virtual void accept();

private slots:
	void clickedKeyBox();
	void clickedControllerBox();

private:
	setupDialog * m_parent;
	
	bool m_keysActive; // true: configure key, false: configure controller
	
	groupBox * m_actionKeyGroupBox;
	groupBox * m_actionControllerGroupBox;
	QComboBox * m_actionsKeyBox;
	QComboBox * m_actionsControllerBox;
	lcdSpinBoxModel * m_controllerSbModel;
	
	DummyMidiEventProcessor m_mep;
	Piano m_pianoModel;
} ;


#endif
