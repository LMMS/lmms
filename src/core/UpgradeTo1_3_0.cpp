/*
 * UpgradeTo1_3_0.cpp
 *   Functor for upgrading data files to 1.3.0
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

#include "UpgradeTo1_3_0.h"

#include <algorithm>
#include <cmath>

#include <QStringList>
#include <QDebug>


namespace lmms
{


/**
 *  Helper function to call a functor for all effect ports' DomElements,
 *  providing the functor with lists to add and remove DomElements. Helpful for
 *  patching port values from savefiles.
 */
template<class Ftor>
void iterate_ladspa_ports(QDomElement& effect, Ftor& ftor)
{
	// Head back up the DOM to upgrade ports
	QDomNodeList ladspacontrols = effect.elementsByTagName( "ladspacontrols" );
	for( int m = 0; !ladspacontrols.item( m ).isNull(); ++m )
	{
		QList<QDomElement> addList, removeList;
		QDomElement ladspacontrol = ladspacontrols.item( m ).toElement();
		for( QDomElement port = ladspacontrol.firstChild().toElement();
			!port.isNull(); port = port.nextSibling().toElement() )
		{
			QStringList parts = port.tagName().split("port");
			// Not a "port"
			if ( parts.size() < 2 )
			{
				continue;
			}
			int num = parts[1].toInt();

			// From Qt's docs of QDomNode:
			// * copying a QDomNode is OK, they still have the same
			//   pointer to the "internal" QDomNodePrivate.
			// * Also, they are using linked lists, which means
			//   deleting or appending QDomNode does not invalidate
			//   any other pointers.
			// => Inside ftor, you can (and should) push back the
			//    QDomElements by value, not references
			// => The loops below for adding and removing don't
			//    invalidate any other QDomElements
			ftor(port, num, addList, removeList);
		}

		// Add ports marked for adding
		for ( QDomElement e : addList )
		{
			ladspacontrol.appendChild( e );
		}
		// Remove ports marked for removal
		for ( QDomElement e : removeList )
		{
			ladspacontrol.removeChild( e );
		}
	}
}


// helper function if you need to print a QDomNode
QDebug operator<<(QDebug dbg, const QDomNode& node)
{
	QString s;
	QTextStream str(&s, QIODevice::WriteOnly);
	node.save(str, 2);
	dbg << qPrintable(s);
	return dbg;
}


void UpgradeTo1_3_0::upgrade_calf()
{
	if (attrName == "file" && (attrVal == "calf" || attrVal == "calf.so" ))
	{
		attribute.setAttribute( "value", "veal" );
	}
}


void UpgradeTo1_3_0::rename_plugins()
{
	const auto newName = pluginNames.find(plugin);
	if (newName != pluginNames.end())
	{
		attribute.setAttribute("value", newName->second);
	}
}


void UpgradeTo1_3_0::upgrade_ports()
{
	if (plugin == "MultibandLimiter" ||	plugin == "MultibandCompressor" || plugin == "MultibandGate")
	{
		upgrade_multiband_ports();
	}
	else if (plugin == "Pulsator")
	{
		upgrade_pulsator_ports();
	}
	else if (plugin == "VintageDelay")
	{
		upgrade_pulsator_ports();
	}
	else if (plugin == "Equalizer5Band" || plugin == "Equalizer8Band" || plugin == "Equalizer12Band")
	{
		upgrade_equalizer_ports();
	}
	else if (plugin == "Saturator")
	{
		upgrade_saturator_ports();
	}
	else if (plugin == "StereoTools")
	{
		upgrade_stereotools_ports();
	}
	else if (plugin == "amPitchshift")
	{
		upgrade_ampitchshift_ports();
	}
}


