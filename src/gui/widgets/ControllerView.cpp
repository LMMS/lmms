/*
 * ControllerView.cpp - view-component for an controller
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ControllerView.h"

#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>


#include "CaptionMenu.h"
#include "ControllerConnection.h"
#include "ControllerDialog.h"
#include "ControllerRackView.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "ToolTip.h"


ControllerView::ControllerView(Controller * controller, QWidget * parent) :
	QFrame(parent),
	ModelView(controller, this),
	m_controllerDlg(NULL),
	m_titleBarHeight(24),
	m_show(true),
	m_modelC(controller)
{
	const QSize buttonsize(17, 17);
	setFrameStyle(QFrame::Plain);
	setFrameShadow(QFrame::Plain);
	setLineWidth(0);
	setContentsMargins(0, 0, 0, 0);

	m_controllerDlg = getController()->createDialog(this);
	m_controllerDlg->move(1, m_titleBarHeight);

	controllerTypeLabel = new QLabel(this);
	controllerTypeLabel->setText(QString(getController()->type() == Controller::LfoController
										 ? QString("LFO: ")
										 : QString("PEAK: ")));
	controllerTypeLabel->move(3, 3);
	controllerTypeLabel->show();

	m_nameLineEdit = new QLineEdit(this);
	m_nameLineEdit->setText(controller->name());
	m_nameLineEdit->setReadOnly(true);
	m_nameLineEdit->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_nameLineEdit->move(controllerTypeLabel->sizeHint().width() + 3, 2);
	connect(m_nameLineEdit, SIGNAL(editingFinished()), this, SLOT(renameFinished()));

	setFixedWidth(m_controllerDlg->width() + 2);
	setFixedHeight(m_controllerDlg->height() + m_titleBarHeight + 1);

	m_collapseButton = new QPushButton(embed::getIconPixmap("stepper-down"), QString::null, this );
	m_collapseButton->resize(buttonsize);
	m_collapseButton->setFocusPolicy(Qt::NoFocus);
	m_collapseButton->setCursor(Qt::ArrowCursor);
	m_collapseButton->setAttribute(Qt::WA_NoMousePropagation);
	m_collapseButton->setToolTip(tr("collapse"));
	m_collapseButton->move(width() - buttonsize.width() - 3, 3);
	connect(m_collapseButton, SIGNAL(clicked()), this, SLOT(toggleCollapseController()));

	setAcceptDrops(true);
	setModel(controller);
}




ControllerView::~ControllerView()
{
}




void ControllerView::collapseController()
{
	m_collapseButton->setIcon(embed::getIconPixmap("stepper-left"));
	m_controllerDlg->hide();
	setFixedHeight(m_titleBarHeight);
	emit controllerCollapsed();
	gui->getControllerRackView()->setAllCollapsed(false);
	gui->getControllerRackView()->setAllExpanded(false);

}




void ControllerView::expandController()
{
	m_controllerDlg->show();
	setFixedHeight(m_controllerDlg->height() + m_titleBarHeight + 1);
	m_collapseButton->setIcon(embed::getIconPixmap("stepper-down"));
	gui->getControllerRackView()->setAllCollapsed(false);
	gui->getControllerRackView()->setAllExpanded(false);
}




void ControllerView::deleteController()
{
	emit(deleteController(this));
}




void ControllerView::toggleCollapseController()
{
	if(m_controllerDlg->isHidden())
	{
		expandController();
	}
	else
	{
		collapseController();
	}
}




void ControllerView::renameFinished()
{
	m_nameLineEdit->setReadOnly(true);
	Controller * c = castModel<Controller>();
	QString new_name = m_nameLineEdit->text();
	if (new_name != c->name())
	{
		c->setName(new_name);
		Engine::getSong()->setModified();
	}
}




void ControllerView::rename()
{
	m_nameLineEdit->setReadOnly(false);
	m_nameLineEdit->setFocus();
	m_nameLineEdit->selectAll();
}




void ControllerView::moveUp()
{
	emit controllerMoveUp(this);
}




void ControllerView::moveDown()
{
	emit controllerMoveDown(this);
}




void ControllerView::mouseDoubleClickEvent(QMouseEvent *)
{
	rename();
}




void ControllerView::dragEnterEvent(QDragEnterEvent *dee)
{
	StringPairDrag::processDragEnterEvent(dee, "float_value,automatable_model" );
}




void ControllerView::dropEvent(QDropEvent * de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString val = StringPairDrag::decodeValue(de);
	if (type == "automatable_model")
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(Engine::projectJournal()->journallingObject(val.toInt()));
		if (m_modelC->hasModel(mod))
		{
			QMessageBox::warning(this, tr("LMMS"), tr("Cycle Detected."));
		}

		if (mod != NULL && m_modelC->hasModel(mod) == false)
		{
			if (mod->controllerConnection())
			{
				mod->controllerConnection()->setController(getController());
			}
			else
			{
				ControllerConnection * cc = new ControllerConnection(getController());
				mod->setControllerConnection(cc);
			}
		}
	}
}




void ControllerView::modelChanged()
{
}




void ControllerView::paintEvent(QPaintEvent*)
{
	QPainter p( this );
	QRect rect( 1, 1, width()-2, m_titleBarHeight );
	p.fillRect( rect, p.background() );
}




void ControllerView::contextMenuEvent(QContextMenuEvent *)
{
	QPointer<CaptionMenu> contextMenu = new CaptionMenu(model()->displayName(), this);

	contextMenu->addAction(embed::getIconPixmap("cancel"), tr("&Remove this controller"), this, SLOT(deleteController()));

	contextMenu->addAction(embed::getIconPixmap("stepper-left"), tr("&Collaps all controllers"),
												this, SIGNAL(collapseAll())
						   )->setDisabled(gui->getControllerRackView()->allCollapsed());
	contextMenu->addAction(embed::getIconPixmap("stepper-down"), tr("&Expand all controllers"),
												this, SIGNAL(expandAll())
						   )->setDisabled(gui->getControllerRackView()->allExpanded());
	contextMenu->addAction(embed::getIconPixmap("stepper-up"), tr("Move &up"), this, SLOT(moveUp())
						   )->setDisabled(gui->getControllerRackView()->controllerViews().first() == this);
	contextMenu->addAction(embed::getIconPixmap("stepper-down"), tr("Move &down"), this, SLOT(moveDown())
						   )->setDisabled(gui->getControllerRackView()->controllerViews().last() == this);
	contextMenu->addSeparator();
	contextMenu->exec(QCursor::pos());
	delete contextMenu;
}
