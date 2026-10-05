/*
 * EffectSelectDialog.cpp - dialog to choose effect plugin
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail.com>
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

#include "EffectSelectDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableView>
#include <QVBoxLayout>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qdebug.h>
#include <qglobal.h>
#include <qicon.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qstandarditemmodel.h>

#include "DummyEffect.h"
#include "Effect.h"
#include "EffectCategory.h"
#include "EffectChain.h"
#include "PluginFactory.h"
#include "embed.h"

namespace lmms::gui {

EffectSelectDialog::EffectSelectDialog(QWidget* parent)
	: QDialog(parent)
	, m_effectKeys()
	, m_currentSelection()
	, m_sourceModel()
	, m_model()
	, m_descriptionWidget(nullptr)
	, m_pluginList(new QTableView(this))
	, m_scrollArea(new QScrollArea(this))
{
	setWindowTitle(tr("Add effect"));
	resize(640, 480);

	setWindowIcon(embed::getIconPixmap("setup_audio"));

	// Query effects
	EffectKeyList subPluginEffectKeys;
	QIcon favoriteIcon = QIcon(embed::getIconPixmap("star"));
	for (const auto desc : getPluginFactory()->descriptors(Plugin::Type::Effect))
	{
		if (desc->subPluginFeatures) { desc->subPluginFeatures->listSubPluginKeys(desc, subPluginEffectKeys); }
		else
		{
			m_effectKeys << EffectKey(desc, desc->name);
		}
	}
	m_effectKeys += subPluginEffectKeys;

	// Fill the source model
	m_sourceModel.setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
	m_sourceModel.setHorizontalHeaderItem(1, new QStandardItem(tr("Category")));
	m_sourceModel.setHorizontalHeaderItem(2, new QStandardItem(tr("Type")));
	m_sourceModel.setHorizontalHeaderItem(3, new QStandardItem(tr("Favorites")));
	int row = 0;
	for (EffectKeyList::ConstIterator it = m_effectKeys.begin(); it != m_effectKeys.end(); ++it)
	{
		QString type;
		QString name;
		if (it->desc->subPluginFeatures)
		{
			name = it->displayName();
			type = it->desc->displayName;
		}
		else
		{
			name = it->desc->displayName;
			type = "LMMS";
		}
		m_sourceModel.setItem(row, 0, new QStandardItem(name));
		m_sourceModel.setItem(row, 1, new QStandardItem(getEffectCategory()->getCategoryName(name)));
		m_sourceModel.setItem(row, 2, new QStandardItem(type));
		auto* favoriteItem = new QStandardItem();
		connect(this, &EffectSelectDialog::refreshData, this, [this, name, favoriteIcon, favoriteItem](QString nameToRefresh){
			if(QString::compare(name, nameToRefresh) == 0)
			{
				qDebug() << "refreshing favorite for " + name;
				favoriteItem->setData(getEffectCategory()->getIsFavorite(name) ? favoriteIcon : QIcon());
				QModelIndex topLeft = m_sourceModel.index(0, 0);
				QModelIndex bottomRight = m_sourceModel.index(m_sourceModel.rowCount() - 1, 3);
				emit m_sourceModel.dataChanged(topLeft, bottomRight, {Qt::DecorationRole});
			}
		});
		if (getEffectCategory()->getIsFavorite(name)) { favoriteItem->setIcon(favoriteIcon); }
		m_sourceModel.setItem(row, 3, favoriteItem);
		++row;
	}

	// Setup filtering
	m_model.setSourceModel(&m_sourceModel);
	m_model.setFilterCaseSensitivity(Qt::CaseInsensitive);

	auto* mainLayout = new QHBoxLayout(this);

	m_filterEdit = new QLineEdit(this);
	connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
		m_model.setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
	});
	connect(m_filterEdit, &QLineEdit::textChanged, this, &EffectSelectDialog::updateSelection);
	m_filterEdit->setFocus();
	m_filterEdit->setFocusPolicy(Qt::StrongFocus);
	m_filterEdit->setPlaceholderText(tr("Search"));
	m_filterEdit->setClearButtonEnabled(true);
	m_filterEdit->addAction(embed::getIconPixmap("zoom"), QLineEdit::LeadingPosition);

	m_pluginList->setModel(&m_model);
	m_pluginList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pluginList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pluginList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pluginList->setSortingEnabled(true);
	m_pluginList->sortByColumn(0, Qt::AscendingOrder); // Initial sort by column 0 (Name)
	m_pluginList->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_pluginList->verticalHeader()->hide();
	m_pluginList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_pluginList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_pluginList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_pluginList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_pluginList->setFocusPolicy(Qt::NoFocus);
	m_pluginList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pluginList, &QTableView::customContextMenuRequested, this, &EffectSelectDialog::showContextMenu);

	// Scroll Area
	m_scrollArea->setWidgetResizable(true);
	auto* scrollAreaWidgetContents = new QWidget(m_scrollArea);
	scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
	m_scrollArea->setWidget(scrollAreaWidgetContents);
	m_scrollArea->setMaximumHeight(180);
	m_scrollArea->setFocusPolicy(Qt::NoFocus);

	// Button Box
	auto* buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	buttonBox->setFocusPolicy(Qt::NoFocus);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &EffectSelectDialog::acceptSelection);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &EffectSelectDialog::reject);

	auto* rightSectionLayout = new QVBoxLayout();
	rightSectionLayout->addWidget(m_filterEdit);
	rightSectionLayout->addLayout(buildFiltersLayout());
	rightSectionLayout->addWidget(m_pluginList);
	rightSectionLayout->addWidget(m_scrollArea);
	rightSectionLayout->addWidget(buttonBox);
	mainLayout->addLayout(rightSectionLayout);

	setLayout(mainLayout);

	auto selectionModel = new QItemSelectionModel(&m_model);
	m_pluginList->setSelectionModel(selectionModel);
	connect(selectionModel, &QItemSelectionModel::currentRowChanged, this, &EffectSelectDialog::rowChanged);

	connect(m_pluginList, &QTableView::doubleClicked, this, &EffectSelectDialog::acceptSelection);

	setModal(true);
	installEventFilter(this);

	updateSelection();
	show();
}

Effect* EffectSelectDialog::instantiateSelectedPlugin(EffectChain* parent)
{
	Effect* result = nullptr;
	if (!m_currentSelection.name.isEmpty() && m_currentSelection.desc)
	{
		result = Effect::instantiate(m_currentSelection.desc->name, parent, &m_currentSelection);
	}
	if (!result) { result = new DummyEffect(parent, QDomElement()); }
	return result;
}

void EffectSelectDialog::acceptSelection()
{
	if (m_currentSelection.isValid()) { accept(); }
}

void EffectSelectDialog::rowChanged(const QModelIndex& idx, const QModelIndex&)
{
	delete m_descriptionWidget;
	m_descriptionWidget = nullptr;

	if (m_model.mapToSource(idx).row() < 0)
	{
		// Invalidate current selection
		m_currentSelection = Plugin::Descriptor::SubPluginFeatures::Key();
	}
	else
	{
		m_currentSelection = m_effectKeys[m_model.mapToSource(idx).row()];
	}
	if (m_currentSelection.desc)
	{
		m_descriptionWidget = new QWidget;

		auto hbox = new QHBoxLayout(m_descriptionWidget);
		hbox->setAlignment(Qt::AlignTop);

		Plugin::Descriptor const& descriptor = *(m_currentSelection.desc);

		const PixmapLoader* pixLoa = m_currentSelection.logo();
		if (pixLoa)
		{
			auto logoLabel = new QLabel(m_descriptionWidget);
			logoLabel->setPixmap(pixLoa->pixmap());
			logoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			logoLabel->setMaximumSize(64, 64);

			hbox->addWidget(logoLabel);
		}

		auto textualInfoWidget = new QWidget(m_descriptionWidget);
		textualInfoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		hbox->addWidget(textualInfoWidget);

		auto textWidgetLayout = new QVBoxLayout(textualInfoWidget);
		textWidgetLayout->setContentsMargins(4, 4, 4, 4);
		textWidgetLayout->setSpacing(8);

		if (m_currentSelection.desc->subPluginFeatures)
		{
			auto subWidget = new QWidget(textualInfoWidget);
			auto subLayout = new QVBoxLayout(subWidget);
			subLayout->setContentsMargins(0, 0, 0, 0);
			subLayout->setSpacing(8);

			m_currentSelection.desc->subPluginFeatures->fillDescriptionWidget(subWidget, &m_currentSelection);
			for (QWidget* w : subWidget->findChildren<QWidget*>())
			{
				if (w->parent() == subWidget)
				{
					subLayout->addWidget(w);
					subLayout->setAlignment(w, QFlags<Qt::AlignmentFlag>(Qt::AlignTop | Qt::AlignLeft));
				}
			}
			textWidgetLayout->addWidget(subWidget);
		}
		else
		{
			// HACK: Markup inside translation strings due to RTL not being handled correctly.
			// Move the markup out of the translation strings and into the QString template as soon as RTL layout works properly
			auto labelText = QString{
				"<p>%1%2</p>"
				"<p>%3%4</p>"
				"<p>%5%6</p>"
			}.arg(
				tr("<b>Name: </b>"), descriptor.displayName,
				tr("<b>Author: </b>"), QString::fromUtf8(descriptor.author)
					.replace("/dot/", ".").replace("/at/", "@").toHtmlEscaped(),
				tr("<b>Description: </b>"), qApp->translate("PluginBrowser", descriptor.description)
					.toHtmlEscaped()
			);
			auto label = new QLabel(labelText, m_descriptionWidget);
			label->setWordWrap(true);
			textWidgetLayout->addWidget(label);
		}

		m_scrollArea->setWidget(m_descriptionWidget);
		m_descriptionWidget->show();
	}
}

void EffectSelectDialog::updateSelection()
{
	// No valid selection anymore due to changed filter?
	if (m_pluginList->selectionModel()->selection().size() <= 0)
	{
		// Then select our first item
		m_pluginList->selectionModel()->select(
			m_model.index(0, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		rowChanged(m_model.index(0, 0), QModelIndex());
	}
}

bool EffectSelectDialog::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == this && event->type() == QEvent::KeyPress)
	{
		auto* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
		{
			QItemSelectionModel* selectionModel = m_pluginList->selectionModel();
			int currentRow = selectionModel->currentIndex().row();
			int newRow = (keyEvent->key() == Qt::Key_Up) ? currentRow - 1 : currentRow + 1;
			int rowCount = m_pluginList->model()->rowCount();
			newRow = qBound(0, newRow, rowCount - 1);

			selectionModel->setCurrentIndex(m_pluginList->model()->index(newRow, 0),
				QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			m_pluginList->scrollTo(m_pluginList->model()->index(newRow, 0));
			return true;
		}
	}

	return QDialog::eventFilter(obj, event);
}

QHBoxLayout* EffectSelectDialog::buildFiltersLayout()
{
	auto* layout = new QHBoxLayout();
	layout->addLayout(buildTypeFilterLayout());
	layout->addLayout(buildCategoryFilterLayout());
	layout->addLayout(buildFavoriteOnlyFilterLayout());
	return layout;
}

QHBoxLayout* EffectSelectDialog::buildTypeFilterLayout()
{
	auto* label = new QLabel(tr("Type"));
	auto* buttonFilter = new QComboBox();
	QStringList labels = {tr("All"), "LMMS", "LADSPA", "LV2", "VST"};
	buttonFilter->addItems(labels);
	connect(buttonFilter, &QComboBox::textActivated, this, [this](QString value) 
	{
		m_model.setEffectTypeFilter(value == tr("All") ? "" : value);
		updateSelection();
	});
	auto* layout = new QHBoxLayout();
	layout->addWidget(label);
	layout->addWidget(buttonFilter);
	return layout;
}

QHBoxLayout* EffectSelectDialog::buildCategoryFilterLayout()
{
	auto* label = new QLabel(tr("Category"));
	auto* buttonFilter = new QComboBox();
	QStringList categories = getEffectCategory()->getCategories();
	QStringList labels = categories;
	labels.push_front(tr("All"));
	buttonFilter->addItems(labels);
	connect(buttonFilter, &QComboBox::textActivated, this, [this](QString value) 
	{
		m_model.setEffectCategoryFilter(value == tr("All") ? "" : value);
		updateSelection();
	});
	auto* layout = new QHBoxLayout();
	layout->addWidget(label);
	layout->addWidget(buttonFilter);
	return layout;
}

QHBoxLayout* EffectSelectDialog::buildFavoriteOnlyFilterLayout()
{
	auto* checkBoxFilter = new QCheckBox(tr("Favorites only"));
	connect(checkBoxFilter, &QCheckBox::toggled, this, [this](bool checked) 
	{
		m_model.setFavoriteOnlyFilter(checked);
		updateSelection();
	});
	checkBoxFilter->toggled(false);
	auto* layout = new QHBoxLayout();
	layout->addWidget(checkBoxFilter);
	return layout;
}

void EffectSelectDialog::showContextMenu(const QPoint& pos)
{
	QMenu menu(this);
	QModelIndex selectedItem = m_pluginList->indexAt(pos);
	QString selectedName = selectedItem.siblingAtColumn(0).data(Qt::DisplayRole).toString();
	QString selectedCategory = selectedItem.siblingAtColumn(1).data(Qt::DisplayRole).toString();
	bool selectedIsFavorite = !selectedItem.siblingAtColumn(3).data(Qt::DecorationRole).isNull();
	auto* setFavoriteAction = new QAction(tr("Toggle favorite"), this);
	connect(setFavoriteAction, &QAction::triggered, this, [this, selectedName, selectedIsFavorite, selectedItem]() {
		getEffectCategory()->toggleFavorite(selectedName, !selectedIsFavorite);
	});
	menu.addAction(setFavoriteAction);
	menu.exec(mapToGlobal(pos));
	emit this->refreshData(selectedName);
}

} // namespace lmms::gui
