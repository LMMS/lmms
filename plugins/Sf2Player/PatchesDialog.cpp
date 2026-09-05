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
#include <QLabel>
#include <QKeyEvent>
#include <QStandardItem>

#include "embed.h"
#include "fluidsynthshims.h"

namespace lmms::gui
{

namespace
{
// Row numbers for each column in the table, as to reduce issues when reordering them.
static constexpr auto ColBank = 0;
static constexpr auto ColPatch = 1;
static constexpr auto ColName = 2;
static constexpr auto ColSearchSort = 3; //!< For an invisible column used in sorting
static constexpr auto TotalCols = 4;
}

PatchesDialog::PatchesDialog(QWidget* parent, Qt::WindowFlags wflags)
	: QDialog(parent, wflags)
	, m_pSynth{nullptr}
	, m_iChan{0}
	, m_iBank{0}
	, m_iProg{0}
	, m_showingAllBankPatches{false}
	, m_selProg{0}
	, m_selBank{0}
{
	// Setup UI struct...
	setupUi(this);

	// Configure bank list view
	auto bankHeader = m_bankListView->header();
	bankHeader->setSectionResizeMode(0, QHeaderView::Stretch);
	bankHeader->setStretchLastSection(true);
	bankHeader->resizeSection(0, 30);

	m_splitter->setStretchFactor(0, 2);
	m_splitter->setStretchFactor(1, 6);

	// FIXME: this looks awful.
	auto headerLabels = QList<QString>();
	headerLabels.reserve(TotalCols);
	for (int i = 0; i < TotalCols; i++) { headerLabels.append(QString{}); }
	headerLabels[ColName] = tr("Name");
	headerLabels[ColBank] = tr("Bank");
	headerLabels[ColPatch] = tr("Patch");
	headerLabels[ColSearchSort] = "HIDE THIS";
	m_progListSourceModel.setHorizontalHeaderLabels(headerLabels);

	// Configure program list models
	m_progListProxyModel.setSourceModel(&m_progListSourceModel);
	m_progListProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_progListProxyModel.setDynamicSortFilter(true);
	m_progListProxyModel.setFilterKeyColumn(ColName);

	// Configure program list view
	m_progListView->setModel(&m_progListProxyModel);
	m_progListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_progListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_progListView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_progListView->setSortingEnabled(true);
	m_progListView->sortByColumn(ColPatch, Qt::AscendingOrder);
	m_progListView->setColumnHidden(ColBank, true);
	m_progListView->setColumnHidden(ColSearchSort, true);

	constexpr int RowHeight = 18;
	auto progVHeader = m_progListView->verticalHeader();
	progVHeader->setSectionResizeMode(QHeaderView::Fixed);
	progVHeader->setMinimumSectionSize(RowHeight);
	progVHeader->setMaximumSectionSize(RowHeight);
	progVHeader->setDefaultSectionSize(RowHeight);
	progVHeader->hide();

	auto progHeader = m_progListView->horizontalHeader();
	progHeader->setDefaultAlignment(Qt::AlignLeft);
	progHeader->setSectionResizeMode(ColBank, QHeaderView::ResizeToContents);
	progHeader->setSectionResizeMode(ColPatch, QHeaderView::ResizeToContents);
	progHeader->setSectionResizeMode(ColName, QHeaderView::Stretch);
	progHeader->setSectionsMovable(false);
	progHeader->setStretchLastSection(true);

	m_bankListView->sortItems(ColPatch, Qt::AscendingOrder);

	// Configure search bar
	m_filterEdit->setPlaceholderText(tr("Search"));
	m_filterEdit->setClearButtonEnabled(true);
	m_filterEdit->addAction(embed::getIconPixmap("zoom"), QLineEdit::LeadingPosition);

	// Configure focus (only allow for search bar and dialog buttons)
	m_filterEdit->setFocus();
	m_filterEdit->setFocusPolicy(Qt::StrongFocus);
	m_bankListView->setFocusPolicy(Qt::NoFocus);
	m_progListView->setFocusPolicy(Qt::NoFocus);

	updateSearchUi(false);
	QObject::connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
		const auto hasQuery = !text.isEmpty();
		updateSearchUi(hasQuery);

		const auto regexp = QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);
		m_progListProxyModel.setFilterRegularExpression(regexp);

		// Fix the selection if it has been invalidated.
		diffSelectProgRow(0);
	});

	QObject::connect(m_bankListView, &QTreeWidget::currentItemChanged, this, &PatchesDialog::updatePatchList);
	QObject::connect(m_progListView, &QTableView::doubleClicked, this, &PatchesDialog::accept);
	QObject::connect(m_progListView->selectionModel(), &QItemSelectionModel::currentRowChanged,
		this, &PatchesDialog::progChanged);

	QObject::connect(m_okButton, &QPushButton::clicked, this, &PatchesDialog::accept);
	QObject::connect(m_cancelButton, &QPushButton::clicked, this, &PatchesDialog::reject);
}

