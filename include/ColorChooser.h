/* ColorChooser.h - declaration and definition of ColorChooser class.
 *
 * Copyright (c) 2019 CYBERDEViLNL <cyberdevilnl/at/protonmail/dot/ch>
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

#include <QColorDialog>
#include <QApplication>
#include <QKeyEvent>

class ColorChooser: public QColorDialog
{
public:
	ColorChooser(const QColor &initial, QWidget *parent): QColorDialog(initial, parent) {};
	ColorChooser(QWidget *parent): QColorDialog(parent) {};

protected:
	// Forward key events to the parent to prevent stuck notes when the dialog gets focus
	void keyReleaseEvent(QKeyEvent *event) override
	{
		QKeyEvent ke(*event);
		QApplication::sendEvent(parentWidget(), &ke);
	}
};
