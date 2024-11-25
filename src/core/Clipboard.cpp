/*
 * Clipboard.cpp - the clipboard for clips, notes etc.
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

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include "Clipboard.h"

#include "AutomatableModel.h"


namespace lmms::Clipboard
{

	const QMimeData * getMimeData()
	{
		return QApplication::clipboard()->mimeData( QClipboard::Clipboard );
	}




	bool hasFormat( MimeType mT )
	{
		return getMimeData()->hasFormat( mimeType( mT ) );
	}




	void copyString( const QString & str, MimeType mT )
	{
		auto content = new QMimeData;

		content->setData( mimeType( mT ), str.toUtf8() );
		QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
	}




	QString getString( MimeType mT )
	{
		return QString( getMimeData()->data( mimeType( mT ) ) );
	}


	QString getStringPairKeyName(StringPairDataType type)
	{
		switch (type)
		{
			case StringPairDataType::FloatValue:
				return QString("FloatValue");
			case StringPairDataType::AutomatableModelLink:
				return QString("AutomatableModelLink");
			case StringPairDataType::Instrument:
				return QString("Instrument");
			case StringPairDataType::PresetFile:
				return QString("PresetFile");
			case StringPairDataType::PluginPresetFile:
				return QString("PluginPresetFile");
			case StringPairDataType::SampleFile:
				return QString("SampleFile");
			case StringPairDataType::SoundFontFile:
				return QString("SoundFontFile");
			case StringPairDataType::PatchFile:
				return QString("PatchFile");
			case StringPairDataType::VstPluginFile:
				return QString("VstPluginFile");
			case StringPairDataType::ImportedProject:
				return QString("ImportedProject");
			case StringPairDataType::ProjectFile:
				return QString("ProjectFile");
			case StringPairDataType::SampleData:
				return QString("SampleData");
			case StringPairDataType::InstrumentTrack:
				return QString("InstrumentTrack");
			case StringPairDataType::PatternTrack:
				return QString("PatternTrack");
			case StringPairDataType::SampleTrack:
				return QString("SampleTrack");
			case StringPairDataType::AutomationTrack:
				return QString("AutomationTrack");
			case StringPairDataType::HiddenAutomationTrack:
				return QString("HiddenAutomationTrack");
			case StringPairDataType::MidiClip:
				return QString("MidiClip");
			case StringPairDataType::PatternClip:
				return QString("PatternClip");
			case StringPairDataType::SampleClip:
				return QString("SampleClip");
			case StringPairDataType::AutomationClip:
				return QString("AutomationClip");
			default:
				break;
		};
		return QString("None_error");
	}


	void copyStringPair(StringPairDataType key, const QString& value)
	{
		QString finalString = getStringPairKeyName(key) + ":" + value;

		auto content = new QMimeData;
		content->setData( mimeType( MimeType::StringPair ), finalString.toUtf8() );
		QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
	}




	StringPairDataType decodeKey(const QMimeData* mimeData)
	{
		QString keyString = QString::fromUtf8(mimeData->data(mimeType(MimeType::StringPair))).section(':', 0, 0);
		for (size_t i = 0; i < static_cast<size_t>(StringPairDataType::Count); i++)
		{
			if (getStringPairKeyName(static_cast<StringPairDataType>(i)) == keyString)
			{
				return static_cast<StringPairDataType>(i);
			}
		}
		return StringPairDataType::None;
	}




	QString decodeValue( const QMimeData * mimeData )
	{
		return( QString::fromUtf8( mimeData->data( mimeType( MimeType::StringPair ) ) ).section( ':', 1, -1 ) );
	}
	
	QString encodeFloatValue(float value)
	{
		qDebug("encodeFloatValue, outout: %f", value);
		return QString::number(value);
	}
	
	QString encodeAutomatableModelLink(const AutomatableModel& model)
	{
		qDebug("encodeAutomatableModelLink, outout: %d", model.id());
		return QString::number(model.id());
	}


} // namespace lmms::Clipboard
