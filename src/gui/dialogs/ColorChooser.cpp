/* ColorChooser.cpp - definition of ColorChooser class.
 *
 * Copyright (c) 2020 russiankumar <adityakumar4644/at/gmail/dot/com>
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

#include <ColorChooser.h>



void ColorChooser::setPalette (const QVector<QColor> colors)
{
	const int max = qMin (colors.size(), 48);
	for (int i = 0; i < max; i++)
	{
		ColorChooser::setStandardColor (i, colors[i]);
	}
}


void ColorChooser::setPalette (const CCPalette palette)
{
	switch (palette)
	{
		case CCPalette::Default: setPalette (defaultPalette()); break;
		case CCPalette::Mixer: setPalette (nicePalette(140)); break;
		case CCPalette::Track: setPalette (nicePalette(150)); break;
	}
}


ColorChooser* ColorChooser::withPalette (const CCPalette palette)
{
	setPalette (palette);
	return this;
}




QVector<QColor> ColorChooser::defaultPalette()
{
	QVector <QColor> result (48);
	for (int i = 0; i < 48; i++)
	{
		result[i] = (QColorDialog::standardColor(i));
	}
	return result;
}


QVector<QColor> ColorChooser::nicePalette (const int base)
{
	QVector <QColor> result (48);
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 6; y++)
		{
			result[6 * x + y].setHsl (qMax(0, 44 * x - 1), 150 - 20 * y, base - 10 * y);
		}
	}
	return result;
}
