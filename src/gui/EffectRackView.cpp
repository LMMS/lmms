/*
 * EffectRackView.cpp - view for effectChain model
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn@netscape.net>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "EffectRackView.h"

#include <QApplication>
#include <QAction>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QEvent>

#include "Effect.h"
#include "DeprecationHelper.h"
#include "EffectSelectDialog.h"
#include "EffectView.h"
#include "StringPairDrag.h"
#include "GroupBox.h"
#include "TextFloat.h"
#include "FileDialog.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "CaptionMenu.h"
#include "embed.h"


namespace lmms::gui
{

EffectRackView::EffectRackView( EffectChain* model, QWidget* parent ) :
	QWidget( parent ),
	ModelView( nullptr, this )
{
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);

	m_effectsGroupBox = new GroupBox( tr( "EFFECTS CHAIN" ) );
	mainLayout->addWidget( m_effectsGroupBox );

	auto effectsLayout = new QVBoxLayout(m_effectsGroupBox);
	effectsLayout->setSpacing( 0 );
	effectsLayout->setContentsMargins( 2, m_effectsGroupBox->titleBarHeight() + 2, 2, 2 );

	m_scrollArea = new QScrollArea;
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setFrameStyle( QFrame::NoFrame );
	m_scrollArea->setWidget( new QWidget );

	effectsLayout->addWidget( m_scrollArea );

	auto rowLayout = new QHBoxLayout;
	QSize smallIconSize(14, 14);

	auto addButton = new QPushButton;
	addButton->setText( tr( "Add effect" ) );
	addButton->setFocusPolicy(Qt::NoFocus);

	auto clearButton = new QPushButton;
	clearButton->setIcon(embed::getIconPixmap("discard"));
	clearButton->setFocusPolicy(Qt::NoFocus);
	clearButton->setIconSize(smallIconSize);
	clearButton->setFixedSize(18, 18);
	clearButton->setToolTip(tr("Clear the effects from the effect chain"));

	auto savePresetButton = new QPushButton;
	savePresetButton->setIcon(embed::getIconPixmap("project_import"));
	savePresetButton->setFocusPolicy(Qt::NoFocus);
	savePresetButton->setIconSize(smallIconSize);
	savePresetButton->setFixedSize(18, 18);
	savePresetButton->setToolTip(tr("Save the effects as a preset file"));

	auto loadPresetButton = new QPushButton;
	loadPresetButton->setIcon(embed::getIconPixmap("project_export"));
	loadPresetButton->setFocusPolicy(Qt::NoFocus);
	loadPresetButton->setIconSize(smallIconSize);
	loadPresetButton->setFixedSize(18, 18);
	loadPresetButton->setToolTip(tr("Append effects from a preset file"));

	rowLayout->addWidget(addButton, 85);
	rowLayout->addWidget(clearButton, 5);
	rowLayout->addWidget(savePresetButton, 5);
	rowLayout->addWidget(loadPresetButton, 5);

	effectsLayout->addLayout(rowLayout);

	connect(addButton, &QPushButton::clicked, this, &EffectRackView::addEffect);
	connect(clearButton, &QPushButton::clicked, this, &EffectRackView::clearEffects);
	connect(savePresetButton, &QPushButton::clicked, this, &EffectRackView::savePreset);
	connect(loadPresetButton, &QPushButton::clicked, this, &EffectRackView::loadPreset);

	setAcceptDrops(true);

	m_lastY = 0;

	setModel( model );
}



EffectRackView::~EffectRackView()
{
	clearViews();
}




void EffectRackView::dragEnterEvent(QDragEnterEvent* event)
{
	const QString type = StringPairDrag::decodeKey(event);

	if (type == "effectpresetfile")
	{
		event->acceptProposedAction();
	}
	else
	{
		event->ignore();
	}
}

void EffectRackView::dropEvent(QDropEvent* event)
{
	const QString type = StringPairDrag::decodeKey(event);

	if (type == "effectpresetfile")
	{
		addEffectFromPreset(StringPairDrag::decodeValue(event));
		event->acceptProposedAction();
	}
	else
	{
		event->ignore();
	}
}



void EffectRackView::addEffectFromPreset(const QString& filePath)
{
	DataFile dataFile(filePath);
	const QDomElement content = dataFile.content();

	if (content.isNull())
	{
		TextFloat::displayMessage(
			tr("Preset loading error"),
			tr("Could not read the preset file."),
			embed::getIconPixmap("error"));
		return;
	}

	QVector<Effect*> effects;

	auto appendEffectFromElement = [&](const QDomElement& effectElement) -> bool
	{
		const QString pluginName = effectElement.attribute("pluginname");
		const QDomElement keyElement = effectElement.firstChildElement("key");

		if (pluginName.isEmpty() || keyElement.isNull())
		{
			return false;
		}

		EffectKey key(keyElement);
		Effect* effect = Effect::instantiate(
			pluginName,
			fxChain(),
			&key);

		if (effect == nullptr || !effect->isOkay())
		{
			delete effect;
			return false;
		}

		effect->loadSettings(effectElement);
		effects.push_back(effect);
		return true;
	};

	// Backward-compatible single-effect LFXP:
	//
	// <effectsettings pluginname="..." displayname="...">
	//     ...
	// </effectsettings>
	if (!content.attribute("pluginname").isEmpty())
	{
		if (!appendEffectFromElement(content))
		{
			TextFloat::displayMessage(
				tr("Preset loading error"),
				tr("Could not instantiate the effect in the preset."),
				embed::getIconPixmap("error"));
			return;
		}
	}
	else
	{
		// Multi-effect LFXP:
		//
		// <effectsettings numofeffects="N">
		//     <effect>...</effect>
		//     <effect>...</effect>
		// </effectsettings>
		const int expectedCount = content.attribute("numofeffects").toInt();
		const QDomNodeList effectNodes = content.elementsByTagName("effect");

		const int effectCount = expectedCount > 0
			? qMin(expectedCount, effectNodes.count())
			: effectNodes.count();

		for (int i = 0; i < effectCount; ++i)
		{
			if (!appendEffectFromElement(effectNodes.at(i).toElement()))
			{
				for (Effect* effect : effects)
				{
					delete effect;
				}

				TextFloat::displayMessage(
					tr("Preset loading error"),
					tr("Could not instantiate every effect in the preset."),
					embed::getIconPixmap("error"));
				return;
			}
		}
	}

	for (Effect* effect : effects)
	{
		fxChain()->appendEffect(effect);
	}

	update();
}

void EffectRackView::clearEffects()
{
	this->fxChain()->clear();
}

void EffectRackView::savePreset()
{
	FileDialog sfd(this, tr("Save preset"), "",
		tr("LMMS FX Preset (*.lfxp)"));

	const QString workingDir = ConfigManager::inst()->userPresetsDir();

	sfd.setAcceptMode(FileDialog::AcceptSave);
	sfd.setDirectory(workingDir);
	sfd.setFileMode(FileDialog::AnyFile);
	sfd.setDefaultSuffix("lfxp");

	if (sfd.exec() != QDialog::Accepted
		|| sfd.selectedFiles().isEmpty()
		|| sfd.selectedFiles().first().isEmpty())
	{
		return;
	}

	DataFile dataFile(DataFile::Type::EffectSettings);
	QDomElement& content = dataFile.content();

	content.setAttribute("numofeffects",
		static_cast<int>(fxChain()->m_effects.size()));

	for (Effect* effect : fxChain()->m_effects)
	{
		auto effectElement = dataFile.createElement("effect");

		effectElement.setAttribute(
			"pluginname",
			QString::fromUtf8(effect->descriptor()->name));

		effectElement.setAttribute(
			"displayname",
			effect->displayName());

		effect->saveSettings(dataFile, effectElement);
		effectElement.appendChild(effect->key().saveXML(dataFile));

		content.appendChild(effectElement);
	}

	dataFile.writeFile(sfd.selectedFiles().first());
}

void EffectRackView::loadPreset()
{
	FileDialog sfd(this, tr("Load preset"), "",
		tr("LMMS FX Preset (*.lfxp)"));

	const QString workingDir = ConfigManager::inst()->userPresetsDir();

	sfd.setAcceptMode(FileDialog::AcceptOpen);
	sfd.setDirectory(workingDir);
	sfd.setFileMode(FileDialog::ExistingFile);
	sfd.setDefaultSuffix("lfxp");

	if (sfd.exec() == QDialog::Accepted
		&& !sfd.selectedFiles().isEmpty()
		&& !sfd.selectedFiles().first().isEmpty())
	{
		addEffectFromPreset(sfd.selectedFiles().first());
	}
}





void EffectRackView::clearViews()
{
	while( m_effectViews.size() )
	{
		EffectView * e = m_effectViews[m_effectViews.size() - 1];
		m_effectViews.pop_back();
		delete e;
	}
}




void EffectRackView::moveUp( EffectView* view )
{
	fxChain()->moveUp(view->effect());
	if (view != m_effectViews.first())
	{
		int i = 0;
		for (auto it = m_effectViews.begin();
			it != m_effectViews.end(); it++, i++)
		{
			if (*it == view)
			{
				break;
			}
		}

		EffectView * temp = m_effectViews[ i - 1 ];

		m_effectViews[i - 1] = view;
		m_effectViews[i] = temp;

		update();
	}
}




void EffectRackView::moveDown( EffectView* view )
{
	if( view != m_effectViews.last() )
	{
		// moving next effect up is the same
		moveUp( *( std::find( m_effectViews.begin(), m_effectViews.end(), view ) + 1 ) );
	}
}




void EffectRackView::deletePlugin( EffectView* view )
{
	Effect * e = view->effect();
	m_effectViews.erase( std::find( m_effectViews.begin(), m_effectViews.end(), view ) );
	delete view;
	fxChain()->removeEffect( e );
	e->deleteLater();
	update();
}




void EffectRackView::update()
{
	QWidget * w = m_scrollArea->widget();
	QVector<bool> view_map( qMax<int>( fxChain()->m_effects.size(),
						m_effectViews.size() ), false );

	for (const auto& effect : fxChain()->m_effects)
	{
		int i = 0;
		for (const auto& effectView : m_effectViews)
		{
			if (effectView->model() == effect)
			{
				view_map[i] = true;
				break;
			}
			++i;
		}
		if( i >= m_effectViews.size() )
		{
			auto view = new EffectView(effect, w);
			connect(view, &EffectView::movedUp, this, &EffectRackView::moveUp);
			connect(view, &EffectView::movedDown, this, &EffectRackView::moveDown);
			connect(view, &EffectView::deletedPlugin, this, &EffectRackView::deletePlugin, Qt::QueuedConnection);

			QAction* moveUpAction = new QAction(view);
			moveUpAction->setShortcut(keySequence(Qt::Key_Up, Qt::AltModifier));
			moveUpAction->setShortcutContext(Qt::WidgetShortcut);
			connect(moveUpAction, &QAction::triggered, view, &EffectView::moveUp);
			view->addAction(moveUpAction);

			QAction* moveDownAction = new QAction(view);
			moveDownAction->setShortcut(keySequence(Qt::Key_Down, Qt::AltModifier));
			moveDownAction->setShortcutContext(Qt::WidgetShortcut);
			connect(moveDownAction, &QAction::triggered, view, &EffectView::moveDown);
			view->addAction(moveDownAction);

			view->show();
			m_effectViews.append( view );
			if( i < view_map.size() )
			{
				view_map[i] = true;
			}
			else
			{
				view_map.append( true );
			}

		}
	}

	int i = 0, nView = 0;

	const int EffectViewMargin = 3;
	m_lastY = EffectViewMargin;

	for (auto it = m_effectViews.begin(); it != m_effectViews.end(); i++)
	{
		if( i < view_map.size() && view_map[i] == false )
		{
			delete m_effectViews[nView];
			it = m_effectViews.erase( it );
		}
		else
		{
			( *it )->move( EffectViewMargin, m_lastY );
			m_lastY += ( *it )->height();
			++nView;
			++it;
		}
	}

	w->setFixedSize(EffectView::DEFAULT_WIDTH + 2 * EffectViewMargin, m_lastY);

	QWidget::update();
}




void EffectRackView::addEffect()
{
	EffectSelectDialog esd( this );
	esd.exec();

	if( esd.result() == QDialog::Rejected )
	{
		return;
	}

	Effect * fx = esd.instantiateSelectedPlugin( fxChain() );

	fxChain()->appendEffect( fx );
	update();

	// Find the effectView, and show the controls
	for (const auto& effectView : m_effectViews)
	{
		if (effectView->effect() == fx)
		{
			effectView->editControls();
			break;
		}
	}


}




void EffectRackView::modelChanged()
{
	//clearViews();
	m_effectsGroupBox->setModel( &fxChain()->m_enabledModel );
	connect( fxChain(), SIGNAL(aboutToClear()), this, SLOT(clearViews()));
	update();
}




QSize EffectRackView::sizeHint() const
{
	// Use the formula from InstrumentTrackWindow.cpp
	return QSize{EffectRackView::DEFAULT_WIDTH, 254 /* INSTRUMENT_HEIGHT */ - 4 - 1};
}




} // namespace lmms::gui
