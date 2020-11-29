/*
 * PatchesDialog.cpp - display GIG patches (based on Sf2 patches_dialog.cpp)
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


#include "PatchesDialog.h"

#include <QHeaderView>


// Custom list-view item (as for numerical sort purposes...)
class PatchItem : public QTreeWidgetItem
{
public:

	// Constructor.
	PatchItem( QTreeWidget *pListView,
		QTreeWidgetItem *pItemAfter )
		: QTreeWidgetItem( pListView, pItemAfter ) {}

	// Sort/compare overriden method.
	bool operator< ( const QTreeWidgetItem& other ) const
	{
		int iColumn = QTreeWidgetItem::treeWidget()->sortColumn();
		const QString& s1 = text( iColumn );
		const QString& s2 = other.text( iColumn );

		if( iColumn == 0 || iColumn == 2 )
		{
			return s1.toInt() < s2.toInt();
		}
		else
		{
			return s1 < s2;
		}
	}
};




// Constructor.
PatchesDialog::PatchesDialog( QWidget * pParent, Qt::WindowFlags wflags )
	: QDialog( pParent, wflags )
{
	// Setup UI struct...
	setupUi( this );

	m_pSynth = NULL;
	m_iChan  = 0;
	m_iBank  = 0;
	m_iProg  = 0;

	// Soundfonts list view...
	QHeaderView * pHeader = m_progListView->header();
	pHeader->setDefaultAlignment( Qt::AlignLeft );
	pHeader->setSectionsMovable( false );
	pHeader->setStretchLastSection( true );

	m_progListView->resizeColumnToContents( 0 );	// Prog.

	// Initial sort order...
	m_bankListView->sortItems( 0, Qt::AscendingOrder );
	m_progListView->sortItems( 0, Qt::AscendingOrder );

	// UI connections...
	QObject::connect( m_bankListView,
		SIGNAL( currentItemChanged( QTreeWidgetItem *,QTreeWidgetItem * ) ),
		SLOT( bankChanged() ) );
	QObject::connect( m_progListView,
		SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ),
		SLOT( progChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ) );
	QObject::connect( m_progListView,
		SIGNAL( itemActivated( QTreeWidgetItem *, int ) ),
		SLOT( accept() ) );
	QObject::connect( m_okButton,
		SIGNAL( clicked() ),
		SLOT( accept() ) );
	QObject::connect( m_cancelButton,
		SIGNAL( clicked() ),
		SLOT( reject() ) );
}




// Destructor.
PatchesDialog::~PatchesDialog()
{
}




// Dialog setup loader.
void PatchesDialog::setup( GigInstance * pSynth, int iChan,
						const QString & chanName,
						LcdSpinBoxModel * bankModel,
						LcdSpinBoxModel * progModel,
						QLabel * patchLabel )
{

	// We'll going to changes the whole thing...
	m_dirty = 0;
	m_bankModel = bankModel;
	m_progModel = progModel;
	m_patchLabel =  patchLabel;

	// Set the proper caption...
	setWindowTitle( chanName + " - GIG patches" );

	// set m_pSynth to NULL so we don't trigger any progChanged events
	m_pSynth = NULL;

	// Load bank list from actual synth stack...
	m_bankListView->setSortingEnabled( false );
	m_bankListView->clear();

	// now it should be safe to set internal stuff
	m_pSynth = pSynth;
	m_iChan  = iChan;


	//fluid_preset_t preset;
	QTreeWidgetItem * pBankItem = NULL;

	// Currently just use zero as the only bank
	int iBankDefault = -1;
	int iProgDefault = -1;

	gig::Instrument * pInstrument = m_pSynth->gig.GetFirstInstrument();

	while( pInstrument )
	{
		int iBank = pInstrument->MIDIBank;
		int iProg = pInstrument->MIDIProgram;

		if ( !findBankItem( iBank ) )
		{
			pBankItem = new PatchItem( m_bankListView, pBankItem );

			if( pBankItem )
			{
				pBankItem->setText( 0, QString::number( iBank ) );

				if( iBankDefault == -1 )
				{
					iBankDefault = iBank;
					iProgDefault = iProg;
				}
			}
		}

		pInstrument = m_pSynth->gig.GetNextInstrument();
	}

	m_bankListView->setSortingEnabled( true );

	// Set the selected bank.
	if( iBankDefault != -1 )
	{
		m_iBank = iBankDefault;
	}

	pBankItem = findBankItem( m_iBank );
	m_bankListView->setCurrentItem( pBankItem );
	m_bankListView->scrollToItem( pBankItem );
	bankChanged();

	// Set the selected program.
	if( iProgDefault != -1 )
	{
		m_iProg = iProgDefault;
	}

	QTreeWidgetItem * pProgItem = findProgItem( m_iProg );
	m_progListView->setCurrentItem( pProgItem );
	m_progListView->scrollToItem( pProgItem );
}




// Stabilize current state form.
void PatchesDialog::stabilizeForm()
{
	m_okButton->setEnabled( validateForm() );
}




// Validate form fields.
bool PatchesDialog::validateForm()
{
	bool bValid = true;

	bValid = bValid && ( m_bankListView->currentItem() != NULL );
	bValid = bValid && ( m_progListView->currentItem() != NULL );

	return bValid;
}




// Realize a bank-program selection preset.
void PatchesDialog::setBankProg( int iBank, int iProg )
{
	if( m_pSynth == NULL )
	{
		return;
	}
}




// Validate form fields and accept it valid.
void PatchesDialog::accept()
{
	if( validateForm() )
	{
		// Unload from current selected dialog items.
		int iBank = ( m_bankListView->currentItem() )->text( 0 ).toInt();
		int iProg = ( m_progListView->currentItem() )->text( 0 ).toInt();

		// And set it right away...
		setBankProg( iBank, iProg );

		if( m_dirty > 0 )
		{
			m_bankModel->setValue( iBank );
			m_progModel->setValue( iProg );
			m_patchLabel->setText( m_progListView->
						currentItem()->text( 1 ) );
		}

		// We got it.
		QDialog::accept();
	}
}




// Reject settings (Cancel button slot).
void PatchesDialog::reject()
{
	// Reset selection to initial selection, if applicable...
	if( m_dirty > 0 )
	{
		setBankProg( m_bankModel->value(), m_progModel->value() );
	}

	// Done (hopefully nothing).
	QDialog::reject();
}




// Find the bank item of given bank number id.
QTreeWidgetItem * PatchesDialog::findBankItem( int iBank )
{
	QList<QTreeWidgetItem *> banks
		= m_bankListView->findItems(
			QString::number( iBank ), Qt::MatchExactly, 0 );

	QListIterator<QTreeWidgetItem *> iter( banks );

	if( iter.hasNext() )
	{
		return iter.next();
	}
	else
	{
		return NULL;
	}
}




// Find the program item of given program number id.
QTreeWidgetItem *PatchesDialog::findProgItem( int iProg )
{
	QList<QTreeWidgetItem *> progs
		= m_progListView->findItems(
			QString::number( iProg ), Qt::MatchExactly, 0 );

	QListIterator<QTreeWidgetItem *> iter( progs );

	if( iter.hasNext() )
	{
		return iter.next();
	}
	else
	{
		return NULL;
	}
}




// Bank change slot.
void PatchesDialog::bankChanged()
{
	if( m_pSynth == NULL )
	{
		return;
	}

	QTreeWidgetItem * pBankItem = m_bankListView->currentItem();

	if( pBankItem == NULL )
	{
		return;
	}

	int iBankSelected = pBankItem->text( 0 ).toInt();

	// Clear up the program listview.
	m_progListView->setSortingEnabled( false );
	m_progListView->clear();
	QTreeWidgetItem * pProgItem = NULL;

	gig::Instrument * pInstrument = m_pSynth->gig.GetFirstInstrument();

	while( pInstrument )
	{
		QString name = QString::fromStdString( pInstrument->pInfo->Name );

		if( name == "" )
		{
			name = "<no name>";
		}

		int iBank = pInstrument->MIDIBank;
		int iProg = pInstrument->MIDIProgram;

		if( iBank == iBankSelected && !findProgItem( iProg ) )
		{
			pProgItem = new PatchItem( m_progListView, pProgItem );

			if( pProgItem )
			{
				pProgItem->setText( 0, QString::number( iProg ) );
				pProgItem->setText( 1, name );
			}
		}

		pInstrument = m_pSynth->gig.GetNextInstrument();
	}

	m_progListView->setSortingEnabled( true );

	// Stabilize the form.
	stabilizeForm();
}




// Program change slot.
void PatchesDialog::progChanged( QTreeWidgetItem * curr, QTreeWidgetItem * prev )
{
	if( m_pSynth == NULL || curr == NULL )
	{
		return;
	}

	// Which preview state...
	if( validateForm() )
	{
		// Set current selection.
		int iBank = ( m_bankListView->currentItem() )->text( 0 ).toInt();
		int iProg = curr->text( 0 ).toInt();

		// And set it right away...
		setBankProg( iBank, iProg );

		// Now we're dirty nuff.
		m_dirty++;
	}

	// Stabilize the form.
	stabilizeForm();
}
