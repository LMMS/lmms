/*
 * Lv2SubPluginFeatures.cpp - derivation from
 *                            Plugin::Descriptor::SubPluginFeatures for
 *                            hosting LV2 plugins
 *
 * Copyright (c) 2018-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include <QHBoxLayout>
#include <QLabel>

#include "Engine.h"
#include "Lv2Basics.h"
#include "Lv2Manager.h"

namespace lmms
{


const LilvPlugin* Lv2SubPluginFeatures::getPlugin(
	const Plugin::Descriptor::SubPluginFeatures::Key& k)
{
	const LilvPlugin* result = Engine::getLv2Manager()->
		getPlugin(k.attributes["uri"]);
	Q_ASSERT(result);
	return result;
}




QString Lv2SubPluginFeatures::pluginName(const LilvPlugin* plug)
{
	return qStringFromPluginNode(plug, lilv_plugin_get_name);
}




Lv2SubPluginFeatures::Lv2SubPluginFeatures(Plugin::Type type) :
	SubPluginFeatures(type)
{
}




static void addHbox(QWidget* parent, QString left, QString right)
{
	if (right.isEmpty()) { return; }

	auto container = new QWidget(parent);
	auto l = new QHBoxLayout(container);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);

	auto leftLabel = new QLabel(container);
	leftLabel->setText(left);
	leftLabel->setAlignment(Qt::AlignTop);

	auto rightLabel = new QLabel(container);
	if (right.startsWith("http") && !right.contains(' ') && !right.contains('\n'))
	{
		right = QString("<a href=\"%1\">%1</a>").arg(right);
		rightLabel->setTextInteractionFlags(rightLabel->textInteractionFlags()
			| Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
		rightLabel->setTextFormat(Qt::RichText);
		rightLabel->setOpenExternalLinks(true);
	}
	rightLabel->setText(right);
	rightLabel->setWordWrap(true);

	l->addWidget(leftLabel);
	l->addWidget(rightLabel, 1);
}




static void addLabel(QWidget* parent, QString left, QString right)
{
	auto lbl = new QLabel(parent);
	if (right.isEmpty()) { return; }
	if (right.startsWith("http") && !right.contains(' ') && !right.contains('\n'))
	{
		right = QString("<a href=\"%1\">%1</a>").arg(right);
		lbl->setTextInteractionFlags(lbl->textInteractionFlags()
			| Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
		lbl->setTextFormat(Qt::RichText);
		lbl->setOpenExternalLinks(true);
	}
	lbl->setText(left + right);
}




AutoLilvNodes pluginGetValues(const LilvPlugin* plug, const char* valueUri)
{
	assert(plug);
	AutoLilvNode valueUriNode{Engine::getLv2Manager()->uri(valueUri)};
	return AutoLilvNodes{lilv_plugin_get_value(plug, valueUriNode.get())};
}




void Lv2SubPluginFeatures::fillDescriptionWidget(QWidget* parent,
													const Key* k) const
{
	const LilvPlugin* plug = getPlugin(*k);

	QString pluginNameAndVersion = "<b>" + pluginName(plug) + "</b>";
	{
		AutoLilvNodes minorVersions = pluginGetValues(plug, LILV_NS_LV2 "minorVersion");
		AutoLilvNodes microVersions = pluginGetValues(plug, LILV_NS_LV2 "microVersion");
		if (minorVersions && microVersions)
		{
			QString min = lilv_node_as_string(lilv_nodes_get_first(minorVersions.get()));
			QString mic = lilv_node_as_string(lilv_nodes_get_first(microVersions.get()));
			pluginNameAndVersion += QString(" v%1.%2").arg(min).arg(mic);
		}
	}

	(new QLabel(parent))->setText(pluginNameAndVersion);
	{
		AutoLilvNodes comments = pluginGetValues(plug, LILV_NS_RDFS "comment");
		if (comments)
		{
			QString description{lilv_node_as_string(lilv_nodes_get_first(comments.get()))};
			auto descLabel = new QLabel(parent);
			descLabel->setText("<i>" + description.trimmed() + "</i>");
			descLabel->setWordWrap(true);
		}
	}

	addLabel(parent, QObject::tr("URI: "), lilv_node_as_uri(lilv_plugin_get_uri(plug)));
	addHbox(parent, QObject::tr("Project: "),
		qStringFromPluginNode(plug, lilv_plugin_get_project));
	addHbox(parent, QObject::tr("Maker: "),
		qStringFromPluginNode(plug, lilv_plugin_get_author_name));
	{
		AutoLilvNodes homepages = pluginGetValues(plug, LILV_NS_DOAP "homepage");
		if (homepages)
		{
			const char* homepage = lilv_node_as_uri(lilv_nodes_get_first(homepages.get()));
			QString homepageStr{homepage};
			addLabel(parent, QObject::tr("Homepage: "), homepageStr);
		}
	}
	{
		AutoLilvNodes licenses = pluginGetValues(plug, LILV_NS_DOAP "license");
		addLabel(parent, QObject::tr("License: "),
			licenses
			? lilv_node_as_uri(lilv_nodes_get_first(licenses.get()))
			: "<unknown>");
	}
	{
		const LilvNode* libraryUriNode = lilv_plugin_get_bundle_uri(plug);
		const char* libraryUri = lilv_node_as_uri(libraryUriNode);
		auto filename = AutoLilvPtr<char>(lilv_file_uri_parse(libraryUri, nullptr));
		if (filename)
		{
			new QLabel(QObject::tr("File: %1").arg(filename.get()), parent);
		}
	}
}




QString Lv2SubPluginFeatures::additionalFileExtensions(
	const Plugin::Descriptor::SubPluginFeatures::Key& k) const
{
	(void)k;
	// lv2 only loads .lv2 files
	// maybe add conversions later, e.g. for loading xmz
	return QString();
}




QString Lv2SubPluginFeatures::displayName(
	const Plugin::Descriptor::SubPluginFeatures::Key& k) const
{
	return pluginName(getPlugin(k));
}




QString Lv2SubPluginFeatures::description(
	const Plugin::Descriptor::SubPluginFeatures::Key& k) const
{
	auto mgr = Engine::getLv2Manager();
	const LilvPlugin* plug = mgr->getPlugin(k.attributes["uri"]);
	if (plug)
	{
		QString result;
		AutoLilvNode rdfs_comment{mgr->uri(LILV_NS_RDFS "comment")};
		AutoLilvNodes comments{lilv_plugin_get_value(plug, rdfs_comment.get())};
		if (comments)
		{
			result += lilv_node_as_string(lilv_nodes_get_first(comments.get()));
			result += "\n\n";
		}
		result += lilv_node_as_uri(lilv_plugin_get_uri(plug));
		return result.trimmed();
	}
	return QObject::tr("failed to load description");
}




const PixmapLoader* Lv2SubPluginFeatures::logo(
	const Plugin::Descriptor::SubPluginFeatures::Key& k) const
{
	(void)k;
	return nullptr; // Lv2 currently does not support this
}




void Lv2SubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* desc,
												KeyList& kl) const
{
	Lv2Manager* lv2Mgr = Engine::getLv2Manager();
	for (const auto& uriInfoPair : *lv2Mgr)
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


} // namespace lmms

#endif // LMMS_HAVE_LV2

