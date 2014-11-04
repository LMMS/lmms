/*
 * QtXmlWrapper.cpp - a QtXml based XML backend for ZynAddSubxFX
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

/* File derived from QtXmlWrapper.C: */
/*
  ZynAddSubFX - a software synthesizer

  QtXmlWrapper.C - XML wrapper
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Copyright (C) 2009-2009 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <QtXml/QDomDocument>
#include <QtCore/QDebug>
#include "QtXmlWrapper.h"
#include <string>
#include <sstream>
#include <cstdarg>
#include <zlib.h>
#include "lmmsconfig.h"
#include "../globals.h"
#include "Util.h"


struct XmlData
{
	XmlData() :
		m_doc( "ZynAddSubFX-data" )
	{
	}
	QDomDocument m_doc;
	QDomElement m_node;
	QDomElement m_info;

	QDomElement addparams( const char *name, unsigned int params, ... );
} ;



QtXmlWrapper::QtXmlWrapper() :
	d( new XmlData )
{
    version.Major    = 2;
    version.Minor    = 4;
    version.Revision = 1;

    minimal = true;

	d->m_node = d->m_doc.createElement( "ZynAddSubFX-data" );
	d->m_node.setAttribute( "version-major", QString::number( version.Major ) );
	d->m_node.setAttribute( "version-minor", QString::number( version.Minor ) );
	d->m_node.setAttribute( "version-revision", QString::number( version.Revision ) );
	d->m_node.setAttribute( "ZynAddSubFX-author", "Nasca Octavian Paul" );
	d->m_doc.appendChild( d->m_node );

    //make the empty branch that will contain the information parameters
    d->m_info = d->addparams("INFORMATION", 0);

    //save zynaddsubfx specifications
    beginbranch("BASE_PARAMETERS");
    addpar("max_midi_parts", NUM_MIDI_PARTS);
    addpar("max_kit_items_per_instrument", NUM_KIT_ITEMS);

    addpar("max_system_effects", NUM_SYS_EFX);
    addpar("max_insertion_effects", NUM_INS_EFX);
    addpar("max_instrument_effects", NUM_PART_EFX);

    addpar("max_addsynth_voices", NUM_VOICES);
    endbranch();
}




QtXmlWrapper::~QtXmlWrapper()
{
	delete d;
}




void QtXmlWrapper::setPadSynth(bool enabled)
{
    /**@bug this might create multiple nodes when only one is needed*/
    QDomElement oldNode = d->m_node;
    d->m_node = d->m_info;
    //Info storing
    addparbool("PADsynth_used", enabled);
    d->m_node = oldNode;
}

QDomElement findElement( QDomElement root, const QString & tagname, const QString & attrname,
						const QString & attrval )
{
	QDomNodeList list = root.elementsByTagName( tagname );
	for( int i = 0; i < list.size(); ++i )
	{
		QDomNode n = list.at( i );
		if( n.isElement() )
		{
			QDomElement e = n.toElement();
			if( e.hasAttribute( attrname ) && e.attribute( attrname ) == attrval )
			{
				return e;
			}
		}
	}

	return QDomElement();
}




bool QtXmlWrapper::hasPadSynth() const
{
    /**Right now this has a copied implementation of setparbool, so this should
     * be reworked as XMLwrapper evolves*/
    QDomElement tmp = d->m_doc.elementsByTagName( "INFORMATION" ).at( 0 ).toElement();
    QDomElement parameter = findElement( tmp, "par_bool", "name", "PADsynth_used" );
	if( !parameter.isNull() )
	{
		const QString val = parameter.attribute( "value" ).toLower();
		return val[0] == 'y';
	}
	return false;
}


/* SAVE XML members */

int QtXmlWrapper::saveXMLfile(const std::string &filename) const
{
    char *xmldata = getXMLdata();
    if(xmldata == NULL)
        return -2;

    int compression = config.cfg.GzipCompression;
    int result      = dosavefile(filename.c_str(), compression, xmldata);

    delete[] xmldata;
    return result;
}


char *QtXmlWrapper::getXMLdata() const
{
	QString xml = d->m_doc.toString( 1 );
	return qstrdup( xml.toUtf8().constData() );
}


int QtXmlWrapper::dosavefile(const char *filename,
                           int compression,
                           const char *xmldata) const
{
    if(compression == 0) {
        FILE *file;
        file = fopen(filename, "w");
        if(file == NULL)
            return -1;
        fputs(xmldata, file);
        fclose(file);
    }
    else {
        if(compression > 9)
            compression = 9;
        if(compression < 1)
            compression = 1;
        char options[10];
        snprintf(options, 10, "wb%d", compression);

        gzFile gzfile;
        gzfile = gzopen(filename, options);
        if(gzfile == NULL)
            return -1;
        gzputs(gzfile, xmldata);
        gzclose(gzfile);
    }

    return 0;
}



