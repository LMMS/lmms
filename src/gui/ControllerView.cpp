/*
 * ControllerView.cpp - view-component for an controller
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2019 Steffen Baranowsky <BaraMGB/at/freenet.de>
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


#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QInputDialog>

#include "ControllerView.h"

#include "CaptionMenu.h"
#include "ControllerConnection.h"
#include "ControllerDialog.h"
#include "embed.h"
#include "ControllerRackView.h"
#include "GuiApplication.h"
#include "LmmsStyle.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "StringPairDrag.h"

namespace lmms::gui
{


ControllerView::ControllerView(Controller* model, QWidget* parent)
	: QFrame{parent}
	, ModelView{model, this}
	, m_controllerDlg{nullptr}
    , m_titleBarHeight(24)
    , m_modelC(model)
{
	const QSize buttonsize(17, 17);
	setFrameStyle(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);
	setFocusPolicy(Qt::StrongFocus);
	setLineWidth(0);
	setContentsMargins(0, 0, 0, 0);

	m_controllerDlg = getController()->createDialog(this);
	m_controllerDlg->move(1, m_titleBarHeight);

	controllerTypeLabel = new QLabel(this);
	controllerTypeLabel->setText(QString(getController()->type() == Controller::ControllerType::Lfo
										 ? QString("LFO: ")
										 : QString("PEAK: ")));
	controllerTypeLabel->move(3, 3);
	controllerTypeLabel->show();

	m_nameLabel = new QLabel(model->name(), this);
	m_nameLabel->move(controllerTypeLabel->sizeHint().width() + 3, 3);

	setFixedWidth(m_controllerDlg->width() + 2);
	setFixedHeight(m_controllerDlg->height() + m_titleBarHeight + 1);

	m_collapseButton = new QPushButton(QString(), this );
	m_collapseButton->setObjectName("btn-stepper-down");
	m_collapseButton->resize(buttonsize);
	m_collapseButton->setFocusPolicy(Qt::NoFocus);
	m_collapseButton->setCursor(Qt::ArrowCursor);
	m_collapseButton->setAttribute(Qt::WA_NoMousePropagation);
	m_collapseButton->setToolTip(tr("collapse"));
	m_collapseButton->move(width() - buttonsize.width() - 3, 3);
	connect(m_collapseButton, SIGNAL(clicked()), this, SLOT(toggleCollapseController()));

	setAcceptDrops(true);
	setModel(model);
}



bool ControllerView::isCollapsed() const
{
	return m_controllerDlg->isHidden();
}




void ControllerView::collapseController(bool collapse)
{
	if (collapse)
	{
		m_collapseButton->setObjectName("btn-stepper-left");
		m_controllerDlg->hide();
		setFixedHeight(m_titleBarHeight);
		emit controllerCollapsed();
	}
	else
	{
		m_controllerDlg->show();
		setFixedHeight(m_controllerDlg->height() + m_titleBarHeight + 1);
		m_collapseButton->setObjectName("btn-stepper-down");
	}
	m_collapseButton->style()->unpolish(m_collapseButton);
	m_collapseButton->style()->polish(m_collapseButton);
}




void ControllerView::moveUp() { emit movedUp(this); }

void ControllerView::moveDown() { emit movedDown(this); }

void ControllerView::removeController() { emit removedController(this); }




void ControllerView::toggleCollapseController()
{
	collapseController(!m_controllerDlg->isHidden());
}


void ControllerView::renameController()
{
	bool ok;
	auto c = castModel<Controller>();
	QString new_name = QInputDialog::getText( this,
			tr( "Rename controller" ),
			tr( "Enter the new name for this controller" ),
			QLineEdit::Normal, c->name() , &ok );
	if( ok && !new_name.isEmpty() )
	{
		c->setName( new_name );
		m_nameLabel->setText( new_name );
	}
}


void ControllerView::mouseDoubleClickEvent(QMouseEvent* me)
{
	if (me->y() <= m_titleBarHeight)
	{
		toggleCollapseController();
	}
}




void ControllerView::dragEnterEvent(QDragEnterEvent* dee)
{
	StringPairDrag::processDragEnterEvent(dee, "automatable_model" );
}




void ControllerView::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString val = StringPairDrag::decodeValue(de);
	if (type == "automatable_model")
	{
		auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(val.toInt()));
		if (mod != nullptr)
		{
			if (m_modelC->hasModel(mod))
			{
				QMessageBox::warning(this, tr("LMMS"), tr("Cycle Detected."));
			}
			else
			{
				if (mod->controllerConnection())
				{
					mod->controllerConnection()->setController(getController());
				}
				else
				{
					ControllerConnection* cc = new ControllerConnection(getController());
					mod->setControllerConnection(cc);
				}
			}
		}
	}
}




void ControllerView::modelChanged()
{
}



void ControllerView::contextMenuEvent( QContextMenuEvent * )
{
	Controller* c = castModel<Controller>();
	QPointer<CaptionMenu> contextMenu = new CaptionMenu(c->name(), this);
	auto moveUpAction = contextMenu->addAction(embed::getIconPixmap("arp_up"), tr("Move &up"), this, &ControllerView::moveUp);
	moveUpAction->setDisabled(getGUI()->getControllerRackView()->controllerViews().first() == this);

	auto moveDownAction = contextMenu->addAction(embed::getIconPixmap("arp_down"), tr("Move &down"), this, &ControllerView::moveDown);
	moveDownAction->setDisabled(getGUI()->getControllerRackView()->controllerViews().last() == this);

	contextMenu->addSeparator();
	contextMenu->addAction(
		embed::getIconPixmap("cancel"), tr("&Remove this controller"), this, &ControllerView::removeController);
	contextMenu->addAction( tr("Re&name this controller"), this, SLOT(renameController()));

	contextMenu->addSeparator();
	auto collapseAction = contextMenu->addAction(embed::getIconPixmap("arrow-left"), tr("&Collapse all controllers"), this, SIGNAL(collapseAll()));
	collapseAction->setDisabled(getGUI()->getControllerRackView()->allCollapsed());

	auto expandAction = contextMenu->addAction(embed::getIconPixmap("arrow-down"), tr("&Expand all controllers"), this, SIGNAL(expandAll()));
	expandAction->setDisabled(getGUI()->getControllerRackView()->allExpanded());

	contextMenu->addSeparator();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}


void ControllerView::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	QRect rect(1, 1, width() - 2, m_titleBarHeight);
	p.fillRect(rect, p.background());
}

} // namespace lmms::gui