void PatchesDialog::setup(fluid_synth_t* pSynth, int iChan, const QString& chanName,
	LcdSpinBoxModel* bankModel, LcdSpinBoxModel* progModel, QLabel* patchLabel)
{
	// We're going to change the whole thing...
	m_dirty = 0;
	m_bankModel = bankModel;
	m_progModel = progModel;
	m_patchLabel = patchLabel;

	// Set the proper caption
	setWindowTitle(tr("%1 - Soundfont patches").arg(chanName));

	// Set m_pSynth to null so we don't trigger any progChanged events
	m_pSynth = nullptr;

	// Load bank list from actual synth stack...
	m_bankListView->setSortingEnabled(false);
	m_bankListView->clear();

	// Now it should be safe to set internal stuff
	m_pSynth = pSynth;
	m_iChan = iChan;

	// For all soundfonts, in reversed stack order, fill the available banks
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts; i++)
	{
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont)
		{
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
			while ((pCurPreset = fluid_sfont_iteration_next_wrapper(pSoundFont, pCurPreset)))
			{
				int iBank = fluid_preset_get_banknum(pCurPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				if (!findBankItem(iBank))
				{
					auto* bankItem = new QTreeWidgetItem();
					bankItem->setData(0, Qt::DisplayRole, iBank);
					m_bankListView->addTopLevelItem(bankItem);
				}
			}
		}
	}
	m_bankListView->setSortingEnabled(true);

	// Set the selected bank.
	m_iBank = 0;
	fluid_preset_t *pPreset = ::fluid_synth_get_channel_preset(m_pSynth, m_iChan);
	if (pPreset)
	{
		m_iBank = fluid_preset_get_banknum(pPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
		m_iBank += ::fluid_synth_get_bank_offset(m_pSynth, fluid_sfont_get_id(fluid_preset_get_sfont(sfont)));
#endif
	}

	auto* bankItem = findBankItem(m_iBank);
	m_bankListView->setCurrentItem(bankItem);
	m_bankListView->scrollToItem(bankItem);
	updatePatchList();

	// Set the selected program.
	if (pPreset) { m_iProg = fluid_preset_get_num(pPreset); }

	if (auto* progItem = findProgItem(m_iProg); progItem != nullptr)
	{
		auto sourceIdx = progItem->index();
		auto proxyIdx = m_progListProxyModel.mapFromSource(sourceIdx);

		if (proxyIdx.isValid())
		{
			constexpr auto setMask = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;
			int row = proxyIdx.row();
			auto idx = m_progListView->model()->index(row, ColPatch);
			m_progListView->selectionModel()->setCurrentIndex(idx, setMask);
			m_progListView->scrollTo(idx);
		}
	}

	// Done with setup...
	// m_iDirtySetup--;
}

void PatchesDialog::stabilizeForm()
{
	m_okButton->setEnabled(validateForm());
}

bool PatchesDialog::validateForm()
{
	return m_bankListView->currentItem() != nullptr;
}

void PatchesDialog::setBankProg(int iBank, int iProg)
{
	if (m_pSynth == nullptr) { return; }

	// just select the synth's program preset...
	::fluid_synth_bank_select(m_pSynth, m_iChan, iBank);
	::fluid_synth_program_change(m_pSynth, m_iChan, iProg);
	// Maybe this is needed to stabilize things around.
	::fluid_synth_program_reset(m_pSynth);
}

// Validate form fields and accept it valid.
void PatchesDialog::accept()
{
	if (validateForm())
	{
		bool updateUi = m_dirty > 0;
		updatePatch(updateUi);

		// Do remember preview state...
		// if (m_pOptions)
			// m_pOptions->bPresetPreview = m_ui.PreviewCheckBox->isChecked();
		// We got it.

		QDialog::accept();
	}
}

void PatchesDialog::reject()
{
	// Reset selection to original selection, if applicable
	if (m_dirty > 0) { setBankProg(m_bankModel->value(), m_progModel->value()); }
	QDialog::reject();
}

QTreeWidgetItem* PatchesDialog::findBankItem(int iBank)
{
	auto banks = m_bankListView->findItems(QString::number(iBank), Qt::MatchExactly, ColBank);

	auto it = QListIterator<QTreeWidgetItem*>(banks);
	return it.hasNext() ? it.next() : nullptr;
}

QStandardItem* PatchesDialog::findProgItem(int iProg)
{
	auto progs = m_progListSourceModel.findItems(QString::number(iProg), Qt::MatchExactly, ColPatch);

	auto it = QListIterator<QStandardItem*>(progs);
	return it.hasNext() ? it.next() : nullptr;
}

void PatchesDialog::updatePatch(bool updateUi)
{
	setBankProg(m_selBank, m_selProg);

	if (updateUi)
	{
		m_bankModel->setValue(m_selBank);
		m_progModel->setValue(m_selProg);
		m_patchLabel->setText(m_selProgName);
	}
}