void QtXmlWrapper::addpar(const std::string &name, int val)
{
    d->addparams("par", 2, "name", name.c_str(), "value", stringFrom<int>(
                  val).c_str());
}

void QtXmlWrapper::addparreal(const std::string &name, float val)
{
    d->addparams("par_real", 2, "name", name.c_str(), "value",
              stringFrom<float>(val).c_str());
}

void QtXmlWrapper::addparbool(const std::string &name, int val)
{
    if(val != 0)
        d->addparams("par_bool", 2, "name", name.c_str(), "value", "yes");
    else
        d->addparams("par_bool", 2, "name", name.c_str(), "value", "no");
}

void QtXmlWrapper::addparstr(const std::string &name, const std::string &val)
{
	QDomElement e = d->m_doc.createElement( "string" );
	e.setAttribute( "name", name.c_str() );
	e.appendChild( d->m_doc.createTextNode( val.c_str() ) );
	d->m_node.appendChild( e );
}


void QtXmlWrapper::beginbranch(const std::string &name)
{
    d->m_node = d->addparams(name.c_str(), 0);
}

void QtXmlWrapper::beginbranch(const std::string &name, int id)
{
    d->m_node = d->addparams(name.c_str(), 1, "id", stringFrom<int>(id).c_str());
}

void QtXmlWrapper::endbranch()
{
	d->m_node = d->m_node.parentNode().toElement();
}



/* LOAD XML members */

int QtXmlWrapper::loadXMLfile(const std::string &filename)
{
    const char *xmldata = doloadfile(filename.c_str());
    if(xmldata == NULL)
	{
		qDebug() << "QtXmlWrapper::loadXMLfile(): empty data";
        return -1;                //the file could not be loaded or uncompressed
	}

	QByteArray b( xmldata );
	while( !b.isEmpty() && b[0] != '<' )
	{
		// remove first blank line
		b.remove( 0, 1 );
	}

	if( !d->m_doc.setContent( b ) )
	{
		qDebug() << "QtXmlWrapper::loadXMLfile(): could not set document content";
    	delete[] xmldata;
		return -2;
	}
    delete[] xmldata;

    d->m_node = d->m_doc.elementsByTagName( "ZynAddSubFX-data" ).at( 0 ).toElement();
    if( d->m_node.isNull() || !d->m_node.isElement() )
	{
		qDebug() << "QtXmlWrapper::loadXMLfile(): missing root node";
        return -3;             //the XML doesnt embbed zynaddsubfx data
	}
	QDomElement root = d->m_node.toElement();
    //fetch version information
    version.Major    = root.attribute( "version-major").toInt();
    version.Minor    = root.attribute( "version-minor").toInt();
    version.Revision = root.attribute( "version-revision").toInt();

    return 0;
}


char *QtXmlWrapper::doloadfile(const std::string &filename) const
{
    char  *xmldata = NULL;
    gzFile gzfile  = gzopen(filename.c_str(), "rb");

    if(gzfile != NULL) { //The possibly compressed file opened
        std::stringstream strBuf;             //reading stream
        const int    bufSize = 500;      //fetch size
        char fetchBuf[bufSize + 1];      //fetch buffer
        int  read = 0;                   //chars read in last fetch

        fetchBuf[bufSize] = 0; //force null termination

        while(bufSize == (read = gzread(gzfile, fetchBuf, bufSize)))
            strBuf << fetchBuf;

        fetchBuf[read] = 0; //Truncate last partial read
        strBuf << fetchBuf;

        gzclose(gzfile);

        //Place data in output format
        std::string tmp = strBuf.str();
        xmldata = new char[tmp.size() + 1];
        strncpy(xmldata, tmp.c_str(), tmp.size() + 1);
    }

    return xmldata;
}

bool QtXmlWrapper::putXMLdata(const char *xmldata)
{
    d->m_doc.setContent( QString::fromUtf8( xmldata ) );

    d->m_node = d->m_doc.elementsByTagName( "ZynAddSubFX-data" ).at( 0 ).toElement();
    if( d->m_node.isNull() )
	{
        return false;
	}

    return true;
}



int QtXmlWrapper::enterbranch(const std::string &name)
{
	QDomElement tmp = d->m_node.firstChildElement( name.c_str() );
	if( tmp.isNull() )
	{
		return 0;
	}

    d->m_node = tmp;

	return 1;
}