void UpgradeTo1_3_0::upgrade_multiband_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& removeList)
	{
		// Mark ports for removal
		if ( num >= 18 && num <= 23 )
		{
			removeList << port;
		}
		// Bump higher ports up 6 positions
		else if ( num >= 24 )
		{
			// port01...port010, etc
			QString name( "port0" );
			name.append( QString::number( num -6 ) );
			port.setTagName( name );
		}
	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_pulsator_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& removeList)
	{
		switch(num)
		{
			case 16:
			{
				// old freq is now at port 25
				QDomElement portCopy = createElement("port025");
				portCopy.setAttribute("data", port.attribute("data"));
				addList << portCopy;
				// remove old freq port
				removeList << port;
				// set the "timing" port to choose port23+2=port25 (timing in Hz)
				QDomElement timing = createElement("port022");
				timing.setAttribute("data", 2);
				addList << timing;
				break;
			}
			// port 18 (modulation) => 17
			case 17:
				port.setTagName("port016");
				break;
			case 18:
			{
				// leave port 18 (offsetr), but add port 17 (offsetl)
				QDomElement offsetl = createElement("port017");
				offsetl.setAttribute("data", 0.0f);
				addList << offsetl;
				// additional: bash port 21 to 1
				QDomElement pulsewidth = createElement("port021");
				pulsewidth.setAttribute("data", 1.0f);
				addList << pulsewidth;
				break;
			}
		}


	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_vintagedelay_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
	{
		switch(num)
		{
			case 4:
			{
				// BPM is now port028
				port.setTagName("port028");
				// bash timing to BPM
				QDomElement timing = createElement("port027");
				timing.setAttribute("data", 0);
				addList << timing;

				// port 5 and 6 (in, out gain) need to be bashed to 1:
				QDomElement input = createElement("port05");
				input.setAttribute("data", 1.0f);
				addList << input;
				QDomElement output = createElement("port06");
				output.setAttribute("data", 1.0f);
				addList << output;

				break;
			}
			default:
				// all other ports increase by 10
				QString name( "port0" );
				name.append( QString::number( num + 10 ) );
				port.setTagName( name );
		}


	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_equalizer_ports()
{
	// NBand equalizers got 4 q nobs inserted. We need to shift everything else...
	// HOWEVER: 5 band eq has only 2 q nobs inserted (no LS/HS filters)
	bool band5 = plugin == "Equalizer5Band";
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
	{
		if(num == 4)
		{
			// don't modify port 4, but some other ones:
			int zoom_port;
			if (plugin == "Equalizer5Band")
				zoom_port = 36;
			else if (plugin == "Equalizer8Band")
				zoom_port = 48;
			else // 12 band
				zoom_port = 64;
			// bash zoom to 0.25
			QString name( "port0" );
			name.append( QString::number( zoom_port ) );
			QDomElement timing = createElement(name);
			timing.setAttribute("data", 0.25f);
			addList << timing;
		}
		// the following code could be refactored, but I did careful code-reading
		// to prevent copy-paste-errors
		if(num == 18)
		{
			// 18 => 19
			port.setTagName("port019");
			// insert port 18 (q)
			QDomElement q = createElement("port018");
			q.setAttribute("data", 0.707f);
			addList << q;
		}
		else if(num >= 19 && num <= 20)
		{
			// num += 1
			QString name( "port0" );
			name.append( QString::number( num + 1 ) );
			port.setTagName( name );
		}
		else if(num == 21)
		{
			// 21 => 23
			port.setTagName("port023");
			// insert port 22 (q)
			QDomElement q = createElement("port022");
			q.setAttribute("data", 0.707f);
			addList << q;
		}
		else if(num >= 22 && (num <= 23 || band5))
		{
			// num += 2
			QString name( "port0" );
			name.append( QString::number( num + 2 ) );
			port.setTagName( name );
		}
		else if(num == 24 && !band5)
		{
			// 24 => 27
			port.setTagName("port027");
			// insert port 26 (q)
			QDomElement q = createElement("port026");
			q.setAttribute("data", 0.707f);
			addList << q;
		}
		else if(num >= 25 && num <= 26 && !band5)
		{
			// num += 3
			QString name( "port0" );
			name.append( QString::number( num + 3 ) );
			port.setTagName( name );
		}
		else if(num == 27 && !band5)
		{
			// 27 => 31
			port.setTagName("port031");
			// insert port 30 (q)
			QDomElement q = createElement("port030");
			q.setAttribute("data", 0.707f);
			addList << q;
		}
		else if(num >= 28 && !band5)
		{
			// num += 4
			QString name( "port0" );
			name.append( QString::number( num + 4 ) );
			port.setTagName( name );
		}
	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_saturator_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
	{
		// These ports have been shifted a bit weird...
		if( num == 7 )
		{
			port.setTagName("port015");
		}
		else if(num == 12)
		{
			port.setTagName("port016");
		}
		else if(num == 13)
		{
			port.setTagName("port017");
		}
		else if ( num >= 15 )
		{
			QString name( "port0" );
			name.append( QString::number( num + 3 ) );
			port.setTagName( name );
		}
	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_stereotools_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
	{
		// This effect can not be back-ported due to bugs in the old version,
		// or due to different behaviour. We thus port all parameters we can,
		// and bash all new parameters (in this case, s.level and m.level) to
		// their new defaults (both 1.0f in this case)

		if( num == 23 || num == 25 )
		{
			port.setAttribute("data", 1.0f);
		}
	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_ampitchshift_ports()
{
	auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& removeList)
	{
		switch (num)
		{
		case 0:
			port.setTagName("port01");
			break;
		case 1:
			port.setTagName("port03");
			break;
		case 10:
			port.setTagName("port11");
			break;
		case 11:
			port.setTagName("port13");
			break;
		}
	};
	iterate_ladspa_ports(effect, fn);
}


void UpgradeTo1_3_0::upgrade_effects()
{
	m_elements = elementsByTagName( "effect" );
	for (int i = 0; i < m_elements.length(); ++i)
	{
		effect = m_elements.item(i).toElement();
		if (effect.isNull()) { continue; }
		if(effect.attribute( "name" ) == "ladspaeffect" )
		{
			QDomNodeList keys = effect.elementsByTagName( "key" );
			for( int j = 0; !keys.item( j ).isNull(); ++j )
			{
				QDomElement key = keys.item( j ).toElement();
				QDomNodeList attributes = key.elementsByTagName( "attribute" );
				for( int k = 0; !attributes.item( k ).isNull(); ++k )
				{
					// Effect name changes
					attribute = attributes.item( k ).toElement();
					attrName = attribute.attribute("name");
					attrVal = attribute.attribute("value");
					plugin = attrName == "plugin" ? attrVal : "";

					// Handle calf rename to veal
					upgrade_calf();

					// Handle port changes
					upgrade_ports();
				}
			}
		}
	}
}


void UpgradeTo1_3_0::upgrade()
{
	m_elements = m_document.elementsByTagName( "instrument" );
	for (int i = 0; i < m_elements.length(); ++i)
	{
		QDomElement el = m_elements.item(i).toElement();
		if (el.isNull()) { continue; }
		if (el.attribute("name") == "papu")
		{
			el.setAttribute("name", "freeboy");
			QDomNodeList children = el.elementsByTagName("papu");
			renameElements(children, "freeboy");
		}
		else if (el.attribute( "name" ) == "OPL2")
		{
			el.setAttribute("name", "opulenz");
			QDomNodeList children = el.elementsByTagName("OPL2");
			renameElements(children, "opulenz");
		}
	}

	upgrade_effects();
}


} // namespace lmms
