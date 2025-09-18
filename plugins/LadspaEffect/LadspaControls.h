/*
 * LadspaControls.h - model for LADSPA plugin controls
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

#ifndef LADSPA_CONTROLS_H
#define LADSPA_CONTROLS_H

#include "EffectControls.h"
#include "LadspaMatrixControlDialog.h"

namespace lmms
{


class LadspaControl;
using control_list_t = QVector<LadspaControl*>;
namespace gui {
class LadspaControlDialog;
}
class LadspaEffect;


class LadspaControls : public EffectControls
{
	Q_OBJECT
public:
	LadspaControls( LadspaEffect * _eff );
	~LadspaControls() override;

	inline int controlCount() override
	{
		return m_controlCount;
	}

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "ladspacontrols";
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::LadspaMatrixControlDialog( this );
	}


protected slots:
	void updateLinkStatesFromGlobal();
	void linkPort( int _port, bool _state );


private:
	LadspaEffect * m_effect;
	ch_cnt_t m_processors;
	ch_cnt_t m_controlCount;
	bool m_noLink;
	BoolModel m_stereoLinkModel;
	//! control vector for each processor
	QVector<control_list_t> m_controls;


	friend class gui::LadspaControlDialog;
	friend class gui::LadspaMatrixControlDialog;
	friend class LadspaEffect;


signals:
	void effectModelChanged( lmms::LadspaControls * );

} ;


} // namespace lmms

#endif
