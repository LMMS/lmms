/*
 * communication.h - header file defining stuff concerning communication between
 *                   LVSL-server and -client
 *
 * Copyright (c) 2005-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#include "RemotePlugin.h"


struct VstParameterDumpItem
{
	int32_t index;
	std::string shortLabel;
	float value;
} ;



enum VstHostLanguages
{
	LanguageEnglish = 1,
	LanguageGerman,
	LanguageFrench,
	LanguageItalian,
	LanguageSpanish,
	LanguageJapanese,
	LanguageKorean
} ;



enum VstRemoteMessageIDs
{
	// vstPlugin -> remoteVstPlugin
	IdVstLoadPlugin = IdUserBase,
	IdVstPluginWindowInformation,
	IdVstClosePlugin,
	IdVstSetTempo,
	IdVstSetLanguage,
	IdVstGetParameterCount,
	IdVstGetParameterDump,
	IdVstSetParameterDump,
	IdVstGetParameterProperties,
	IdVstProgramNames,
	IdVstCurrentProgram,
	IdVstCurrentProgramName,
	IdVstSetProgram,
	IdVstRotateProgram,
	IdVstIdleUpdate,

	// remoteVstPlugin -> vstPlugin
	IdVstFailedLoadingPlugin,
	IdVstBadDllFormat,
	IdVstPluginWindowID,
	IdVstPluginEditorGeometry,
	IdVstPluginName,
	IdVstPluginVersion,
	IdVstPluginVendorString,
	IdVstPluginProductString,
	IdVstPluginPresetsString,
	IdVstPluginUniqueID,
	IdVstSetParameter,
	IdVstParameterCount,
	IdVstParameterDump,
	IdVstParameterProperties

} ;



#endif
