/*
 * UpgradeOld.cpp - upgrade functors for old methods
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

#include "datafile/UpgradeTo02.h"

#include "LocaleHelper.h"


namespace lmms
{

/*
 * Upgrade to 0.2.1-20070501
 *
 * Upgrade to version 0.2.1-20070501
 */
void UpgradeTo0_2_1_20070501::upgrade()
{
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "arpdir" ) )
		{
			int arpdir = el.attribute( "arpdir" ).toInt();
			if( arpdir > 0 )
			{
				el.setAttribute( "arpdir", arpdir - 1 );
			}
			else
			{
				el.setAttribute( "arpdisabled", "1" );
			}
		}
	}

	list = elementsByTagName( "sampletrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "vol" ) != "" )
		{
			el.setAttribute( "vol", LocaleHelper::toFloat(
					el.attribute( "vol" ) ) * 100.0f );
		}
		else
		{
			QDomNode node = el.namedItem(
						"automation-pattern" );
			if( !node.isElement() ||
				!node.namedItem( "vol" ).isElement() )
			{
				el.setAttribute( "vol", 100.0f );
			}
		}
	}

	list = elementsByTagName( "ladspacontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QDomNode anode = el.namedItem( "automation-pattern" );
		QDomNode node = anode.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() )
			{
				QString name = node.nodeName();
				if( name.endsWith( "link" ) )
				{
					el.setAttribute( name,
						node.namedItem( "time" )
						.toElement()
						.attribute( "value" ) );
					QDomNode oldNode = node;
					node = node.nextSibling();
					anode.removeChild( oldNode );
					continue;
				}
			}
			node = node.nextSibling();
		}
	}

	auto& head = m_document.head();
	QDomNode node = head.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "bpm" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					head.setAttribute( "bpm",
								value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "mastervol" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					head.setAttribute(
						"mastervol", value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "masterpitch" )
			{
				head.setAttribute( "masterpitch",
					-node.toElement().attribute(
						"value" ).toInt() );
				QDomNode oldNode = node;
				node = node.nextSibling();
				head.removeChild( oldNode );
				continue;
			}
		}
		node = node.nextSibling();
	}
}

/*
 * Upgrade to 0.2.1-20070508
 *
 * Upgrade to version 0.2.1-20070508 from some version greater than
 * or equal to 0.2.1-20070501
 */
void UpgradeTo0_2_1_20070508::upgrade()
{
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "chorddisabled" ) )
		{
			el.setAttribute( "chord-enabled",
				!el.attribute( "chorddisabled" )
							.toInt() );
			el.setAttribute( "arp-enabled",
				!el.attribute( "arpdisabled" )
							.toInt() );
		}
		else if( !el.hasAttribute( "chord-enabled" ) )
		{
			el.setAttribute( "chord-enabled", true );
			el.setAttribute( "arp-enabled",
				el.attribute( "arpdir" ).toInt() != 0 );
		}
	}

	while( !( list = elementsByTagName( "channeltrack" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "instrumenttrack" );
	}

	list = elementsByTagName( "instrumenttrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "vol" ) )
		{
			float value = LocaleHelper::toFloat( el.attribute( "vol" ) );
			value = roundf( value * 0.585786438f );
			el.setAttribute( "vol", value );
		}
		else
		{
			QDomNodeList vol_list = el.namedItem(
						"automation-pattern" )
					.namedItem( "vol" ).toElement()
					.elementsByTagName( "time" );
			for( int j = 0; !vol_list.item( j ).isNull();
								++j )
			{
				QDomElement timeEl = list.item( j )
							.toElement();
				int value = timeEl.attribute( "value" )
							.toInt();
				value = (int)roundf( value *
							0.585786438f );
				timeEl.setAttribute( "value", value );
			}
		}
	}
}

} // namespace lmms
