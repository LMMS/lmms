/*
 * UpgradeTo04.cpp
 *   Functor for upgrading data files from 0.3.0 through 0.4.0
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

#include "datafile/UpgradeTo04.h"

#include <QStringList>
#include <QVariant>

#include "base64.h"
#include "Effect.h"


namespace lmms
{

/*
 * Upgrade to 0.4.0-20080104
 *
 * Upgrade to version 0.4.0-20080104 from some version greater than
 * or equal to 0.3.0 (final)
 */
void UpgradeTo0_4_0_20080104::upgrade()
{
	QDomNodeList list = elementsByTagName( "fx" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "fxdisabled" ) &&
			el.attribute( "fxdisabled" ).toInt() == 0 )
		{
			el.setAttribute( "enabled", 1 );
		}
	}
}

/*
 * Upgrade to 0.4.0-20080118
 *
 * Upgrade to version 0.4.0-20080118 from some version greater than
 * or equal to 0.4.0-20080104
 */
void UpgradeTo0_4_0_20080118::upgrade()
{
	QDomNodeList list;
	while( !( list = elementsByTagName( "fx" ) ).isEmpty() )
	{
		QDomElement fxchain = list.item( 0 ).toElement();
		fxchain.setTagName( "fxchain" );
		QDomNode rack = list.item( 0 ).firstChild();
		QDomNodeList effects = rack.childNodes();
		// move items one level up
		while( effects.count() )
		{
			fxchain.appendChild( effects.at( 0 ) );
		}
		fxchain.setAttribute( "numofeffects",
			rack.toElement().attribute( "numofeffects" ) );
		fxchain.removeChild( rack );
	}
}

/*
 * Upgrade to 0.4.0-20080129
 *
 * Upgrade to version 0.4.0-20080129 from some version greater than
 * or equal to 0.4.0-20080118
 */
void UpgradeTo0_4_0_20080129::upgrade()
{
	QDomNodeList list;
	while( !( list =
		elementsByTagName( "arpandchords" ) ).isEmpty() )
	{
		QDomElement aac = list.item( 0 ).toElement();
		aac.setTagName( "arpeggiator" );
		QDomNode cloned = aac.cloneNode();
		cloned.toElement().setTagName( "chordcreator" );
		aac.parentNode().appendChild( cloned );
	}
}

/*
 * Upgrade to 0.4.0-20080409
 *
 * Upgrade to version 0.4.0-20080409 from some version greater than
 * or equal to 0.4.0-20080129
 */
void UpgradeTo0_4_0_20080409::upgrade()
{
	QStringList s;
	s << "note" << "pattern" << "bbtco" << "sampletco" << "time";
	for( QStringList::iterator it = s.begin(); it < s.end(); ++it )
	{
		QDomNodeList list = elementsByTagName( *it );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			el.setAttribute( "pos",
				el.attribute( "pos" ).toInt()*3 );
			el.setAttribute( "len",
				el.attribute( "len" ).toInt()*3 );
		}
	}
	QDomNodeList list = elementsByTagName( "timeline" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		el.setAttribute( "lp0pos",
			el.attribute( "lp0pos" ).toInt()*3 );
		el.setAttribute( "lp1pos",
			el.attribute( "lp1pos" ).toInt()*3 );
	}
}

/*
 * Upgrade to 0.4.0-20080607
 *
 * Upgrade to version 0.4.0-20080607 from some version greater than
 * or equal to 0.3.0-20080409
 */
void UpgradeTo0_4_0_20080607::upgrade()
{
	QDomNodeList list;
	while( !( list = elementsByTagName( "midi" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "midiport" );
	}
}

/*
 * Upgrade to 0.4.0-20080622
 *
 * Upgrade to version 0.4.0-20080622 from some version greater than
 * or equal to 0.3.0-20080607
 */
void UpgradeTo0_4_0_20080622::upgrade()
{
	QDomNodeList list;
	while( !( list = elementsByTagName(
				"automation-pattern" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "automationpattern" );
	}

	list = elementsByTagName( "bbtrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "name" );
		s.replace( QRegExp( "^Beat/Baseline " ),
						"Beat/Bassline " );
		el.setAttribute( "name", s );
	}
}

/*
 * Upgrade to 0.4.0-beta1
 *
 * Upgrade to version 0.4.0-beta1 from some version greater than
 * or equal to 0.4.0-20080622
 * convert binary effect-key-blobs to XML
 */
void UpgradeTo0_4_0_beta1::upgrade()
{
	QDomNodeList list;
	list = elementsByTagName( "effect" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString k = el.attribute( "key" );
		if( !k.isEmpty() )
		{
			const QList<QVariant> l =
				base64::decode( k, QVariant::List ).toList();
			if( !l.isEmpty() )
			{
				QString name = l[0].toString();
				QVariant u = l[1];
				EffectKey::AttributeMap m;
				// VST-effect?
				if( u.type() == QVariant::String )
				{
					m["file"] = u.toString();
				}
				// LADSPA-effect?
				else if( u.type() == QVariant::StringList )
				{
					const QStringList sl = u.toStringList();
					m["plugin"] = sl.value( 0 );
					m["file"] = sl.value( 1 );
				}
				EffectKey key( nullptr, name, m );
				el.appendChild( key.saveXML( m_document ) );
			}
		}
	}
}

/*
 * Upgrade to 0.4.0-rc2
 *
 * Upgrade to version 0.4.0-rc2 from some version greater than
 * or equal to 0.4.0-beta1
 */
void UpgradeTo0_4_0_rc2::upgrade()
{
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( "drumsynth/misc ", "drumsynth/misc_" );
		s.replace( "drumsynth/r&b", "drumsynth/r_n_b" );
		s.replace( "drumsynth/r_b", "drumsynth/r_n_b" );
		el.setAttribute( "src", s );
	}
	list = elementsByTagName( "lb302" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		int s = el.attribute( "shape" ).toInt();
		if( s >= 1 )
		{
			s--;
		}
		el.setAttribute( "shape", QString("%1").arg(s) );
	}
}


} // namespace lmms
