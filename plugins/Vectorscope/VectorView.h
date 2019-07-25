/* VectorView.h - declaration of VectorView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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
#ifndef VECTORVIEW_H
#define VECTORVIEW_H

#include <QPainter>
#include <QWidget>

#include "VecControls.h"


// Widget that displays a vectorscope visualization of stereo signal.
class VectorView : public QWidget
{
	Q_OBJECT
public:
	explicit VectorView(VecControls *controls, QWidget *_parent = 0);
	virtual ~VectorView() {}

	QSize sizeHint() const override {return QSize(300, 200);}

protected:
	void paintEvent(QPaintEvent *event) override;

private slots:
	void periodicUpdate();

private:
	VecControls *m_controls;

	bool m_visible;
};
#endif // VECTORVIEW_H
