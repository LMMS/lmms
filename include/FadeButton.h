/*
 * FadeButton.h - declaration of class fadeButton
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_FADE_BUTTON_H
#define LMMS_GUI_FADE_BUTTON_H

#include <QAbstractButton>
#include <QColor>
#include <QElapsedTimer>


namespace lmms::gui
{


class FadeButton : public QAbstractButton
{
	Q_OBJECT
public:
	enum class State
	{
		Normal,
		Corrupted,
		Count
	};

	/**
	 * @brief Creates a fade button with the given color palette.
	 * 
	 * @param inactiveColor The default color for the widget
	 * @param normalColor The color to use on activation when in the Normal state
	 * @param corruptedColor The color to use on activation when in the Corrupted state
	 * @param mutedColor The color to use when the fade button is muted
	 * @param holdColor The color after the "play" fade is done but a note is still playing
	 */
	FadeButton(
		const QColor& inactiveColor,
		const QColor& normalColor,
		const QColor& corruptedColor,
		const QColor& mutedColor,
		const QColor& holdColor,
		QWidget* parent = nullptr);

	//! @brief Creates a fade button with the default application colors.
	FadeButton(QWidget* parent = nullptr);

	//! @brief Destroys the fade button.
	~FadeButton() override = default;

	//! @returns the current state of the fade button.
	auto state() const -> State;

	//! @returns true if the fade button is muted.
	auto muted() const -> bool;

	//! Set the state of the fade button to @a state.
	void setState(State state);

	//! Mute or unmute the fade button.
	void setMuted(bool mute);

public slots:
	void activate();
	void activateOnce();
	void noteEnd();


protected:
	void paintEvent( QPaintEvent * _pe ) override;


private:
	State m_state = State::Normal;

	QElapsedTimer m_stateTimer;
	QElapsedTimer m_releaseTimer;

	QColor m_inactiveColor;
	QColor m_normalColor;
	QColor m_corruptedColor;
	QColor m_mutedColor;
	QColor m_holdColor;

	int activeNotes;
	bool m_muted = false;

	QColor fadeToColor(QColor, QColor, QElapsedTimer, float);
	QColor activeColor() const;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_FADE_BUTTON_H
