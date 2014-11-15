/*
 * delaycontrols.cpp - definition of DelayControls class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include <QtXml/QDomElement>

#include "delaycontrols.h"
#include "delayeffect.h"
#include "engine.h"
#include "song.h"

DelayControls::DelayControls(DelayEffect* effect):
    EffectControls( effect ),
    m_effect ( effect ),
    m_delayTimeModel( 2.0, 0.01, 20.0, 0.0001, 20000.0, this, tr( "Delay Samples" )) ,
    m_feebackModel(0.0f,0.0f,1.0f,0.01f,this,tr( "Feedback" ) ),
    m_lfoTimeModel(2.0, 0.01, 20.0, 0.0001, 20000.0, this, tr( "Lfo Frequency" ) ),
    m_lfoAmmountModel(0.0f,0.0f,0.5f,0.01f, this, tr ( "Lfo Ammount" ) )
{

}




void DelayControls::changeControl()
{
    //engine::getSong()->setModified();
}




void DelayControls::loadSettings(const QDomElement &_this)
{
    m_delayTimeModel.loadSettings(_this, "DelayTimeSamples" );
    m_feebackModel.loadSettings( _this, "FeebackAmmount" );
    m_lfoTimeModel.loadSettings( _this , "LfoFrequency");
    m_lfoAmmountModel.loadSettings( _this, "LfoAmmount");
}




void DelayControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
    m_delayTimeModel.saveSettings( doc, _this, "DelayTimeSamples");
    m_feebackModel.saveSettings( doc, _this ,"FeebackAmmount");
    m_lfoTimeModel.saveSettings( doc, _this, "LfoFrequency");
    m_lfoAmmountModel.saveSettings( doc, _this ,"LfoAmmount");
}

#include "moc_delaycontrols.cxx"
