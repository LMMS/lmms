/*
 * eqcontrolsdialog.h - defination of EqControlsDialog class.
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

#ifndef EQCONTROLSDIALOG_H
#define EQCONTROLSDIALOG_H


#include "EffectControlDialog.h"

namespace lmms
{

class BoolModel;
class FloatModel;

class EqControls;

namespace gui
{

class EqBand;
class EqParameterWidget;

class EqControlsDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	EqControlsDialog( EqControls * controls );
	~EqControlsDialog() override = default;

	EqBand * setBand( EqControls * controls );

private:
	EqControls * m_controls;
	EqParameterWidget * m_parameterWidget;

	void mouseDoubleClickEvent(QMouseEvent *event) override;

	EqBand *setBand( int index, BoolModel *active, FloatModel *freq, FloatModel *res, FloatModel *gain, QColor color, QString name, float *peakL, float *peakR, BoolModel *hp12, BoolModel *hp24, BoolModel *hp48, BoolModel *lp12, BoolModel *lp24, BoolModel *lp48 );

	int m_originalHeight;
};


} // namespace gui

} // namespace lmms

#endif // EQCONTROLSDIALOG_H
