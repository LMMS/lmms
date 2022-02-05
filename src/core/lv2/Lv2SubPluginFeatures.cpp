/*
 * Lv2SubPluginFeatures.cpp - derivation from
 *                            Plugin::Descriptor::SubPluginFeatures for
 *                            hosting LV2 plugins
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2SubPluginFeatures.h"

#ifdef LMMS_HAVE_LV2

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>

#include "Engine.h"
#include "Lv2Basics.h"
#include "Lv2Manager.h"


const LilvPlugin *Lv2SubPluginFeatures::getPlugin(
	const Plugin::Descriptor::SubPluginFeatures::Key &k)
{
	const LilvPlugin* result = Engine::getLv2Manager()->
		getPlugin(k.attributes["uri"]);
	Q_ASSERT(result);
	return result;
}




QString Lv2SubPluginFeatures::pluginName(const LilvPlugin *plug)
{
	return qStringFromPluginNode(plug, lilv_plugin_get_name);
}




Lv2SubPluginFeatures::Lv2SubPluginFeatures(Plugin::PluginTypes type) :
	SubPluginFeatures(type)
{
}




void Lv2SubPluginFeatures::fillDescriptionWidget(QWidget *parent,
													const Key *k) const
{
	const LilvPlugin *plug = getPlugin(*k);

	QLabel *label = new QLabel(parent);
	label->setText(QWidget::tr("Name: ") + pluginName(plug));

	QLabel *label2 = new QLabel(parent);
	label2->setText(QWidget::tr("URI: ") +
		lilv_node_as_uri(lilv_plugin_get_uri(plug)));

	QWidget *maker = new QWidget(parent);
	QHBoxLayout *l = new QHBoxLayout(maker);
	l->setMargin(0);
	l->setSpacing(0);

	QLabel *maker_label = new QLabel(maker);
	maker_label->setText(QWidget::tr("Maker: "));
	maker_label->setAlignment(Qt::AlignTop);

	QLabel *maker_content = new QLabel(maker);
	maker_content->setText(
		qStringFromPluginNode(plug, lilv_plugin_get_author_name));
	maker_content->setWordWrap(true);

	l->addWidget(maker_label);
	l->addWidget(maker_content, 1);

	QWidget *copyright = new QWidget(parent);
	l = new QHBoxLayout(copyright);
	l->setMargin(0);
	l->setSpacing(0);
	copyright->setMinimumWidth(parent->minimumWidth());

	QLabel *copyright_label = new QLabel(copyright);
	copyright_label->setText(QWidget::tr("Copyright: "));
	copyright_label->setAlignment(Qt::AlignTop);

	QLabel *copyright_content = new QLabel(copyright);
	copyright_content->setText("<unknown>");
	copyright_content->setWordWrap(true);
	l->addWidget(copyright_label);
	l->addWidget(copyright_content, 1);

	AutoLilvNodes extensions(lilv_plugin_get_extension_data(plug));
	(void)extensions;
	// possibly TODO: version, project, plugin type, number of channels
}




QString Lv2SubPluginFeatures::additionalFileExtensions(
	const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	(void)k;
	// lv2 only loads .lv2 files
	// maybe add conversions later, e.g. for loading xmz
	return QString();
}




QString Lv2SubPluginFeatures::displayName(
	const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	return pluginName(getPlugin(k));
}




QString Lv2SubPluginFeatures::description(
	const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	(void)k;
	return QString::fromUtf8("description not implemented yet"); // TODO
}




const PixmapLoader *Lv2SubPluginFeatures::logo(
	const Plugin::Descriptor::SubPluginFeatures::Key &k) const
{
	(void)k; // TODO
	return nullptr;
}




void Lv2SubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor *desc,
												KeyList &kl) const
{
	Lv2Manager *lv2Mgr = Engine::getLv2Manager();
	for (const auto &uriInfoPair : *lv2Mgr)
	{
		if (uriInfoPair.second.type() == m_type && uriInfoPair.second.isValid())
		{
			using KeyType =
				Plugin::Descriptor::SubPluginFeatures::Key;
			KeyType::AttributeMap atm;
			atm["uri"] = QString::fromUtf8(uriInfoPair.first.c_str());
			const LilvPlugin* plug = uriInfoPair.second.plugin();

			kl.push_back(KeyType(desc, pluginName(plug), atm));
			//qDebug() << "Found LV2 sub plugin key of type" <<
			//	m_type << ":" << pr.first.c_str();
		}
	}
}

#endif // LMMS_HAVE_LV2