int QtXmlWrapper::enterbranch(const std::string &name, int id)
{
	QDomElement tmp = findElement( d->m_node, name.c_str(),
									"id", QString::number( id ) );
	if( tmp.isNull() )
	{
		return 0;
	}

    d->m_node = tmp;

	return 1;
}


void QtXmlWrapper::exitbranch()
{
	d->m_node = d->m_node.parentNode().toElement();
}


int QtXmlWrapper::getbranchid(int min, int max) const
{
	if( !d->m_node.isElement() )
	{
		return min;
	}
	QDomElement tmp = d->m_node.toElement();
	if( !tmp.hasAttribute( "id" ) )
	{
		return min;
	}
    int id = tmp.attribute( "id" ).toInt();
    if((min == 0) && (max == 0))
        return id;

    if(id < min)
        id = min;
    else
    if(id > max)
        id = max;

    return id;
}

int QtXmlWrapper::getpar(const std::string &name, int defaultpar, int min,
                       int max) const
{
	QDomElement tmp = findElement( d->m_node, "par", "name", name.c_str() );
	if( tmp.isNull() || !tmp.hasAttribute( "value" ) )
	{
		return defaultpar;
	}

    int val = tmp.attribute( "value" ).toInt();
    if(val < min)
        val = min;
    else
    if(val > max)
        val = max;

    return val;
}

int QtXmlWrapper::getpar127(const std::string &name, int defaultpar) const
{
    return getpar(name, defaultpar, 0, 127);
}

int QtXmlWrapper::getparbool(const std::string &name, int defaultpar) const
{
	QDomElement tmp = findElement( d->m_node, "par_bool", "name", name.c_str() );
	if( tmp.isNull() || !tmp.hasAttribute( "value" ) )
	{
		return defaultpar;
	}

    const QString val = tmp.attribute( "value" ).toLower();
	if( val[0] == 'y' )
	{
		return 1;
	}
	return 0;
}

void QtXmlWrapper::getparstr(const std::string &name, char *par, int maxstrlen) const
{
    ZERO(par, maxstrlen);
	QDomNode tmp = findElement( d->m_node, "string", "name", name.c_str() );
	if( tmp.isNull() || !tmp.hasChildNodes() )
	{
		return;
	}

	tmp = tmp.firstChild();
	if( tmp.nodeType() == QDomNode::ElementNode )
	{
		snprintf(par, maxstrlen, "%s", tmp.toElement().tagName().toUtf8().constData() );
		return;
	}
	if( tmp.nodeType() == QDomNode::TextNode )
	{
		snprintf(par, maxstrlen, "%s", tmp.toText().data().toUtf8().constData() );
		return;
	}
}

std::string QtXmlWrapper::getparstr(const std::string &name,
                             const std::string &defaultpar) const
{
	QDomNode tmp = findElement( d->m_node, "string", "name", name.c_str() );
	if( tmp.isNull() || !tmp.hasChildNodes() )
	{
		return defaultpar;
	}

	tmp = tmp.firstChild();
	if( tmp.nodeType() == QDomNode::ElementNode && !tmp.toElement().tagName().isEmpty() )
	{
		return tmp.toElement().tagName().toUtf8().constData();
	}
	if( tmp.nodeType() == QDomNode::TextNode && !tmp.toText().data().isEmpty() )
	{
		return tmp.toText().data().toUtf8().constData();
	}

    return defaultpar;
}

float QtXmlWrapper::getparreal(const char *name, float defaultpar) const
{
	QDomElement tmp = findElement( d->m_node, "par_real", "name", name );
	if( tmp.isNull() || !tmp.hasAttribute( "value" ) )
	{
		return defaultpar;
	}

    return tmp.attribute( "value" ).toFloat();
}

float QtXmlWrapper::getparreal(const char *name,
                                float defaultpar,
                                float min,
                                float max) const
{
    float result = getparreal(name, defaultpar);

    if(result < min)
        result = min;
    else
    if(result > max)
        result = max;
    return result;
}


/** Private members **/

QDomElement XmlData::addparams(const char *name, unsigned int params,
                                   ...)
{
    /**@todo make this function send out a good error message if something goes
     * wrong**/
    QDomElement element = m_doc.createElement( name );
	m_node.appendChild( element );

    if(params) {
        va_list variableList;
        va_start(variableList, params);

        const char *ParamName;
        const char *ParamValue;
        while(params--) {
            ParamName  = va_arg(variableList, const char *);
            ParamValue = va_arg(variableList, const char *);
            element.setAttribute( ParamName, ParamValue);
        }
    }
    return element;
}

