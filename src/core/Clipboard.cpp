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


	QString getStringPairKeyName(DataType type)
	{
		switch (type)
		{
			case DataType::All:
				return QString("All");
			case DataType::Any:
				return QString("Any");
			case DataType::FloatValue:
				return QString("FloatValue");
			case DataType::AutomatableModelLink:
				return QString("AutomatableModelLink");
			case DataType::Instrument:
				return QString("Instrument");
			case DataType::PresetFile:
				return QString("PresetFile");
			case DataType::PluginPresetFile:
				return QString("PluginPresetFile");
			case DataType::SampleFile:
				return QString("SampleFile");
			case DataType::SoundFontFile:
				return QString("SoundFontFile");
			case DataType::PatchFile:
				return QString("PatchFile");
			case DataType::VstPluginFile:
				return QString("VstPluginFile");
			case DataType::ImportedProject:
				return QString("ImportedProject");
			case DataType::ProjectFile:
				return QString("ProjectFile");
			case DataType::SampleData:
				return QString("SampleData");
			case DataType::InstrumentTrack:
				return QString("InstrumentTrack");
			case DataType::PatternTrack:
				return QString("PatternTrack");
			case DataType::SampleTrack:
				return QString("SampleTrack");
			case DataType::AutomationTrack:
				return QString("AutomationTrack");
			case DataType::HiddenAutomationTrack:
				return QString("HiddenAutomationTrack");
			case DataType::MidiClip:
				return QString("MidiClip");
			case DataType::PatternClip:
				return QString("PatternClip");
			case DataType::SampleClip:
				return QString("SampleClip");
			case DataType::AutomationClip:
				return QString("AutomationClip");
			default:
				break;
		};
		return QString("Error");
	}


	void copyStringPair(DataType key, const QString& value)
	{
		QString finalString = getStringPairKeyName(key) + ":" + value;

		auto content = new QMimeData;
		content->setData( mimeType( MimeType::StringPair ), finalString.toUtf8() );
		QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
	}




	DataType decodeKey(const QMimeData* mimeData)
	{
		if (Clipboard::hasFormat(Clipboard::MimeType::StringPair) == false) { return DataType::Error; }

		QString keyString = QString::fromUtf8(mimeData->data(mimeType(MimeType::StringPair))).section(':', 0, 0);
		for (size_t i = 0; i < static_cast<size_t>(DataType::Count); i++)
		{
			if (getStringPairKeyName(static_cast<DataType>(i)) == keyString)
			{
				return static_cast<DataType>(i);
			}
		}
		return DataType::Error;
	}




	QString decodeValue( const QMimeData * mimeData )
	{
		return( QString::fromUtf8( mimeData->data( mimeType( MimeType::StringPair ) ) ).section( ':', 1, -1 ) );
	}
	
	QString encodeFloatValue(float value)
	{
		return QString::number(value);
	}
	
	QString encodeAutomatableModelLink(const AutomatableModel& model)
	{
		return QString::number(model.id());
	}


} // namespace lmms::Clipboard
