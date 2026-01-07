/*
 * PatchesDialog.cpp - display sf2 patches
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

#include <fluidsynth.h>
#include <QHeaderView>
//#include <QFileInfo>
#include <QLabel>
#include <QKeyEvent>
#include <QDebug> // TODO: remove
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStandardItem>

#include "embed.h"
#include "fluidsynthshims.h"

namespace lmms::gui
{


// Custom list-view item (as for numerical sort purposes...)
class PatchItem : public QTreeWidgetItem
{
public:

	// Constructor.
	PatchItem( QTreeWidget *pListView,
		QTreeWidgetItem *pItemAfter )
		: QTreeWidgetItem( pListView, pItemAfter ) {}

	// Sort/compare overriden method.
	bool operator< ( const QTreeWidgetItem& other ) const override
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

class NumericItem : public QStandardItem {
public:
	NumericItem(QString x) : QStandardItem(x) {}

	bool operator<(const QStandardItem &other) const
	{
		return text().toInt() < other.text().toInt();
	}
};

// TODO: sorting via the column button is messed up!! it doesn't take into account the NumericItem class, for some reason

// Constructor.
PatchesDialog::PatchesDialog( QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog( pParent, wflags ), m_progListSourceModel{}, m_progListProxyModel{}
{
	// Setup UI struct...
	setupUi( this );

	m_pSynth = nullptr;
	m_iChan  = 0;
	m_iBank  = 0;
	m_iProg  = 0;

	// Soundfonts list view...
	// QHeaderView *pHeader = m_progListView->header();
//	pHeader->setResizeMode(QHeaderView::Custom);
	// pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(200);
	// pHeader->setSectionsMovable(false);
	// pHeader->setStretchLastSection(true);

	m_progListSourceModel.setHorizontalHeaderLabels({tr("Patch"), tr("Name")});

	// Configure program list models
	m_progListProxyModel.setSourceModel(&m_progListSourceModel);
	m_progListProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_progListProxyModel.setFilterKeyColumn(1); // "Name" column
	m_progListProxyModel.setDynamicSortFilter(true);

	// Configure program list view
	m_progListView->setModel(&m_progListProxyModel);
	// m_progListView->resizeColumnToContents(0);  // Prog.
	// pHeader->resizeSection(1, 200);          // Name.
	m_progListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_progListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_progListView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_progListView->setSortingEnabled(true);
	m_progListView->sortByColumn(0, Qt::AscendingOrder); // Initial sort by column 0 (Name)
	m_progListView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_progListView->verticalHeader()->hide();

	auto pHeader = m_progListView->horizontalHeader();
	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	pHeader->setSectionResizeMode(1, QHeaderView::Stretch);
	pHeader->setSectionsMovable(false);
	pHeader->setStretchLastSection(true);

	// Initial sort order...
	m_bankListView->sortItems(0, Qt::AscendingOrder);
	// m_progListView->sortItems(0, Qt::AscendingOrder); // TODO: how to do this

	m_filterEdit->setPlaceholderText(tr("Search"));
	m_filterEdit->setClearButtonEnabled(true);
	m_filterEdit->addAction(embed::getIconPixmap("zoom"), QLineEdit::LeadingPosition);

	// Configure focus (only allow for search bar and dialog buttons)
	m_filterEdit->setFocus();
	m_filterEdit->setFocusPolicy(Qt::StrongFocus);
	m_bankListView->setFocusPolicy(Qt::NoFocus);
	m_progListView->setFocusPolicy(Qt::NoFocus);

	// UI connections...
	QObject::connect(m_bankListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(bankChanged()));
	QObject::connect(m_progListView,
		&QTableView::doubleClicked, this, &PatchesDialog::accept);
	QObject::connect(m_progListView->selectionModel(),
		&QItemSelectionModel::currentRowChanged, this,
		[this](auto& cur, auto& prev) { progChanged(cur, prev); });
	// QObject::connect(m_progListView,
	// 	SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	// 	SLOT(progChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
	// QObject::connect(m_progListView,
	// 	SIGNAL(itemActivated(QTreeWidgetItem*,int)),
	// 	SLOT(accept()));
	QObject::connect(m_okButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_cancelButton,
		SIGNAL(clicked()),
		SLOT(reject()));
	QObject::connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
		m_progListProxyModel.setFilterRegularExpression(
			QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
		diffSelectRow(0); // just check if everything is fine
	});

	installEventFilter(this);
}


// Dialog setup loader.
void PatchesDialog::setup ( fluid_synth_t * pSynth, int iChan,
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
	m_pSynth = nullptr;

	// Load bank list from actual synth stack...
	m_bankListView->setSortingEnabled(false);
	m_bankListView->clear();

	// now it should be safe to set internal stuff
	m_pSynth = pSynth;
	m_iChan  = iChan;

	QTreeWidgetItem *pBankItem = nullptr;
	// For all soundfonts (in reversed stack order) fill the available banks...
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts; i++) {
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont) {
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOffset = ::fluid_synth_get_bank_offset(m_pSynth, fluid_sfont_get_id(pSoundFont));
#endif
			fluid_sfont_iteration_start(pSoundFont);
#if FLUIDSYNTH_VERSION_MAJOR < 2
			fluid_preset_t preset;
			fluid_preset_t *pCurPreset = &preset;
#else
			fluid_preset_t *pCurPreset = nullptr;
#endif
			while ((pCurPreset = fluid_sfont_iteration_next_wrapper(pSoundFont, pCurPreset))) {
				int iBank = fluid_preset_get_banknum(pCurPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				if (!findBankItem(iBank)) {
					pBankItem = new PatchItem(m_bankListView, pBankItem);
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
		m_iBank = fluid_preset_get_banknum(pPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
		m_iBank += ::fluid_synth_get_bank_offset(m_pSynth, fluid_sfont_get_id(fluid_preset_get_sfont(sfont)));
#endif
	}

	pBankItem = findBankItem(m_iBank);
	m_bankListView->setCurrentItem(pBankItem);
	m_bankListView->scrollToItem(pBankItem);
	bankChanged();

	// Set the selected program.
	if (pPreset)
		m_iProg = fluid_preset_get_num(pPreset);

	// TODO: make this work.....
	QStandardItem *progItem = findProgItem(m_iProg);
	if (progItem != nullptr)
	{
		QModelIndex idx = progItem->index();
		m_progListView->selectionModel()->select(idx, QItemSelectionModel::Rows);
		m_progListView->scrollTo(idx);
	}
	// m_progListModel->setCurrentItem(pProgItem); // TODO: ACCOUNT
	// m_progListView->scrollToItem(pProgItem); // TODO: ACCOUNT

	// Done with setup...
	//m_iDirtySetup--;
}


// Stabilize current state form.
void PatchesDialog::stabilizeForm()
{
	m_okButton->setEnabled(validateForm());
}


// Validate form fields.
bool PatchesDialog::validateForm()
{
	bool bValid = true;

	bValid = bValid && (m_bankListView->currentItem() != nullptr);
	bValid = bValid && (m_selProg != -1);

	return bValid;
}


// Realize a bank-program selection preset.
void PatchesDialog::setBankProg ( int iBank, int iProg )
{
	if (m_pSynth == nullptr)
		return;

	// just select the synth's program preset...
	::fluid_synth_bank_select(m_pSynth, m_iChan, iBank);
	::fluid_synth_program_change(m_pSynth, m_iChan, iProg);
	// Maybe this is needed to stabilize things around.
	::fluid_synth_program_reset(m_pSynth);
}


// Validate form fields and accept it valid.
void PatchesDialog::accept()
{
	if (validateForm()) {
		bool updateUi = m_dirty > 0;
		updatePatch(updateUi);

		// Do remember preview state...
		// if (m_pOptions)
			// m_pOptions->bPresetPreview = m_ui.PreviewCheckBox->isChecked();
		// We got it.

		QDialog::accept();
	}
}


// Reject settings (Cancel button slot).
void PatchesDialog::reject ()
{
	// Reset selection to initial selection, if applicable...
	if (m_dirty > 0)
		setBankProg(m_bankModel->value(), m_progModel->value());
	// Done (hopefully nothing).
	QDialog::reject();
}


// Find the bank item of given bank number id.
QTreeWidgetItem *PatchesDialog::findBankItem ( int iBank )
{
	QList<QTreeWidgetItem *> banks
		= m_bankListView->findItems(
			QString::number(iBank), Qt::MatchExactly, 0);

	QListIterator<QTreeWidgetItem *> iter(banks);
	if (iter.hasNext())
		return iter.next();
	else
		return nullptr;
}


// Find the program item of given program number id.
QStandardItem *PatchesDialog::findProgItem ( int iProg )
{
	QList<QStandardItem *> progs = m_progListSourceModel.findItems(
		QString::number(iProg), Qt::MatchExactly, 0);

	QListIterator<QStandardItem *> iter(progs);
	if (iter.hasNext())
		return iter.next();
	else
		return nullptr;
}


// Bank change slot.
void PatchesDialog::bankChanged ()
{
	if (m_pSynth == nullptr)
		return;

	QTreeWidgetItem *pBankItem = m_bankListView->currentItem();
	if (pBankItem == nullptr)
		return;

	int iBankSelected = pBankItem->text(0).toInt();

	// Clear up the program list
	m_progListView->setSortingEnabled(false);
	m_progListSourceModel.setRowCount(0);
	QTreeWidgetItem *pProgItem = nullptr;
	// For all soundfonts (in reversed stack order) fill the available programs...
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts && !pProgItem; i++) {
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont) {
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOffset = ::fluid_synth_get_bank_offset(m_pSynth, fluid_sfont_get_id(pSoundFont));
#endif
			fluid_sfont_iteration_start(pSoundFont);
#if FLUIDSYNTH_VERSION_MAJOR < 2
			fluid_preset_t preset;
			fluid_preset_t *pCurPreset = &preset;
#else
			fluid_preset_t *pCurPreset = nullptr;
#endif
			int row = 0;
			while ((pCurPreset = fluid_sfont_iteration_next_wrapper(pSoundFont, pCurPreset))) {
				int iBank = fluid_preset_get_banknum(pCurPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				int iProg = fluid_preset_get_num(pCurPreset);
				if (iBank == iBankSelected && !findProgItem(iProg)) {
					// FIXME: use PatchItem here? It makes sense, but I'm not fully sure how to use it here

					m_progListSourceModel.setItem(row, 0, new NumericItem(QString::number(iProg)));
					m_progListSourceModel.setItem(row, 1, new QStandardItem(fluid_preset_get_name(pCurPreset)));
					row++;

					// pProgItem = new PatchItem(&m_progListSourceModel, pProgItem);
					// if (pProgItem) {
						//pProgItem->setText(2, QString::number(fluid_sfont_get_id(pSoundFont)));
						//pProgItem->setText(3, QFileInfo(
						//	fluid_sfont_get_name(pSoundFont).baseName());
					// }
				}
			}
		}
	}
	m_progListView->setSortingEnabled(true);
	m_progListSourceModel.sort(0); // sort by patch column

	// Stabilize the form.
	stabilizeForm();
}

void PatchesDialog::updatePatch(bool updateUi)
{
	if (m_selProg < 0)
		return;

	// Get bank value
	int iBank = m_bankListView->currentItem()->text(0).toInt();

	// And set it right away...
	setBankProg(iBank, m_selProg);

	if (updateUi)
	{
		m_bankModel->setValue(iBank);
		m_progModel->setValue(m_selProg);
		m_patchLabel->setText(m_selProgName);
	}
}

// Program change slot.
void PatchesDialog::progChanged(const QModelIndex& cur, const QModelIndex& prev)
{
	if (m_pSynth == nullptr)
		return;

	auto curRow = m_progListProxyModel.mapToSource(cur).row();
	if (curRow < 0)
		return;

	auto progIdx = m_progListSourceModel.index(curRow, 0);
	m_selProg = m_progListSourceModel.data(progIdx).toInt();

	auto nameIdx = m_progListSourceModel.index(curRow, 1);
	m_selProgName = m_progListSourceModel.data(nameIdx).toString();

	// Which preview state...
	if (validateForm())
	{
		updatePatch(false);

		// Now we're dirty nuff.
		m_dirty++;
	}

	// Stabilize the form.
	stabilizeForm();
}

bool PatchesDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == this && event->type() == QEvent::KeyPress)
	{
		auto key = static_cast<QKeyEvent*>(event)->key();
		if (key == Qt::Key_Up || key == Qt::Key_Down)
		{
			int rowDiff = (key == Qt::Key_Up) ? -1 : +1;
			diffSelectRow(rowDiff);
			return true;
		}
	}

	return QDialog::eventFilter(obj, event);
}

void PatchesDialog::diffSelectRow(int offset)
{
	QItemSelectionModel* selectionModel = m_progListView->selectionModel();

	int curRow = selectionModel->currentIndex().row();
	int newRow = curRow + offset;
	int rowCount = m_progListView->model()->rowCount();
	newRow = qBound(0, newRow, rowCount - 1);
	
	selectionModel->setCurrentIndex(m_progListView->model()->index(newRow, 0),
		QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	m_progListView->scrollTo(m_progListView->model()->index(newRow, 0));
}

} // namespace lmms::gui
