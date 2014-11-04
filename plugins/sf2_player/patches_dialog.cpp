/*
 * patches_dialog.cpp - display sf2 patches
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


#include "patches_dialog.h"

#include <QtGui/QHeaderView>
//#include <QFileInfo>


// Custom list-view item (as for numerical sort purposes...)
class patchItem : public QTreeWidgetItem
{
public:

	// Constructor.
	patchItem( QTreeWidget *pListView,
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
			return( s1.toInt() < s2.toInt() );
		} 
		else 
		{
			return( s1 < s2 );
		}
	}
};



// Constructor.
patchesDialog::patchesDialog( QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog( pParent, wflags )
{
	// Setup UI struct...
	setupUi( this );

	m_pSynth = NULL;
	m_iChan  = 0;
	m_iBank  = 0;
	m_iProg  = 0;

	// Soundfonts list view...
	QHeaderView *pHeader = m_progListView->header();
//	pHeader->setResizeMode(QHeaderView::Custom);
	pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(200);
	pHeader->setMovable(false);
	pHeader->setStretchLastSection(true);

	m_progListView->resizeColumnToContents(0);	// Prog.
	//pHeader->resizeSection(1, 200);					// Name.

	// Initial sort order...
	m_bankListView->sortItems(0, Qt::AscendingOrder);
	m_progListView->sortItems(0, Qt::AscendingOrder);

	// UI connections...
	QObject::connect(m_bankListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(bankChanged()));
	QObject::connect(m_progListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(progChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
	QObject::connect(m_progListView,
		SIGNAL(itemActivated(QTreeWidgetItem*,int)),
		SLOT(accept()));
	QObject::connect(m_okButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_cancelButton,
		SIGNAL(clicked()),
		SLOT(reject()));
}


// Destructor.
patchesDialog::~patchesDialog()
{
}


// Dialog setup loader.
void patchesDialog::setup ( fluid_synth_t * pSynth, int iChan,
						const QString & _chanName,
						LcdSpinBoxModel * _bankModel,
						LcdSpinBoxModel * _progModel,
							QLabel * _patchLabel )
{

	// We'll going to changes the whole thing...
	m_dirty = 0;
	m_bankModel = _bankModel;
	m_progModel = _progModel;
	m_patchLabel =  _patchLabel;

	// Set the proper caption...
	setWindowTitle( _chanName + " - Soundfont patches" );

	// set m_pSynth to NULL so we don't trigger any progChanged events
	m_pSynth = NULL;

	// Load bank list from actual synth stack...
	m_bankListView->setSortingEnabled(false);
	m_bankListView->clear();

	// now it should be safe to set internal stuff
	m_pSynth = pSynth;
	m_iChan  = iChan;


	fluid_preset_t preset;
	QTreeWidgetItem *pBankItem = NULL;
	// For all soundfonts (in reversed stack order) fill the available banks...
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts; i++) {
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont) {
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOffset = ::fluid_synth_get_bank_offset(m_pSynth, pSoundFont->id);
#endif
			pSoundFont->iteration_start(pSoundFont);
			while (pSoundFont->iteration_next(pSoundFont, &preset)) {
				int iBank = preset.get_banknum(&preset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				if (!findBankItem(iBank)) {
					pBankItem = new patchItem(m_bankListView, pBankItem);
					if (pBankItem)
						pBankItem->setText(0, QString::number(iBank));
				}
			}
		}
	}
	m_bankListView->setSortingEnabled(true);

	// Set the selected bank.
	m_iBank = 0;
	fluid_preset_t *pPreset = ::fluid_synth_get_channel_preset(m_pSynth, m_iChan);
	if (pPreset) {
		m_iBank = pPreset->get_banknum(pPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
		m_iBank += ::fluid_synth_get_bank_offset(m_pSynth, (pPreset->sfont)->id);
#endif
	}

	pBankItem = findBankItem(m_iBank);
	m_bankListView->setCurrentItem(pBankItem);
	m_bankListView->scrollToItem(pBankItem);
	bankChanged();

	// Set the selected program.
	if (pPreset)
		m_iProg = pPreset->get_num(pPreset);
	QTreeWidgetItem *pProgItem = findProgItem(m_iProg);
	m_progListView->setCurrentItem(pProgItem);
	m_progListView->scrollToItem(pProgItem);

	// Done with setup...
	//m_iDirtySetup--;
}


// Stabilize current state form.
void patchesDialog::stabilizeForm()
{
	m_okButton->setEnabled(validateForm());
}


// Validate form fields.
bool patchesDialog::validateForm()
{
	bool bValid = true;

	bValid = bValid && (m_bankListView->currentItem() != NULL);
	bValid = bValid && (m_progListView->currentItem() != NULL);

	return bValid;
}


// Realize a bank-program selection preset.
void patchesDialog::setBankProg ( int iBank, int iProg )
{
	if (m_pSynth == NULL)
		return;

	// just select the synth's program preset...
	::fluid_synth_bank_select(m_pSynth, m_iChan, iBank);
	::fluid_synth_program_change(m_pSynth, m_iChan, iProg);
	// Maybe this is needed to stabilize things around.
	::fluid_synth_program_reset(m_pSynth);
}


// Validate form fields and accept it valid.
void patchesDialog::accept()
{
	if (validateForm()) {
		// Unload from current selected dialog items.
		int iBank = (m_bankListView->currentItem())->text(0).toInt();
		int iProg = (m_progListView->currentItem())->text(0).toInt();
		// And set it right away...
		setBankProg(iBank, iProg);
		
		if (m_dirty > 0) {
			m_bankModel->setValue( iBank );
			m_progModel->setValue( iProg );
			m_patchLabel->setText( m_progListView->
						currentItem()->text( 1 ) );
		}

		// Do remember preview state...
		// if (m_pOptions)
			// m_pOptions->bPresetPreview = m_ui.PreviewCheckBox->isChecked();
		// We got it.
		QDialog::accept();
	}
}


// Reject settings (Cancel button slot).
void patchesDialog::reject (void)
{
	// Reset selection to initial selection, if applicable...
	if (m_dirty > 0)
		setBankProg(m_bankModel->value(), m_progModel->value());
	// Done (hopefully nothing).
	QDialog::reject();
}


// Find the bank item of given bank number id.
QTreeWidgetItem *patchesDialog::findBankItem ( int iBank )
{
	QList<QTreeWidgetItem *> banks
		= m_bankListView->findItems(
			QString::number(iBank), Qt::MatchExactly, 0);

	QListIterator<QTreeWidgetItem *> iter(banks);
	if (iter.hasNext())
		return iter.next();
	else
		return NULL;
}


// Find the program item of given program number id.
QTreeWidgetItem *patchesDialog::findProgItem ( int iProg )
{
	QList<QTreeWidgetItem *> progs
		= m_progListView->findItems(
			QString::number(iProg), Qt::MatchExactly, 0);

	QListIterator<QTreeWidgetItem *> iter(progs);
	if (iter.hasNext())
		return iter.next();
	else
		return NULL;
}



// Bank change slot.
void patchesDialog::bankChanged (void)
{
	if (m_pSynth == NULL)
		return;

	QTreeWidgetItem *pBankItem = m_bankListView->currentItem();
	if (pBankItem == NULL)
		return;

	int iBankSelected = pBankItem->text(0).toInt();

	// Clear up the program listview.
	m_progListView->setSortingEnabled(false);
	m_progListView->clear();
	fluid_preset_t preset;
	QTreeWidgetItem *pProgItem = NULL;
	// For all soundfonts (in reversed stack order) fill the available programs...
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts && !pProgItem; i++) {
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont) {
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOffset = ::fluid_synth_get_bank_offset(m_pSynth, pSoundFont->id);
#endif
			pSoundFont->iteration_start(pSoundFont);
			while (pSoundFont->iteration_next(pSoundFont, &preset)) {
				int iBank = preset.get_banknum(&preset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				int iProg = preset.get_num(&preset);
				if (iBank == iBankSelected && !findProgItem(iProg)) {
					pProgItem = new patchItem(m_progListView, pProgItem);
					if (pProgItem) {
						pProgItem->setText(0, QString::number(iProg));
						pProgItem->setText(1, preset.get_name(&preset));
						//pProgItem->setText(2, QString::number(pSoundFont->id));
						//pProgItem->setText(3, QFileInfo(
						//	pSoundFont->get_name(pSoundFont)).baseName());
					}
				}
			}
		}
	}
	m_progListView->setSortingEnabled(true);

	// Stabilize the form.
	stabilizeForm();
}


// Program change slot.
void patchesDialog::progChanged (QTreeWidgetItem * _curr, QTreeWidgetItem * _prev)
{
	if (m_pSynth == NULL || _curr == NULL)
		return;

	// Which preview state...
	if( validateForm() ) {
		// Set current selection.
		int iBank = (m_bankListView->currentItem())->text(0).toInt();
		int iProg = _curr->text(0).toInt();
		// And set it right away...
		setBankProg(iBank, iProg);
		// Now we're dirty nuff.
		m_dirty++;
	}

	// Stabilize the form.
	stabilizeForm();
}


#include "moc_patches_dialog.cxx"
