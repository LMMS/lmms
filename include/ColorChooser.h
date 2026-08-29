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

#ifndef LMMS_GUI_COLOR_CHOOSER_H
#define LMMS_GUI_COLOR_CHOOSER_H

#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QKeyEvent>
#include <QVector>

namespace lmms::gui
{


class ColorChooser : public QColorDialog
{
public:
	ColorChooser(const QColor &initial, QWidget *parent): QColorDialog(initial, parent) {};
	ColorChooser(QWidget *parent): QColorDialog(parent) {};
	//! For getting a color without having to initialise a color dialog
	ColorChooser() = default;
	enum class Palette {Default, Track, Mixer};
	//! Set global palette via array, checking bounds
	void setPalette (QVector<QColor>);
	//! Set global paletter via enum
	void setPalette (Palette);
	//! Set palette via enum, return self pointer for chaining
	ColorChooser* withPalette (Palette);
	//! Return a certain palette
	static QVector<QColor> getPalette (Palette);

protected:
	//! Forward key events to the parent to prevent stuck notes when the dialog gets focus
	void keyReleaseEvent(QKeyEvent *event) override
	{
		QApplication::sendEvent(parentWidget(), event);
	}
private:
	//! Copy the current QColorDialog palette into an array
	static QVector<QColor> defaultPalette();
	//! Generate a nice palette, with adjustable value
	static QVector<QColor> nicePalette (int);
};


} // namespace lmms::gui

#endif // LMMS_GUI_COLOR_CHOOSER_H
