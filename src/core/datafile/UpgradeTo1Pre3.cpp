/*
 * UpgradeTo1Pre3.cpp
 *   Functors for upgrading data files from 1.0 through 1.2.2 stable
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

#include "datafile/UpgradeTo1Pre3.h"

#include "lmms_basics.h"

#include <QList>


namespace lmms
{

/*
 * Upgrade to 1.0.99
 *
 * Upgrade to version 1.0.99 from some version less than 1.0.99
 */
void UpgradeTo1_0_99::upgrade()
{
	jo_id_t last_assigned_id = 0;

	QList<jo_id_t> idList;
	findIds(documentElement(), idList);

	QDomNodeList list = elementsByTagName("ladspacontrols");
	for(int i = 0; !list.item(i).isNull(); ++i)
	{
		for(QDomNode node = list.item(i).firstChild(); !node.isNull();
			node = node.nextSibling())
		{
			QDomElement el = node.toElement();
			QDomNode data_child = el.namedItem("data");
			if(!data_child.isElement())
			{
				if (el.attribute("scale_type") == "log")
				{
					QDomElement me = createElement("data");
					me.setAttribute("value", el.attribute("data"));
					me.setAttribute("scale_type", "log");

					jo_id_t id;
					for(id = last_assigned_id + 1;
						idList.contains(id); id++)
					{
					}

					last_assigned_id = id;
					idList.append(id);
					me.setAttribute("id", id);
					el.appendChild(me);

				}
			}
		}
	}
}


/*
 * Upgrade to 1.1.0
 *
 * Upgrade to version 1.1.0 from some version less than 1.1.0
 */
void UpgradeTo1_1_0::upgrade()
{
	QDomNodeList list = elementsByTagName("fxchannel");
	for (int i = 1; !list.item(i).isNull(); ++i)
	{
		QDomElement el = list.item(i).toElement();
		QDomElement send = createElement("send");
		send.setAttribute("channel", "0");
		send.setAttribute("amount", "1");
		el.appendChild(send);
	}
}


/*
 * Upgrade to 1.1.19
 *
 * Upgrade to version 1.1.91 from some version less than 1.1.91
 */
void UpgradeTo1_1_91::upgrade()
{
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( QRegExp("/samples/bassloopes/"), "/samples/bassloops/" );
		el.setAttribute( "src", s );
	}

	list = elementsByTagName( "attribute" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "name" ) == "plugin" && el.attribute( "value" ) == "vocoder-lmms" ) {
			el.setAttribute( "value", "vocoder" );
		}
	}

	list = elementsByTagName( "crossoevereqcontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// invert the mute LEDs
		for( int j = 1; j <= 4; ++j ){
			QString a = QString( "mute%1" ).arg( j );
			el.setAttribute( a, ( el.attribute( a ) == "0" ) ? "1" : "0" );
		}
	}

	list = elementsByTagName( "arpeggiator" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// Swap elements ArpDirRandom and ArpDirDownAndUp
		if( el.attribute( "arpdir" ) == "3" )
		{
			el.setAttribute( "arpdir", "4" );
		}
		else if( el.attribute( "arpdir" ) == "4" )
		{
			el.setAttribute( "arpdir", "3" );
		}
	}
}


/*
 * Upgrade to 1.2.0 rc3
 *
 * Upgrade from earlier bbtrack beat note behaviour of adding steps if
 * a note is placed after the last step.
 */
void UpgradeTo1_2_0_RC3::upgrade()
{
	QDomNodeList bbtracks = elementsByTagName( "bbtrack" );
	for( int i = 0; !bbtracks.item( i ).isNull(); ++i )
	{
		QDomNodeList patterns = bbtracks.item( i
				).toElement().elementsByTagName(
								"pattern" );
		for( int j = 0; !patterns.item( j ).isNull(); ++j )
		{
			int patternLength, steps;
			QDomElement el = patterns.item( j ).toElement();
			if( el.attribute( "len" ) != "" )
			{
				patternLength = el.attribute( "len" ).toInt();
				steps = patternLength / 12;
				el.setAttribute( "steps", steps );
			}
		}
	}

	QDomElement el = firstChildElement();
	while ( !el.isNull() )
	{
		upgradeElementRc2_42( el );
		el = el.nextSiblingElement();
	}
}


void UpgradeTo1_2_0_RC3::upgradeElementRc2_42( QDomElement& el )
{
	if( el.hasAttribute( "syncmode" ) )
	{
		int syncmode = el.attribute( "syncmode" ).toInt();
		QStringList names;
		QDomNamedNodeMap atts = el.attributes();
		for( uint i = 0; i < atts.length(); i++ )
		{
			QString name = atts.item( i ).nodeName();
			if( name.endsWith( "_numerator" ) )
			{
				names << name.remove( "_numerator" )
								+ "_syncmode";
			}
		}
		for( QStringList::iterator it = names.begin(); it < names.end();
									++it )
		{
			el.setAttribute( *it, syncmode );
		}
	}

	QDomElement child = el.firstChildElement();
	while ( !child.isNull() )
	{
		upgradeElementRc2_42( child );
		child = child.nextSiblingElement();
	}
}


} // namespace lmms
