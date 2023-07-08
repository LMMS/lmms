/*
 * FloatModelEditorBase.h - Base editor for float models
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2023 Michael Gregorius
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

#ifndef LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H
#define LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H

#include <memory>
#include <QPixmap>
#include <QWidget>
#include <QPoint>
#include <QTextDocument>

#include "AutomatableModelView.h"


class QPixmap;

namespace lmms::gui
{


class SimpleTextFloat;

void convertPixmapToGrayScaleTemp(QPixmap &pixMap);

class LMMS_EXPORT FloatModelEditorBase : public QWidget, public FloatModelView
{
	Q_OBJECT

	mapPropertyFromModel(bool,isVolumeKnob,setVolumeKnob,m_volumeKnob);
	mapPropertyFromModel(float,volumeRatio,setVolumeRatio,m_volumeRatio);

	void initUi( const QString & _name ); //!< to be called by ctors

public:
	FloatModelEditorBase( QWidget * _parent = nullptr, const QString & _name = QString() ); //!< default ctor
	FloatModelEditorBase( const FloatModelEditorBase& other ) = delete;

	// TODO: remove
	inline void setHintText( const QString & _txt_before,
						const QString & _txt_after )
	{
		setDescription( _txt_before );
		setUnit( _txt_after );
	}

signals:
	void sliderPressed();
	void sliderReleased();
	void sliderMoved( float value );


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;
	void focusOutEvent( QFocusEvent * _fe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void mouseMoveEvent( QMouseEvent * _me ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _me ) override;
	void wheelEvent( QWheelEvent * _me ) override;

	virtual float getValue( const QPoint & _p );

private slots:
	virtual void enterValue();
	void friendlyUpdate();
	void toggleScale();

private:
	virtual QString displayValue() const;

	void doConnections() override;

	void setPosition( const QPoint & _p );

	inline float pageSize() const
	{
		return ( model()->maxValue() - model()->minValue() ) / 100.0f;
	}


	static SimpleTextFloat * s_textFloat;

	QTextDocument* m_tdRenderer;

	std::unique_ptr<QPixmap> m_knobPixmap;
	BoolModel m_volumeKnob;
	FloatModel m_volumeRatio;

	QPoint m_lastMousePos; //!< mouse position in last mouseMoveEvent
	float m_leftOver;
	bool m_buttonPressed;

	QImage m_cache;
};


} // namespace lmms::gui

#endif // LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H
