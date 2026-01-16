/*
 * SfzSamplerView.h - Declaration of class SfzSamplerView
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#ifndef LMMS_GUI_SFZSAMPLER_VIEW_H
#define LMMS_GUI_SFZSAMPLER_VIEW_H

#include "InstrumentView.h"

namespace lmms {

class SfzSampler;

namespace gui {

class SfzSamplerView : public InstrumentView
{
	Q_OBJECT

public:
	SfzSamplerView(SfzSampler* instrument, QWidget* parent);

public slots:
	void openFile();

protected:
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void dropEvent(QDropEvent* de) override;

	void paintEvent(QPaintEvent* pe) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	bool isResizable() const override { return true; }

	SfzSampler* m_instrument;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_SFZSAMPLER_VIEW_H
