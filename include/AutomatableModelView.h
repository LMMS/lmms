/*
 * AutomatableModelView.h - provides AutomatableModelView base class and
 * provides BoolModelView, FloatModelView, IntModelView subclasses.
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_AUTOMATABLE_MODEL_VIEW_H
#define LMMS_GUI_AUTOMATABLE_MODEL_VIEW_H

#include "ModelView.h"
#include "AutomatableModel.h"

class QMenu;
class QMouseEvent;

namespace lmms::gui
{

class LMMS_EXPORT AutomatableModelView : public ModelView
{
public:
	AutomatableModelView( Model* model, QWidget* _this );
	~AutomatableModelView() override = default;

	// some basic functions for convenience
	AutomatableModel* modelUntyped()
	{
		return castModel<AutomatableModel>();
	}

	const AutomatableModel* modelUntyped() const
	{
		return castModel<AutomatableModel>();
	}

	void setModel( Model* model, bool isOldModelValid = true ) override;
	void unsetModel() override;

	template<typename T>
	inline T value() const
	{
		return modelUntyped() ? modelUntyped()->value<T>() : 0;
	}

	inline void setDescription( const QString& desc )
	{
		m_description = desc;
	}

	inline void setUnit( const QString& unit )
	{
		m_unit = unit;
	}

	void addDefaultActions( QMenu* menu );

	void setConversionFactor( float factor );
	float getConversionFactor();


protected:
	virtual void mousePressEvent( QMouseEvent* event );

	QString m_description;
	QString m_unit;
	float m_conversionFactor; // Factor to be applied when the m_model->value is displayed
} ;




class AutomatableModelViewSlots : public QObject
{
	Q_OBJECT
public:
	AutomatableModelViewSlots( AutomatableModelView* amv, QObject* parent );

public slots:
	void execConnectionDialog();
	void removeConnection();
	void editSongGlobalAutomation();
	void unlinkAllModels();
	void removeSongGlobalAutomation();

private slots:
	/// Copy the model's value to the clipboard.
	void copyToClipboard();
	/// Paste the model's value from the clipboard.
	void pasteFromClipboard();

protected:
	AutomatableModelView* m_amv;

} ;



template <typename ModelType> class LMMS_EXPORT TypedModelView : public AutomatableModelView
{
public:
	TypedModelView( Model* model, QWidget* _this) :
		AutomatableModelView( model, _this )
	{}

	ModelType* model()
	{
		return castModel<ModelType>();
	}
	const ModelType* model() const
	{
		return castModel<ModelType>();
	}
};

using FloatModelView = TypedModelView<FloatModel>;
using IntModelView = TypedModelView<IntModel>;
using BoolModelView = TypedModelView<BoolModel>;

} // namespace lmms::gui

#endif // LMMS_GUI_AUTOMATABLE_MODEL_VIEW_H
