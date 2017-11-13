/*
 * PatchesDialog.h - display GIG patches (based on Sf2 patches_dialog.h)
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#ifndef PATCHES_DIALOG_H
#define PATCHES_DIALOG_H

#include "ui_PatchesDialog.h"
#include "LcdSpinBox.h"
#include "GigPlayer.h"

#include <QWidget>
#include <QLabel>

//----------------------------------------------------------------------------
// qsynthPresetForm -- UI wrapper form.

class PatchesDialog : public QDialog, private Ui::PatchesDialog
{
	Q_OBJECT

public:

	// Constructor.
	PatchesDialog( QWidget * pParent = 0, Qt::WindowFlags wflags = 0 );

	// Destructor.
	virtual ~PatchesDialog();


	void setup( GigInstance * pSynth, int iChan, const QString & chanName,
			LcdSpinBoxModel * bankModel, LcdSpinBoxModel * progModel, QLabel * patchLabel );

public slots:

	void stabilizeForm();
	void bankChanged();
	void progChanged( QTreeWidgetItem * curr, QTreeWidgetItem * prev );

protected slots:

	void accept();
	void reject();

protected:

	void setBankProg( int iBank, int iProg );

	QTreeWidgetItem * findBankItem( int iBank );
	QTreeWidgetItem * findProgItem( int iProg );

	bool validateForm();

private:

	// Instance variables.
	GigInstance * m_pSynth;

	int m_iChan;
	int m_iBank;
	int m_iProg;

	int m_dirty;

	LcdSpinBoxModel * m_bankModel;
	LcdSpinBoxModel * m_progModel;
	QLabel * m_patchLabel;
};

#endif
