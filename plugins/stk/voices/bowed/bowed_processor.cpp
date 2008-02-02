/*
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include "bowed_processor.h"
#include "config_mgr.h"


bowedProcessor::bowedProcessor( sample_rate_t _sample_rate ):
	stkProcessor<bowedModel, Bowed>( _sample_rate, 20.0f )
{
}




bowedProcessor::~bowedProcessor()
{
}




void bowedProcessor::setControls( bowedModel * _model )
{
	voice()->controlChange( 2, _model->bowPressure()->value() );
	voice()->controlChange( 4, _model->bowPosition()->value() );
	voice()->controlChange( 11, _model->vibratoFrequency()->value() );
	voice()->controlChange( 1, _model->vibratoGain()->value() );
	m_delayRead = m_delayWrite - static_cast<Uint8>( _model->spread()->value() );
}
