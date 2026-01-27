/*
 * SfzSamplerView.h - GUI for SfzSampler
 *
 * Copyright (c) 2026 Keratin
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

class QGridLayout;
class QLabel;

namespace lmms {

class SfzSampler;

namespace gui {


//! This class is planned to be completely redone!
//! In it's current form, it's simply a temporary GUI which shows the keyswitch info, the parameters, and a button to load a file.
//! In the future, it would be great to have a much prettier and useful GUI
class SfzSamplerView : public InstrumentView
{
	Q_OBJECT

public:
	SfzSamplerView(SfzSampler* instrument, QWidget* parent);

public slots:
	void onFileLoaded();
	void openFile();

protected:
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void dropEvent(QDropEvent* de) override;

	void paintEvent(QPaintEvent* pe) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	bool isResizable() const override { return true; }

	SfzSampler* m_instrument;

	QLabel* m_switchKeysLabel;
	QWidget* m_controlsWidget;
	QGridLayout* m_knobLayout;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_SFZSAMPLER_VIEW_H
