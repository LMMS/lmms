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

#include "ClapManager.h"
#include "Engine.h"

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

	auto versionLabel = new QLabel(parent);
	versionLabel->setText(QWidget::tr("Version: ") + QString::fromUtf8(descriptor->version));

	auto urlLabel = new QLabel(parent);
	urlLabel->setText(QWidget::tr("URL: ") + QString::fromUtf8(descriptor->url));

	if (descriptor->manual_url && descriptor->manual_url[0] != '\0')
	{
		auto urlLabel = new QLabel(parent);
		urlLabel->setText(QWidget::tr("Manual URL: ") + QString::fromUtf8(descriptor->manual_url));
	}

	if (descriptor->support_url && descriptor->support_url[0] != '\0')
	{
		auto urlLabel = new QLabel(parent);
		urlLabel->setText(QWidget::tr("Support URL: ") + QString::fromUtf8(descriptor->support_url));
	}

	auto author = new QWidget(parent);
	auto l = new QHBoxLayout(author);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);

	auto authorLabel = new QLabel(author);
	authorLabel->setText(QWidget::tr("Author: "));
	authorLabel->setAlignment(Qt::AlignTop);

	auto authorContent = new QLabel(author);
	authorContent->setText(QString::fromUtf8(descriptor->vendor));
	authorContent->setWordWrap(true);

	l->addWidget(authorLabel);
	l->addWidget(authorContent, 1);

	auto copyright = new QWidget(parent);
	l = new QHBoxLayout(copyright);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	copyright->setMinimumWidth(parent->minimumWidth());

	auto copyrightLabel = new QLabel(copyright);
	copyrightLabel->setText(QWidget::tr("Copyright: "));
	copyrightLabel->setAlignment(Qt::AlignTop);

	auto copyrightContent = new QLabel(copyright);
	copyrightContent->setText("<unknown>");
	copyrightContent->setWordWrap(true);
	l->addWidget(copyrightLabel);
	l->addWidget(copyrightContent, 1);

	// Possibly TODO: project, plugin type, number of channels
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

auto ClapSubPluginFeatures::additionalFileExtensions(const Plugin::Descriptor::SubPluginFeatures::Key& key) const -> QString
{
	(void)key;
	// CLAP only loads .clap files
	return QString{};
}

auto ClapSubPluginFeatures::displayName(const Plugin::Descriptor::SubPluginFeatures::Key& key) const -> QString
{
	return QString::fromUtf8(getPlugin(key)->getDescriptor()->name);
}

auto ClapSubPluginFeatures::description(const Plugin::Descriptor::SubPluginFeatures::Key& key) const -> QString
{
	return QString::fromUtf8(getPlugin(key)->getDescriptor()->description);
}

auto ClapSubPluginFeatures::logo(const Plugin::Descriptor::SubPluginFeatures::Key& key) const -> const PixmapLoader*
{
	(void)key; // TODO
	return nullptr;
}

auto ClapSubPluginFeatures::getPlugin(const Plugin::Descriptor::SubPluginFeatures::Key& key) -> const ClapPlugin*
{
	const auto result = Engine::getClapManager()->getPlugin(key.attributes["uri"]);
	Q_ASSERT(result);
	return result;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
