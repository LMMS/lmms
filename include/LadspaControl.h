/*
 * LadspaControl.h - model for controlling a LADSPA port
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef LADSPA_CONTROL_H
#define LADSPA_CONTROL_H

#include <ladspa.h>

#include "AutomatableModel.h"
#include "TempoSyncKnobModel.h"


typedef struct PortDescription port_desc_t;


class EXPORT LadspaControl : public Model, public JournallingObject
{
	Q_OBJECT
public:
	LadspaControl( Model * _parent, port_desc_t * _port,
							bool _link = false );
	~LadspaControl();

	LADSPA_Data value();
	void setValue( LADSPA_Data _value );
	void setLink( bool _state );

	void linkControls( LadspaControl * _control );
	void unlinkControls( LadspaControl * _control );

	inline BoolModel * toggledModel()
	{
		return &m_toggledModel;
	}

	inline FloatModel * knobModel()
	{
		return &m_knobModel;
	}

	inline TempoSyncKnobModel * tempoSyncKnobModel()
	{
		return &m_tempoSyncKnobModel;
	}

	inline port_desc_t * port()
	{
		return m_port;
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent, const QString & _name );
	virtual void loadSettings( const QDomElement & _this, const QString & _name );
	inline virtual QString nodeName() const
	{
		return "port";
	}


signals:
	void changed( int _port, LADSPA_Data _value );
	void linkChanged( int _port, bool _state );


protected slots:
	void ledChanged();
	void knobChanged();
	void tempoKnobChanged();
	void linkStateChanged();

protected:
	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}



private:
	bool m_link;
	port_desc_t * m_port;

	BoolModel m_linkEnabledModel;
	BoolModel m_toggledModel;
	FloatModel m_knobModel;
	TempoSyncKnobModel m_tempoSyncKnobModel;


	friend class LadspaControlView;

} ;

#endif
