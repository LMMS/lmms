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

#include "FileBrowser.h"

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QMimeData>
#include <QUrl>

#include "Clipboard.h"
#include "StringPairDrag.h"

namespace lmms::Clipboard
{

	const QStringList projectExtensions{"mmp", "mpt", "mmpz"};
	const QStringList presetExtensions{"xpf", "xml", "xiz", "lv2"};
	const QStringList soundFontExtensions{"sf2", "sf3"};
	const QStringList patchExtensions{"pat"};
	const QStringList midiExtensions{"mid", "midi", "rmi"};
	#ifdef LMMS_BUILD_WINDOWS
	const QStringList vstPluginExtensions{"dll"};
	#else
	const QStringList vstPluginExtensions{"dll", "so"};
	#endif
	QStringList audioExtensions{};

	inline QString getExtension(const QString& file)
	{
		const QStringList parts = file.split('.');
		return parts.isEmpty() ? file.toLower() : parts.last().toLower();
	}

	void updateExtensionLists()
	{
		for (const SampleDecoder::AudioType& at : SampleDecoder::supportedAudioTypes())
		{
			audioExtensions += QString::fromStdString(at.extension);
		}
	}

	bool isAudioFile(const QString& ext)
	{
		if (audioExtensions.isEmpty())
		{
			updateExtensionLists();
		}

		return audioExtensions.contains(getExtension(ext));
	}
	bool isProjectFile(const QString& ext)   { return projectExtensions.contains(getExtension(ext)); }
	bool isPresetFile(const QString& ext)    { return presetExtensions.contains(getExtension(ext)); }
	bool isSoundFontFile(const QString& ext) { return soundFontExtensions.contains(getExtension(ext)); }
	bool isPatchFile(const QString& ext)     { return patchExtensions.contains(getExtension(ext)); }
	bool isMidiFile(const QString& ext)      { return midiExtensions.contains(getExtension(ext)); }
	bool isVstPluginFile(const QString& ext) { return vstPluginExtensions.contains(getExtension(ext)); }

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




	void copyStringPair( const QString & key, const QString & value )
	{
		QString finalString = key + ":" + value;

		auto content = new QMimeData;
		content->setData( mimeType( MimeType::StringPair ), finalString.toUtf8() );
		QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
	}




	QString decodeKey( const QMimeData * mimeData )
	{
		return( QString::fromUtf8( mimeData->data( mimeType( MimeType::StringPair ) ) ).section( ':', 0, 0 ) );
	}




	QString decodeValue( const QMimeData * mimeData )
	{
		return( QString::fromUtf8( mimeData->data( mimeType( MimeType::StringPair ) ) ).section( ':', 1, -1 ) );
	}

	std::pair<QString, QString> decodeMimeData(const QMimeData* mimeData)
	{
		const QList<QUrl> urls = mimeData->urls();

		QString type = decodeKey(mimeData);
		QString value = decodeValue(mimeData);

		if (!urls.isEmpty())
		{
			value = urls.first().toLocalFile();
		}

		if (isAudioFile(value))			 { type = "samplefile"; }
		else if (isVstPluginFile(value)) { type = "vstpluginfile"; }
		else if (isPresetFile(value))    { type = "presetfile"; }
		else if (isMidiFile(value))      { type = "importedproject"; }
		else if (isProjectFile(value))   { type = "projectfile"; }
		else if (isPatchFile(value))     { type = "patchfile"; }
		else if (isSoundFontFile(value)) { type = "soundfontfile"; }

		return {type, value};
	}

	void startFileDrag(gui::FileItem* f, QObject* qo)
	{
		if (f == nullptr) { return; }

		auto drag = new QDrag(qo);
		auto mimeData = new QMimeData();

		QString internalType;
		QString iconName;

		switch (f->type())
		{
		case gui::FileItem::FileType::Preset:
			internalType = f->handling() == gui::FileItem::FileHandling::LoadAsPreset ? "presetfile" : "pluginpresetfile";
			iconName = "preset_file";
			break;
		case gui::FileItem::FileType::Sample:
			internalType = "samplefile";
			iconName = "sample_file";
			break;
		case gui::FileItem::FileType::SoundFont:
			internalType = "soundfontfile";
			iconName = "soundfont_file";
			break;
		case gui::FileItem::FileType::Patch:
			internalType = "patchfile";
			iconName = "sample_file";
			break;
		case gui::FileItem::FileType::VstPlugin:
			internalType = "vstpluginfile";
			iconName = "vst_plugin_file";
			break;
		case gui::FileItem::FileType::Midi:
			internalType = "importedproject";
			iconName = "midi_file";
			break;
		case gui::FileItem::FileType::Project:
			internalType = "projectfile";
			iconName = "project_file";
			break;
		default:
			return;
		}

		QString filePath = QUrl::fromLocalFile(f->fullName()).toString();

		// Internal LMMS type
		mimeData->setData("application/x-lmms-type", internalType.toUtf8());
		mimeData->setData("application/x-lmms-path", f->fullName().toUtf8());

		// For external applications
		QList<QUrl> urls;
		urls << QUrl::fromLocalFile(f->fullName());
		mimeData->setUrls(urls); // This sets the "text/uri-list" MIME type

		drag->setMimeData(mimeData);
		drag->setPixmap(embed::getIconPixmap(iconName.toStdString()));
		drag->exec(Qt::CopyAction);
	}



} // namespace lmms::Clipboard
