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

#include <array>

#include <QDomElement>
#include <QMap>

#include "lmms_export.h"

class QMimeData;

namespace lmms::Clipboard
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
		AutomationClip
	};

	// Convenience Methods
	const QMimeData * getMimeData();
	bool hasFormat( MimeType mT );

	// Helper methods for String data
	void LMMS_EXPORT copyString(const QString& str, MimeType mT);
	QString getString( MimeType mT );

	// Helper methods for String Pair data
	void copyStringPair(StringPairDataType key, const QString& value);
	StringPairDataType decodeKey(const QMimeData* mimeData);
	QString decodeValue( const QMimeData * mimeData );

	QString clipboardEncodeFloatValue(float value);
	QString clipboardEncodeAutomatableModelLink(size_t id);

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

	const std::array<QString, 22> StringPairDataTypeNames = {
		QString("None_error"),
		QString("FloatValue"),
		QString("AutomatableModelLink"),
		QString("Instrument"),
		
		QString("PresetFile"),
		QString("PluginPresetFile"),
		QString("SampleFile"),
		QString("SoundFontFile"),
		QString("PatchFile"),
		QString("VstPluginFile"),
		QString("ImportedProject"),
		QString("ProjectFile"),
		
		QString("SampleData"),
		
		QString("InstrumentTrack"),
		QString("PatternTrack"),
		QString("SampleTrack"),
		QString("AutomationTrack"),
		QString("HiddenAutomationTrack"),
		
		QString("MidiClip"),
		QString("PatternClip"),
		QString("SampleClip"),
		QString("AutomationClip")
	};

} // namespace lmms::Clipboard

#endif // LMMS_CLIPBOARD_H
