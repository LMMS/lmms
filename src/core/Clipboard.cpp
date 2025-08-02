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

#include "Clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>

#include "FileBrowser.h"
#include "PluginFactory.h"
#include "SampleDecoder.h"
#include "StringPairDrag.h"

namespace lmms::Clipboard
{

static std::map<std::string, std::vector<std::string>> mimetypes = {{"samplefile", {}}, {"vstpluginfile", {}},
	{"presetfile", {"xpf", "xml", "xiz", "lv2"}}, {"midifile", {"mid", "midi", "rmi"}},
	{"projectfile", {"mmp", "mpt", "mmpz"}}, {"patchfile", {"pat"}}, {"soundfontfile", {"sf2", "sf3"}}};

//! gets the extension of a file, or returns the string back if no extension is found
	inline QString getExtension(const QString& file)
	{
		const QStringList parts = file.split('.');
		return parts.isEmpty() ? file.toLower() : parts.last().toLower();
	}

	/* @brief updates the extension map.
	 */
	void updateExtensionMap()
	{
		for (auto& pluginInfo : PluginFactory::instance()->pluginInfos())
		{
			const char* mimetype = pluginInfo.descriptor->supportedMimetype;

			if (mimetype == nullptr || mimetype[0] == '\0') { continue; }

			std::vector<std::string> fileTypes = {};

			for (auto& fileType : QString(pluginInfo.descriptor->supportedFileTypes).split(","))
			{
				fileTypes.push_back(fileType.toStdString());
				qDebug() << fileType;
			}

			mimetypes.insert_or_assign(mimetype, fileTypes);
		}
	}

	bool isType(const QString& ext, const QString& mimetype)
	{
		auto& fileTypes = mimetypes.find(mimetype.toStdString())->second;
		if (fileTypes.empty()) { updateExtensionMap(); }

		return std::ranges::find(fileTypes, getExtension(ext).toStdString()) != fileTypes.end();
	}

	bool isAudioFile(const QString& ext)	 { return isType(ext, "samplefile"); }
	bool isProjectFile(const QString& ext)   { return isType(ext, "midifile"); }
	bool isPresetFile(const QString& ext)    { return isType(ext, "presetfile"); }
	bool isSoundFontFile(const QString& ext) { return isType(ext, "soundfontfile"); }
	bool isPatchFile(const QString& ext)     { return isType(ext, "patchfile"); }
	bool isMidiFile(const QString& ext)      { return isType(ext, "midifile"); }
	bool isVstPluginFile(const QString& ext) { return isType(ext, "vstpluginfile"); }

	const QMimeData* getMimeData()
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

		if (isAudioFile(value)) { type = "samplefile"; }
		else if (isVstPluginFile(value)) { type = "vstpluginfile"; }
		else if (isPresetFile(value)) { type = "presetfile"; }
		else if (isMidiFile(value)) { type = "midifile"; }
		else if (isProjectFile(value)) { type = "projectfile"; }
		else if (isPatchFile(value)) { type = "patchfile"; }
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
			internalType = "midifile";
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
