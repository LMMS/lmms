/*
 * AboutDialog.cpp - implementation of about-dialog
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsversion.h"
#include "AboutDialog.h"
#include "embed.h"
#include "engine.h"
#include "MainWindow.h"
#include "ui_AboutDialog.h"

#ifdef __GNUC__
#define GCC_VERSION "GCC "__VERSION__
#else
#define GCC_VERSION "unknown compiler"
#endif

#ifdef LMMS_HOST_X86
#define MACHINE "i386"
#endif

#ifdef LMMS_HOST_X86_64
#define MACHINE "x86_64"
#endif

#ifndef MACHINE
#define MACHINE "unknown processor"
#endif

#ifdef LMMS_BUILD_LINUX
#define PLATFORM "Linux"
#endif

#ifdef LMMS_BUILD_APPLE
#define PLATFORM "OS X"
#endif

#ifdef LMMS_BUILD_WIN32
#define PLATFORM "win32"
#endif


AboutDialog::AboutDialog() :
	QDialog( engine::mainWindow() ),
	ui( new Ui::AboutDialog )
{
	ui->setupUi( this );

	ui->iconLabel->setPixmap( embed::getIconPixmap( "icon" ) );

	ui->versionLabel->setText( ui->versionLabel->text().
									arg( LMMS_VERSION ).
									arg( PLATFORM ).
									arg( MACHINE ).
									arg( QT_VERSION_STR ).
									arg( GCC_VERSION ) );

	ui->authorLabel->setPlainText( embed::getText( "AUTHORS" ) );

	ui->licenseLabel->setPlainText( embed::getText( "COPYING" ) );
}



