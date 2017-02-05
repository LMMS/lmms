/*
 * TimeInputDialog.h - dialog for input playback time
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

#ifndef TIMEINPUTDIALOG_H
#define TIMEINPUTDIALOG_H

#include <QDialog>
#include <QPair>

#include "Song.h"
#include "ui_timeinputdialog.h"

namespace Ui
{
class TimeInputDialog;
}
class TimeDisplayWidget;
class TimeInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimeInputDialog(QWidget *parent = 0);
    ~TimeInputDialog();
    void setTimeModel(int time_mode);
    void setMilliSeconds(qint64 milliseconds);
    qint64 getTicks();  // User input

private:
    enum DisplayModes {
        MinutesSeconds,
        BarsTicks,
        DisplayModeCount
    };
    typedef DisplayModes DisplayMode;

    Ui::TimeInputDialog *ui;
    qint64 millisecsToTicks(qint64 milliseconds, qint32 tempo);
    qint64 totalTicks(qint32 bars, qint32 beats, qint32 ticks);
    qint64 totalMilliseconds(qint32 mins, qint32 secs, qint32 milli);
    qint32 m_timemode;
    qint64 m_milliseconds;
    typedef QPair<qint32, qint32> range;
    Song* s;
    void setSpinRange(range Major, range Minor, range Milli);
};

#endif // TIMEINPUTDIALOG_H