void PatchesDialog::progChanged(const QModelIndex& cur, const QModelIndex& prev)
{
	if (m_pSynth == nullptr) { return; }

	auto curRow = m_progListProxyModel.mapToSource(cur).row();
	if (curRow < 0) { return; }

	const auto modelAt = [this](int row, int col) -> QVariant
	{
		const auto idx = m_progListSourceModel.index(row, col);
		return m_progListSourceModel.data(idx);
	};

	m_selBank = modelAt(curRow, ColBank).toInt();
	m_selProg = modelAt(curRow, ColPatch).toInt();
	m_selProgName = modelAt(curRow, ColName).toString();

	if (validateForm())
	{
		updatePatch(false);

		// Now we're dirty nuff.
		m_dirty++;
	}

	stabilizeForm();
}

void PatchesDialog::keyPressEvent(QKeyEvent* event)
{
	const auto key = event->key();

	if (key == Qt::Key_Up || key == Qt::Key_Down)
	{
		event->accept();
		int rowDiff = (key == Qt::Key_Up) ? -1 : +1;
		diffSelectProgRow(rowDiff);
	}
	else if (key == Qt::Key_Return || key == Qt::Key_Enter)
	{
		event->accept();
		accept();
	}
	else if (key == Qt::Key_Escape)
	{
		event->accept();
		reject();
	}
}

void PatchesDialog::diffSelectProgRow(int offset)
{
	auto* selectionModel = m_progListView->selectionModel();

	const int curRow = selectionModel->currentIndex().row();
	int newRow = curRow + offset;
	const int rowCount = m_progListView->model()->rowCount();
	newRow = std::clamp(newRow, 0, std::max(0, rowCount - 1));

	constexpr auto selMask = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;
	const auto idx = m_progListView->model()->index(newRow, ColPatch);
	selectionModel->setCurrentIndex(idx, selMask);

	// NOTE: scrollTo() has to receive an index that points to a visible column. Be careful about that!
	m_progListView->scrollTo(idx);
}

void PatchesDialog::updateSearchUi(bool isSearching)
{
	// FIXME: every time the search starts or stops, the sort column is changed, violating what the user may
	// have picked. I'd say this doesn't matter most of the time (I doubt most would sort before searching)
	// but it might get annoying.
	m_progListView->sortByColumn(isSearching ? ColSearchSort : ColPatch, Qt::AscendingOrder);

	m_progListView->setColumnHidden(ColBank, !isSearching);
	showAllBankPatches(isSearching);
	m_bankListView->setHidden(isSearching);
}

void PatchesDialog::showAllBankPatches(bool value)
{
	// This check here avoids having to reload all patches for every character typed.
	if (value == m_showingAllBankPatches) { return; }
	m_showingAllBankPatches = value;
	updatePatchList();
}

void PatchesDialog::updatePatchList()
{
	//! Create a numeric value, allowing numerical sorting.
	const auto makeNumItem = [](int val) -> QStandardItem*
	{
		const auto ret = new QStandardItem();
		ret->setData(val, Qt::DisplayRole);
		return ret;
	};

	// Clear up the program list to refill
	m_progListSourceModel.setRowCount(0);

	// Attempt to get the selected bank.
	if (m_pSynth == nullptr) { return; }
	const auto* bankItem = m_bankListView->currentItem();
	if (bankItem == nullptr) { return; }

	const auto selectedBank = bankItem->text(0).toInt();

	// For all soundfonts (in reversed stack order) fill the available programs...
	bool stop = false; // replaces the `pProgItem` check that used to exist here
	int cSoundFonts = ::fluid_synth_sfcount(m_pSynth);
	for (int i = 0; i < cSoundFonts && !stop; i++)
	{
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont(m_pSynth, i);
		if (pSoundFont)
		{
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
			while ((pCurPreset = fluid_sfont_iteration_next_wrapper(pSoundFont, pCurPreset)))
			{
				int iBank = fluid_preset_get_banknum(pCurPreset);
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				int iProg = fluid_preset_get_num(pCurPreset);
				if (iBank == selectedBank || m_showingAllBankPatches)
				{
					const auto patchNumItem = makeNumItem(iProg);
					const auto bankNumItem = makeNumItem(iBank);
					const auto patchNameItem = new QStandardItem(fluid_preset_get_name(pCurPreset));
					const auto sortNumItem = makeNumItem(iBank * 1000 + iProg);

					// Old columns:
					// - QString::number(fluid_sfont_get_id(pSoundFont))
					// - QFileInfo(fluid_sfont_get_name(pSoundFont).baseName())

					m_progListSourceModel.appendRow({bankNumItem, patchNumItem, patchNameItem, sortNumItem});

					// If only showing the patches of a single bank, stop.
					if (!m_showingAllBankPatches) { stop = true; }
				}
			}
		}
	}

	stabilizeForm();
}

} // namespace lmms::gui
