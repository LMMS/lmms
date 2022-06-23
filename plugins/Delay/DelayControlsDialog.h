/*
 * delaycontrolsdialog.h - declaration of DelayControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef DELAYCONTROLSDIALOG_H
#define DELAYCONTROLSDIALOG_H

#include "EffectControlDialog.h"

namespace lmms
{


class DelayControls;
class FloatModel;

namespace gui
{

class DelayControlsDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	DelayControlsDialog( DelayControls* controls );
	~DelayControlsDialog() override
	{
	}
};

class XyPad : public QWidget
{
	Q_OBJECT
public:
	XyPad( QWidget *parent = 0, FloatModel *xModel = 0, FloatModel *yModel = 0 );
	~XyPad() override {}

protected:
	void paintEvent ( QPaintEvent * event ) override;
	void mousePressEvent(QMouseEvent * event ) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;

private:
	FloatModel *m_xModel;
	FloatModel *m_yModel;
	bool m_acceptInput;
};


} // namespace gui

} // namespace lmms

#endif // DELAYCONTROLSDIALOG_H
