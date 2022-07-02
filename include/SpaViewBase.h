/*
 * SpaViewBase.h - base class for SPA plugin views
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef SPAVIEWBASE_H
#define SPAVIEWBASE_H

#include <QVector>
#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include "LinkedModelGroupViews.h"

class AutomatableModel;
class QDropEvent;
class QGridLayout;
class QPushButton;
class QWidget;

namespace lmms
{
class AutomatableModel;
class SpaProc;
class SpaControlBase;
}

namespace lmms::gui
{

class SpaViewProc : public LinkedModelGroupView
{
	Q_OBJECT
public:
	SpaViewProc(QWidget *parent, SpaProc *proc,
				std::size_t colNum);
	// TODO: make those two private?
	void dropEvent(QDropEvent *de) override;
	void dragEnterEvent(QDragEnterEvent *dev) override;
private slots:
	void modelAdded(lmms::AutomatableModel* mdl);
	void modelRemoved(lmms::AutomatableModel *mdl);
private:
	SpaProc* m_proc;
};

class SpaViewBase : LinkedModelGroupsView
{
	QGridLayout *m_grid;
	const int m_firstModelRow = 1; // row 0 is for buttons
	const int m_rowNum = 6; // just some guess for what might look good

	//QVector<class AutomatableModelView*> m_modelViews;

	SpaViewProc* m_procView; // TODO: unique_ptr

	LinkedModelGroupView *getGroupView() override;

protected:
	QPushButton *m_toggleUIButton = nullptr;
	QPushButton *m_reloadPluginButton;

	// to be called by virtuals
	void modelChanged(SpaControlBase* ctrlBase);
	void connectSlots(const char* toggleUiSlot);
	SpaViewBase(QWidget *meAsWidget, SpaControlBase* ctrlBase);
	virtual ~SpaViewBase();
	void dropEvent(QDropEvent *de);

private:
	//! Numbers of controls per row; must be multiple of 2 for mono effects
	const std::size_t m_colNum = 6;

	enum Rows
	{
		ButtonRow,
		ProcRow,
		LinkChannelsRow/*,
		DropButtonRow*/
	};
	void dragEnterEvent(QDragEnterEvent *de);
};

#if 0
class SpaFxControlDialog : public EffectControlDialog
{
	Q_OBJECT

	class SpaFxControls *spaControls();
	void modelChanged() override;

public:
	SpaFxControlDialog(class SpaFxControls *controls);
	virtual ~SpaFxControlDialog() override {}

private slots:
	void toggleUI();
	void reloadPlugin();
};


class SpaInsView : public InstrumentView
{
	Q_OBJECT
public:
	SpaInsView(Instrument *_instrument, QWidget *_parent);
	virtual ~SpaInsView();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *_dee);
	virtual void dropEvent(QDropEvent *_de);


private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif

} // namespace lmms::gui

#endif // LMMS_HAVE_SPA

#endif // SPAVIEWBASE_H
