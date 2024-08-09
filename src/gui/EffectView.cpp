/*
 * EffectView.cpp - view-component for an effect
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QBoxLayout>
#include <QLabel>
#include <QToolButton>

#include "Effect.h"
#include "EffectView.h"
#include "DummyEffect.h"
#include "CaptionMenu.h"
#include "EffectLabelButton.h"
#include "embed.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "MainWindow.h"
#include "SubWindow.h"
#include "TempoSyncKnob.h"


namespace lmms::gui
{

EffectView::EffectView(Effect * _model, QWidget * _parent) :
	PluginView(_model, _parent),
	m_subWindow(nullptr),
	m_controlView(nullptr),
	m_dragging(false)
{
	setFixedSize(EffectView::DEFAULT_WIDTH, EffectView::DEFAULT_HEIGHT);
	setFocusPolicy(Qt::StrongFocus);
	
	m_mainLayout = new QHBoxLayout();
	m_mainLayout->setContentsMargins(8, 2, 8, 2);

	bool hasControls = effect()->controls()->controlCount() > 0;
	bool isEnabled = !dynamic_cast<DummyEffect *>( effect() );

	m_bypass = new LedCheckBox(this, "", isEnabled ? LedCheckBox::LedColor::Green : LedCheckBox::LedColor::Red);
	m_bypass->setEnabled(isEnabled);
	m_bypass->setToolTip(tr("On/Off"));
	m_mainLayout->addWidget(m_bypass);

	QFont labelFont = adjustedToPixelSize(font(), 10);
	m_label = new EffectLabelButton(this, this);
	m_label->setText(model()->displayName());
	m_label->setFont(labelFont);
	if(hasControls)
	{
		connect(m_label, &EffectLabelButton::clicked, this, &EffectView::editControls);
	}
	m_mainLayout->addWidget(m_label);

	m_wetDry = new Knob(KnobType::Small17, this);
	m_wetDry->setEnabled(isEnabled);
	m_wetDry->setHintText(tr("Wet Level:"), "");
	m_mainLayout->addWidget(m_wetDry);

	m_autoQuit = new TempoSyncKnob(KnobType::Small17, this);
	m_autoQuit->setEnabled(isEnabled && !effect()->m_autoQuitDisabled);
	m_autoQuit->setVisible(isEnabled && !effect()->m_autoQuitDisabled);
	m_autoQuit->setHintText(tr("Stop after:"), "ms");
	m_mainLayout->addWidget(m_autoQuit);

	setModel( _model );

	if(hasControls)
	{
		m_controlView = effect()->controls()->createView();
		if( m_controlView )
		{
			m_subWindow = getGUI()->mainWindow()->addWindowedWidget( m_controlView );
			if ( !m_controlView->isResizable() )
			{
				m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
				if (m_subWindow->layout())
				{
					m_subWindow->layout()->setSizeConstraint(QLayout::SetFixedSize);
				}
			}

			Qt::WindowFlags flags = m_subWindow->windowFlags();
			flags &= ~Qt::WindowMaximizeButtonHint;
			m_subWindow->setWindowFlags( flags );

			connect( m_controlView, SIGNAL(closed()),
					this, SLOT(closeEffects()));

			m_subWindow->hide();
		}
	}
	
	m_opacityEffect = new QGraphicsOpacityEffect(this);
	m_opacityEffect->setOpacity(1);
	setGraphicsEffect(m_opacityEffect);

	setLayout(m_mainLayout);
}




EffectView::~EffectView()
{
	delete m_subWindow;
}




void EffectView::editControls()
{
	if( m_subWindow )
	{
		if( !m_subWindow->isVisible() )
		{
			m_subWindow->show();
			m_subWindow->raise();
			effect()->controls()->setViewVisible( true );
			m_label->setChecked(true);
		}
		else
		{
			m_subWindow->hide();
			effect()->controls()->setViewVisible( false );
			m_label->setChecked(false);
		}
	}
}




void EffectView::moveUp()
{
	emit movedUp(this);
}




void EffectView::moveDown()
{
	emit movedDown(this);
}



void EffectView::deletePlugin()
{
	emit deletedPlugin(this);
}




void EffectView::closeEffects()
{
	if( m_subWindow )
	{
		m_subWindow->hide();
	}
	effect()->controls()->setViewVisible( false );
	m_label->setChecked(false);
}



void EffectView::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( model()->displayName(), this );
	contextMenu->addAction( embed::getIconPixmap( "arp_up" ),
						tr( "Move &up" ),
						this, SLOT(moveUp()));
	contextMenu->addAction( embed::getIconPixmap( "arp_down" ),
						tr( "Move &down" ),
						this, SLOT(moveDown()));
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT(deletePlugin()));
	contextMenu->addSeparator();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}



void EffectView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging = true;
		m_opacityEffect->setOpacity(0.3);
		QCursor c(Qt::SizeVerCursor);
		QApplication::setOverrideCursor(c);
		update();
	}
}

void EffectView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging = false;
		m_opacityEffect->setOpacity(1);
		QApplication::restoreOverrideCursor();
		update();
	}
}

void EffectView::mouseMoveEvent(QMouseEvent* event)
{
	if (!m_dragging) { return; }
	if (event->pos().y() < 0)
	{
		moveUp();
	}
	else if (event->pos().y() > EffectView::DEFAULT_HEIGHT)
	{
		moveDown();
	}
}



void EffectView::paintEvent( QPaintEvent * )
{
	QPainter p(this);
	QPainterPath path;

	path.addRoundedRect(rect().marginsRemoved(QMargins(2, 2, 2, 2)), 2, 2);

	QPen pen(Qt::black, 1);
	p.setPen(pen);
	p.fillPath(path, QColor(0x3b, 0x42, 0x4a));
	p.drawPath(path);
}




void EffectView::modelChanged()
{
	m_bypass->setModel( &effect()->m_enabledModel );
	m_wetDry->setModel( &effect()->m_wetDryModel );
	m_autoQuit->setModel( &effect()->m_autoQuitModel );
}

} // namespace lmms::gui
