/*
 * TimeInputDialog.cpp - dialog for input playback time
 *
 * Copyright (c) 2016 liushuyu <liushuyu011/at/gmail/dot/com>
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

#include "TimeInputDialog.h"

TimeInputDialog::TimeInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TimeInputDialog)
{
    s = Engine::getSong();
    ui->setupUi(this);
}

TimeInputDialog::~TimeInputDialog()
{
    delete ui;
}

void TimeInputDialog::setTimeModel(int time_mode)
{
    m_timemode = time_mode;
    switch (m_timemode) {
    case MinutesSeconds:
        ui->majorLabel->setText( tr( "MIN" ) );
        ui->minorLabel->setText( tr( "SEC" ) );
        ui->milliLabel->setText( tr( "MSEC" ) );
        setSpinRange(range(0, 9999), range(0, 59), range(0, 999));
        break;

    case BarsTicks:
        ui->majorLabel->setText( tr( "BAR" ) );
        ui->minorLabel->setText( tr( "BEAT" ) );
        ui->milliLabel->setText( tr( "TICK" ) );
        setSpinRange(range(1, 9999), range(1, s->getTimeSigModel().getNumerator()), range(0, s->ticksPerTact() / s->getTimeSigModel().getNumerator() - 1));
        break;

    default: break;
    }
    return;
}

void TimeInputDialog::setMilliSeconds( qint64 milliseconds )
{
    m_milliseconds = ( ( milliseconds > 0 ) ? milliseconds : 0 );
    qint64 ticks;
    switch (m_timemode) {
    case MinutesSeconds:
        ui->majorInput->setValue( m_milliseconds / 60000 );
        ui->minorInput->setValue( (m_milliseconds / 1000) % 60 );
        ui->milliInput->setValue( m_milliseconds % 1000 );
        break;
    case BarsTicks:
        ticks = millisecsToTicks( m_milliseconds, s->getTempo() );
        ui->majorInput->setValue( ticks / s->ticksPerTact() + 1 );
        ui->minorInput->setValue( ( ticks % s->ticksPerTact() ) /
                                  ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) +1);
        ui->milliInput->setValue( ( ticks % s->ticksPerTact() ) %
                                  ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) );
    default:
        break;
    }
}

qint64 TimeInputDialog::millisecsToTicks( qint64 milliseconds, qint32 tempo )
{
    return ( ( milliseconds * tempo * ( DefaultTicksPerTact / 4 ) ) / 60000 );
}

qint64 TimeInputDialog::totalTicks(qint32 bars, qint32 beats, qint32 ticks)
{
    qint64 ticksTotal = 0;
    ticksTotal += ( bars - 1 ) * s->ticksPerTact();
    ticksTotal += ( ( beats -1 ) * s->ticksPerTact() ) / s->getTimeSigModel().getNumerator();
    ticksTotal += ticks;
    return ticksTotal;
}

qint64 TimeInputDialog::totalMilliseconds(qint32 mins, qint32 secs, qint32 milli)
{
    return ( mins * 60000 + secs * 1000 + milli);
}

void TimeInputDialog::setSpinRange(range Major, range Minor, range Milli)
{
    ui->majorInput->setRange(Major.first, Major.second);
    ui->minorInput->setRange(Minor.first, Minor.second);
    ui->milliInput->setRange(Milli.first, Milli.second);
}

qint64 TimeInputDialog::getTicks()
{
    qint64 ticks = 0;
    switch (m_timemode) {
    case MinutesSeconds:
        ticks = millisecsToTicks( totalMilliseconds( ui->majorInput->value(), ui->minorInput->value(), ui->milliInput->value() ),  s->getTempo());
        return ( ticks > 0 ? ticks : 0 );  // Try to prevent from overflow
        break;

    case BarsTicks:
        ticks = totalTicks( ui->majorInput->value(), ui->minorInput->value(), ui->milliInput->value() );
        return ( ticks > 0 ? ticks : 0 );

    default:
        return 0;
        break;
    }
}
