/*
 * Clipboard.h - the clipboard for clips, notes etc.
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_CLIPBOARD_H
#define LMMS_CLIPBOARD_H

#include <QDomElement>
#include <QMap>

#include "lmms_export.h"

class QMimeData;

namespace lmms
{

class AutomatableModel;

namespace Clipboard
{

	enum class MimeType
	{
		StringPair,
		Default
	};
	
	enum StringPairDataType
	{
		None, //!< only use for error handling
		FloatValue,
		AutomatableModelLink,
		Instrument,
		
		PresetFile,
		PluginPresetFile,
		SampleFile,
		SoundFontFile,
		PatchFile,
		VstPluginFile,
		ImportedProject,
		ProjectFile,
		
		SampleData,
		
		InstrumentTrack,
		PatternTrack,
		SampleTrack,
		AutomationTrack,
		HiddenAutomationTrack,
		
		MidiClip,
		PatternClip,
		SampleClip,
		AutomationClip,
		
		Count //!< utility, do not use
	};

	// Convenience Methods
	const QMimeData * getMimeData();
	bool hasFormat( MimeType mT );

	// Helper methods for String data
	void LMMS_EXPORT copyString(const QString& str, MimeType mT);
	QString getString( MimeType mT );

	// Helper methods for String Pair data
	QString getStringPairKeyName(StringPairDataType type);
	void copyStringPair(StringPairDataType key, const QString& value);
	StringPairDataType decodeKey(const QMimeData* mimeData);
	QString decodeValue( const QMimeData * mimeData );

	QString encodeFloatValue(float value);
	QString encodeAutomatableModelLink(const AutomatableModel& model);

	inline const char * mimeType( MimeType type )
	{
		switch( type )
		{
			case MimeType::StringPair:
				return "application/x-lmms-stringpair";
			break;
			case MimeType::Default:
			default:
				return "application/x-lmms-clipboard";
				break;
		}
	}
} // namespace Clipboard

} // namespace lmms

#endif // LMMS_CLIPBOARD_H
