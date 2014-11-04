/*
 * patches_dialog.h - display sf2 patches
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef _PATCHES_DIALOG_H
#define _PATCHES_DIALOG_H

#include "ui_patches_dialog.h"
#include "LcdSpinBox.h"

#include <fluidsynth.h>
#include <QtGui/QWidget>
#include <QtGui/QLabel>

//----------------------------------------------------------------------------
// qsynthPresetForm -- UI wrapper form.

class patchesDialog : public QDialog, private Ui::patchesDialog
{
	Q_OBJECT

public:

	// Constructor.
	patchesDialog(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Destructor.
	virtual ~patchesDialog();


	void setup(fluid_synth_t *pSynth, int iChan, const QString & _chanName,
			LcdSpinBoxModel * _bankModel, LcdSpinBoxModel * _progModel, QLabel *_patchLabel );

public slots:

	void stabilizeForm();
	void bankChanged();
	void progChanged( QTreeWidgetItem * _curr, QTreeWidgetItem * _prev );

protected slots:

	void accept();
	void reject();

protected:

	void setBankProg(int iBank, int iProg);

	QTreeWidgetItem *findBankItem(int iBank);
	QTreeWidgetItem *findProgItem(int iProg);

	bool validateForm();

private:

	// Instance variables.
	fluid_synth_t *m_pSynth;

	int m_iChan;
	int m_iBank;
	int m_iProg;

	//int m_iDirtySetup;
	//int m_iDirtyCount;
	int m_dirty;

	LcdSpinBoxModel * m_bankModel;
	LcdSpinBoxModel * m_progModel;
	QLabel *m_patchLabel;
};


#endif

