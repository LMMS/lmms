/*
 * ClapSubPluginFeatures.cpp - derivation from
 *                             Plugin::Descriptor::SubPluginFeatures for
 *                             hosting CLAP plugins
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapSubPluginFeatures.h"

#ifdef LMMS_HAVE_CLAP

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

#include "Engine.h"
#include "ClapManager.h"

namespace lmms
{


ClapSubPluginFeatures::ClapSubPluginFeatures(Plugin::PluginTypes type)
	: SubPluginFeatures(type)
{
}

void ClapSubPluginFeatures::fillDescriptionWidget(QWidget* parent, const Key* key) const
{
	const auto descriptor = getPlugin(*key)->getDescriptor();

	auto label = new QLabel(parent);
	label->setText(QWidget::tr("Name: ") + QString::fromUtf8(descriptor->name));

	auto label2 = new QLabel(parent);
	label2->setText(QWidget::tr("URI: ") + QString::fromUtf8(descriptor->id));

	auto maker = new QWidget(parent);
	auto l = new QHBoxLayout(maker);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);

	auto maker_label = new QLabel(maker);
	maker_label->setText(QWidget::tr("Maker: "));
	maker_label->setAlignment(Qt::AlignTop);

	auto maker_content = new QLabel(maker);
	maker_content->setText(QString::fromUtf8(descriptor->vendor));
	maker_content->setWordWrap(true);

	l->addWidget(maker_label);
	l->addWidget(maker_content, 1);

	auto copyright = new QWidget(parent);
	l = new QHBoxLayout(copyright);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	copyright->setMinimumWidth(parent->minimumWidth());

	auto copyright_label = new QLabel(copyright);
	copyright_label->setText(QWidget::tr("Copyright: "));
	copyright_label->setAlignment(Qt::AlignTop);

	auto copyright_content = new QLabel(copyright);
	copyright_content->setText("<unknown>");
	copyright_content->setWordWrap(true);
	l->addWidget(copyright_label);
	l->addWidget(copyright_content, 1);

	// TODO: extensions?

	// possibly TODO: version, project, plugin type, number of channels
}

QString ClapSubPluginFeatures::additionalFileExtensions(const Plugin::Descriptor::SubPluginFeatures::Key& key) const
{
	(void)key;
	// CLAP only loads .clap files
	return QString{};
}

QString ClapSubPluginFeatures::displayName(const Plugin::Descriptor::SubPluginFeatures::Key& key) const
{
	return QString::fromUtf8(getPlugin(key)->getDescriptor()->name);
}

QString ClapSubPluginFeatures::description(const Plugin::Descriptor::SubPluginFeatures::Key& key) const
{
	return QString::fromUtf8(getPlugin(key)->getDescriptor()->description);
}

const PixmapLoader* ClapSubPluginFeatures::logo(const Plugin::Descriptor::SubPluginFeatures::Key& key) const
{
	(void)key; // TODO
	return nullptr;
}

void ClapSubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* desc, KeyList& kl) const
{
	qDebug() << "ClapSubPluginFeatures::listSubPluginKeys";
	const auto& manager = *Engine::getClapManager();
	for (const auto& [uri, plugin] : manager)
	{
		if (plugin->getType() == m_type && plugin->isValid())
		{
			using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
			KeyType::AttributeMap atm;
			atm["uri"] = QString::fromUtf8(uri.c_str());

			kl.push_back(KeyType{desc, QString::fromUtf8(plugin->getDescriptor()->name), atm});
			qDebug() << "Found CLAP sub plugin key of type" <<
				m_type << ":" << uri.c_str();
		}
	}
}

const ClapPlugin* ClapSubPluginFeatures::getPlugin(const Plugin::Descriptor::SubPluginFeatures::Key& key)
{
	const auto result = Engine::getClapManager()->getPlugin(key.attributes["uri"]);
	Q_ASSERT(result);
	return result;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
