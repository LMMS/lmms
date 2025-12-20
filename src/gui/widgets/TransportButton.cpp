#include "TransportButton.h"

#include <QMouseEvent>

#include "ControllerConnection.h"
#include "CaptionMenu.h"
#include "embed.h"

namespace lmms::gui
{

TransportButton::TransportButton(QWidget * _parent, const QString & _name) :
	AutomatableButton(_parent, _name)
{
	setCheckable(true);
	model()->setStrictStepSize(true);
}


void TransportButton::mousePressEvent(QMouseEvent * _me)
{
	QPushButton::mousePressEvent(_me);
}

void TransportButton::mouseReleaseEvent(QMouseEvent * _me)
{
	if (_me)
	{
		QPushButton::mouseReleaseEvent(_me);
	}
}

void TransportButton::contextMenuEvent(QContextMenuEvent * _me)
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent(nullptr);

	CaptionMenu contextMenu(this->model()->displayName());
	AutomatableModel* model = modelUntyped();
	auto amvSlots = new AutomatableModelViewSlots(this, &contextMenu);
	QString controllerTxt;
	if (model->controllerConnection())
	{
		Controller* cont = model->controllerConnection()->getController();
		if (cont)
		{
			controllerTxt = AutomatableModel::tr("Connected to %1").arg(cont->name());
		}
		else
		{
			controllerTxt = AutomatableModel::tr("Connected to controller");
		}

		QMenu* contMenu = contextMenu.addMenu(embed::getIconPixmap("controller"), controllerTxt);

		contMenu->addAction(embed::getIconPixmap("controller"),
								AutomatableModel::tr("Edit connection..."),
								amvSlots, SLOT(execConnectionDialog()));
		contMenu->addAction(embed::getIconPixmap( "cancel" ),
								AutomatableModel::tr("Remove connection"),
								amvSlots, SLOT(removeConnection()));
	}
	else
	{
		contextMenu.addAction( embed::getIconPixmap("controller"),
							AutomatableModel::tr("Connect to controller..."),
							amvSlots, SLOT(execConnectionDialog()));
	}
	contextMenu.exec(QCursor::pos());
}

} // namespace lmms::gui
